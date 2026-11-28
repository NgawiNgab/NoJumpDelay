#include "kaura/re/Hooks.h"

#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#include "kaura/AuraCore.h"
#include "kaura/re/GameAccess.h"
#include "kaura/re/Logger.h"
#include "kaura/re/Offsets.h"
#include "kaura/re/Resolver.h"
#include "pl/memory/Hook.hpp"

namespace kaura::re {

namespace {

GameApi g_api;
bool g_apiResolved = false;

// ---- raw struct writers ----------------------------------------------------
inline void writeInt32(void *base, std::uint64_t off, std::int32_t v) {
    std::memcpy(reinterpret_cast<char *>(base) + off, &v, sizeof(v));
}
inline void writeFloat(void *base, std::uint64_t off, float v) {
    std::memcpy(reinterpret_cast<char *>(base) + off, &v, sizeof(v));
}

// ---- detours ---------------------------------------------------------------

// sigma detour 0x0ABB30 — KillAura engine on the LocalPlayer tick.
// The original runs LAST (sigma tails into the trampoline).
void normalTickDetour(void *lp) {
    HookManager &hm = HookManager::instance();
    AuraEngine &eng = hm.engine();

    // sigma entry guards (panic flag not ported -> pass; lp+0x228 non-null).
    const void *pos = readPtr(lp, OFF_LP_STATEVECTOR);
    if (g_api.valid() && g_api.getComponentFn != nullptr && pos != nullptr) {
        Vec3 eye = readVec3(pos);
        eye.y -= 1.62f; // sigma DAT_004586B8

        // sigma: selfAlive && gameMode && lp[0x1E0] && level && level[0x1D8].
        void *level = readPtr(lp, OFF_LP_LEVEL);
        void *gameMode = readPtr(lp, OFF_LP_GAMEMODE);
        void *dimRef = readPtr(lp, 0x1E0);
        void *descOwner = level != nullptr ? readPtr(level, OFF_LEVEL_ACTOR_SUBSYS)
                                           : nullptr;
        bool selfAlive = false;
        {
            const auto fn = reinterpret_cast<void *(*)(const void *, std::uint32_t)>(
                g_api.getComponentFn);
            const void *comp = fn(lp, g_api.compIdA);
            selfAlive = comp != nullptr &&
                        readFloat(comp, OFF_COMPONENT_HEALTH) != 0.0f;
        }

        if (selfAlive && gameMode != nullptr && dimRef != nullptr &&
            level != nullptr && descOwner != nullptr) {
            const std::string selfName =
                readCppString(reinterpret_cast<const char *>(lp) + OFF_ACTOR_NAME);

            // fetch actors (AAPCS sret in x8 — vector by value, 1:1).
            std::vector<void *> actors;
            if (g_api.fetchActorsFn != nullptr) {
                const auto fetch = reinterpret_cast<std::vector<void *> (*)(void *)>(
                    g_api.fetchActorsFn);
                actors = fetch(level); // sret buffer handled by the ABI thunk
            }

            std::vector<ActorSnapshot> snaps;
            for (void *actor : actors) {
                if (actor == nullptr || actor == lp) continue; // sigma self-skip
                ActorSnapshot snap;
                if (!buildSnapshot(g_api, lp, actor, selfName, snap)) continue;
                snaps.push_back(std::move(snap));
            }

            // sigma: unconditional 0.6 width write for accepted entities.
            eng.onEntityAccepted = [](const ActorSnapshot &s) {
                if (s.aabbComp != nullptr) {
                    writeFloat(s.aabbComp, OFF_AABB_WIDTH, 0.6f);
                }
            };

            const AuraEngine::TickResult res =
                eng.tick(eye, selfName, snaps, HookManager::nowSteadyNs());
            eng.onEntityAccepted = nullptr;

            // Execute requests in sigma's order.
            const auto attack = reinterpret_cast<void *(*)(void *, void *,
                                                           std::uint32_t,
                                                           const Vec3 *)>(
                g_api.attackFn);
            for (const AttackRequest &r : res.attacks) {
                if (r.writeDesc) {
                    // sigma lVar22 = level[0x1D8]: +0x18 = 1, +0x2C = hitY,
                    // +0x30 = range, +0x34 = dz, then GameMode::attack.
                    writeInt32(descOwner, 0x18, 1);
                    writeFloat(descOwner, 0x2C, r.hitPos.y);
                    writeFloat(descOwner, 0x30, r.range);
                    writeFloat(descOwner, 0x34, r.dz);
                }
                attack(gameMode, r.actor, 1, &r.hitPos);
            }

            const auto swing = reinterpret_cast<void *(*)(void *, std::uint32_t,
                                                          std::uint32_t)>(
                g_api.swingFn);
            for (int i = 0; i < res.swingCount; ++i) {
                swing(lp, 4, 0); // sigma: (*DAT_00457C00)(param_1, 4, 0)
            }

            if (res.runEmulatePulse) {
                void *interaction = readPtr(lp, OFF_LP_INTERACTION);
                if (interaction != nullptr) {
                    reinterpret_cast<void (*)(void *)>(g_api.enaAFn)(interaction);
                    reinterpret_cast<void (*)(void *)>(g_api.enaBFn)(interaction);
                }
            }
        }
    }

    // sigma: trampoline call at the very end.
    reinterpret_cast<void (*)(void *)>(hm.origNormalTick())(lp);
}

// sigma detour 0x0AB7D4 — rotations injector. Writes the engine's yaw/pitch
// into (param_2 + 0x238) then tails into the original.
void rotationApplyDetour(void *a1, void *a2) {
    HookManager &hm = HookManager::instance();
    AuraEngine &eng = hm.engine();
    if (a2 != nullptr && eng.rotationActive() && eng.settings().rotations) {
        writeFloat(a2, OFF_ACTOR_ROTATION, eng.rotation().yaw);
        writeFloat(a2, OFF_ACTOR_ROTATION + 4, eng.rotation().pitch);
    }
    reinterpret_cast<void (*)(void *, void *)>(hm.origRotationApply())(a1, a2);
}

} // namespace

HookManager &HookManager::instance() {
    static HookManager inst;
    return inst;
}

std::int64_t HookManager::nowSteadyNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool HookManager::install(const AuraSettings &settings) {
    if (installed_) return true;
    engine_.setSettings(settings);

    if (!g_apiResolved) {
        g_apiResolved = resolveGameApi(g_api);
        if (!g_apiResolved) return false;
    }

    // ---- in-memory prologue verification BEFORE hooking (user constraint) --
    unsigned char actual[16];
    if (!verifyPrologue(g_api.base, RVA_NORMAL_TICK, PROLOGUE_NORMAL_TICK, 16,
                        actual)) {
        logError("NormalTick prologue mismatch at base+0x%x — not hooking",
                 RVA_NORMAL_TICK);
        return false;
    }
    if (!verifyPrologue(g_api.base, RVA_ROTATION_APPLY, PROLOGUE_ROTATION_APPLY,
                        16, actual)) {
        logError("RotationApply prologue mismatch at base+0x%x — not hooking",
                 RVA_ROTATION_APPLY);
        return false;
    }
    logInfo("prologues verified; installing hooks");

    // ---- hook installs (pl::memory::hook == preloader's shadowhook wrapper) -
    if (pl::memory::hook(reinterpret_cast<void *>(g_api.base + RVA_NORMAL_TICK),
                         reinterpret_cast<void *>(&normalTickDetour),
                         &origNormalTick_) != 0) {
        logError("failed to hook NormalTick");
        return false;
    }
    if (pl::memory::hook(reinterpret_cast<void *>(g_api.base + RVA_ROTATION_APPLY),
                         reinterpret_cast<void *>(&rotationApplyDetour),
                         &origRotationApply_) != 0) {
        pl::memory::unhook(reinterpret_cast<void *>(g_api.base + RVA_NORMAL_TICK),
                           reinterpret_cast<void *>(&normalTickDetour));
        logError("failed to hook RotationApply");
        return false;
    }
    installed_ = true;
    logInfo("hooks installed (NormalTick, RotationApply)");
    return true;
}

void HookManager::uninstall() {
    if (!installed_) return;
    pl::memory::unhook(reinterpret_cast<void *>(g_api.base + RVA_NORMAL_TICK),
                       reinterpret_cast<void *>(&normalTickDetour));
    pl::memory::unhook(reinterpret_cast<void *>(g_api.base + RVA_ROTATION_APPLY),
                       reinterpret_cast<void *>(&rotationApplyDetour));
    origNormalTick_ = nullptr;
    origRotationApply_ = nullptr;
    installed_ = false;
}

} // namespace kaura::re
