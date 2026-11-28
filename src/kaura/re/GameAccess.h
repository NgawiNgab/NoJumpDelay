#pragma once

// Thin bindings onto the decoded 1.26.60 function table (sigma fn slots
// 0x457B70..0x457C48, decoded by emulating JNI_OnLoad — see
// research/KILLAURA_1_26_60_RE.md §3). Every pointer is resolved from the
// libminecraftpe.so base by exact RVA (no signatures — BedrockTools sigs are
// proven to target a different build) and verified against expected prologue
// bytes before use.

#include <cstdint>
#include <string>
#include <vector>

#include "kaura/AuraCore.h"
#include "kaura/AuraTypes.h"

namespace kaura::re {

// libc++ std::string SSO read (sigma layout: byte0&1 = long form).
std::string readCppString(const void *strObj);

struct GameApi {
    std::uintptr_t base{0};

    // std::vector<void*> fetchActors(Level*) — AAPCS sret (x8) call,
    // sigma fn slot 0x457B78 (target ldr x0,[x0,#0x478]).
    void *fetchActorsFn{};

    // void* getComponent(Actor*, unsigned compId) — slot 0x457BA0.
    void *getComponentFn{};

    // void* getNameComponent(Actor*) — slot 0x457BB0; string at comp+0x48.
    void *getNameComponentFn{};

    // bool interactionCheck(Actor*) — slot 0x457BA8 (registry lookup).
    void *interactionCheckFn{};

    // void GameMode::attack(GameMode*, Actor*, bool, Vec3 const*) — 0x457B88.
    void *attackFn{};

    // void swing(LocalPlayer*, int, int) — slot 0x457C00; sigma (lp, 4, 0).
    void *swingFn{};

    // emulateNativeAttack pulse pair — slots 0x457C10 / 0x457C30,
    // both called with lp[0xCA8].
    void *enaAFn{};
    void *enaBFn{};

    // Component IDs decoded into sigma slots 0x457C38 / 0x457C40 / 0x457C48
    // (data constants — getComponent arguments, not code).
    std::uint32_t compIdA{};
    std::uint32_t compIdB{};
    std::uint32_t compIdC{};

    bool valid() const { return base != 0 && attackFn != nullptr; }
};

// Resolves + verifies the full API; returns false (with logs) on any mismatch.
bool resolveGameApi(GameApi &api);

// ---- raw struct readers (exact sigma offsets) ------------------------------
Vec3 readVec3(const void *ptr);                       // actor/localplayer +0x228
void *readPtr(const void *base, std::uint64_t off);   // *(T**)((char*)base+off)
std::int32_t readInt32(const void *base, std::uint64_t off);
float readFloat(const void *base, std::uint64_t off);

// sigma FUN_001AECD0: tag-string object fetch (returns a std::string* or a
// static empty-string stand-in).
const std::string *fetchTagString(const GameApi &api, const void *actor);

// Per-actor snapshot build — exact sigma read order (see Hooks.cpp).
bool buildSnapshot(const GameApi &api, const void *lp, const void *actor,
                   const std::string &selfName, ActorSnapshot &out);

} // namespace kaura::re
