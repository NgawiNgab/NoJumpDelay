#pragma once

// ============================================================================
// AuraCore — 1:1 port of the libReverseSigma KillAura engine (FUN_001ABB30),
// its CPS scheduler (FUN_001AE86C) and the math helpers FUN_001AEF80 /
// FUN_001AEDE8 / FUN_001AFA60, restructured behind a thin value-snapshot
// interface so the decision logic can be unit-tested on the host.
//
// The game binding layer (kaura/re/Hooks.cpp) builds ActorSnapshots with the
// exact read sequence sigma uses; this file replicates sigma's decisions,
// thresholds and call order exactly:
//   - antiFireball branch: present in sigma (FNV 0xE1BED951) but gated by a
//     feature flag outside this port's scope -> constant false (documented).
//   - mobAura / targetLock / autoClicker / ladderFly / noJumpDelay / hitBox
//     are NOT part of the requested sub-modules and are not ported.
// ============================================================================

#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "kaura/AuraTypes.h"

namespace kaura {

// ---- per-actor snapshot (built by the game binding layer) ------------------
struct ActorSnapshot {
    void *actor{};
    bool namedPath{};      // sigma interaction-check(actor) result: true ->
                           // killAura collect path, false -> nametag path
                           // (fireball / item entities)
    Vec3 pos;              // *(actor + 0x228)
    float aabbWidth{};     // *(aabbComp + 0x18) — also sigma's antiBot check
    float aabbHeight{};    // *(aabbComp + 0x1C)
    std::string nameTag;   // name-tag component string (sigma DAT_00457BB0)
    std::string tagStr;    // sigma FUN_001AECD0 fetch (antiBot / botList / friend)
    std::string actorName; // embedded actor+0xB28 string (self-skip memcmp)
    int health{};          // *(getComponent(actor, compId[0x457C38]) + 0x7C)
    int hurtTime{};        // *(int*)(actor + 0x1B4)
    void *aabbComp{};      // *(actor + 0x230) — binding writes width 0.6 here
                           // (sigma's unconditional non-friend/bot hitbox write)
};

// Attack request — the binding layer performs the attack-desc writes and the
// GameMode::attack call in sigma's order.
struct AttackRequest {
    void *actor{};
    Vec3 hitPos;          // position passed to GameMode::attack (post-jitter)
    float range{};        // attack-desc +0x30 (killRange)
    float dz{};           // attack-desc +0x34
    bool writeDesc{};     // write attack-desc fields (killAura only; items skip)
};

struct RotationUpdate {
    float yaw{};
    float pitch{};
};

// AABB box as sigma reads it: raw width (+0x18) and raw height (+0x1C).
struct AabbBox {
    float rawWidth{};
    float rawHeight{};
};

// ---- pure math helpers (FUN_001AEDE8 / FUN_001AEF80 / FUN_001AFA60) --------
// FUN_001AEDE8: clamp eye into the AABB [feet-height, feet] on Y and
// [t-half, t+half] on X/Z (half = rawWidth * 0.5), signed per-axis delta.
float closestDistanceAABB(const Vec3 &eye, const Vec3 &feet, const AabbBox &box);

// FUN_001AEF80: pitch = atan2f(feetY - eyeY, hdist) * -57.295776f
// (+/-90 when the clamped horizontal distance <= 0.001), yaw =
// atan2f(clampedDx, clampedDz) (computed-but-discarded in the original).
RotationUpdate computeRotations(const Vec3 &eye, const Vec3 &feet,
                                const AabbBox &box);

// FUN_001AFA60: MT19937 uniform jitter, x/z scaled by rawWidth, y by rawHeight.
// Mutates pos in place, returns the new y (float return register).
float jitterHitPoint(std::mt19937 &rng, Vec3 &pos, const AabbBox &box);

// FNV-1a over the string bytes (sigma's do-while exits on the NUL — the
// terminator itself is not mixed in).
std::uint32_t fnv1a(const std::string &s);

// ---- CPS scheduler (FUN_001AE86C, channel A = killAura) --------------------
struct CpsChannel {
    std::int64_t lastFireNs{0};
    int rolledCps{10}; // current randomized CPS (re-rolled on every fire)
    bool tick{false};  // set for exactly one engine tick after an interval
};

// nowNs: steady_clock sample. Returns true when this tick should attack.
// On fire: re-roll rand() % ((max-min)+1) + min, clamp [1, 20] (sigma).
bool updateCpsChannel(CpsChannel &ch, std::int64_t nowNs, int minCps, int maxCps,
                      int (*roll)(int min, int max));

// ---- engine ----------------------------------------------------------------
class AuraEngine {
public:
    explicit AuraEngine(AuraSettings settings);

    void setSettings(const AuraSettings &settings) { settings_ = settings; }
    const AuraSettings &settings() const { return settings_; }
    const CpsChannel &cps() const { return cps_; }

    // Rotation state consumed by the rotation-injector detour.
    bool rotationActive() const { return rotationActive_; }
    const RotationUpdate &rotation() const { return rotation_; }

    // emulateNativeAttack pulse flag (sigma DAT_004586C4).

    struct TickResult {
        std::vector<AttackRequest> attacks; // execute in order
        int swingCount{};                   // swing after each attack (sigma)
        bool runEmulatePulse{};             // tail: c10/c30 on lp[0xCA8]
    };

    // One full engine tick. `eyeIn` = local player eye (pos.y - 1.62 applied
    // by the binding layer, sigma order). `selfName` = embedded lp+0xB28
    // string. Returns the requests the binding layer must execute.
    TickResult tick(const Vec3 &eyeIn, const std::string &selfName,
                    const std::vector<ActorSnapshot> &actors, std::int64_t nowNs);

    // Invoked during the scan for every named-path entity that passed the
    // bot/friend gates (sigma's unconditional AABB-width write point, before
    // the health gate). The binding writes width 0.6 there (1:1).
    std::function<void(const ActorSnapshot &)> onEntityAccepted;

    // Exposed for white-box tests.
    const std::vector<ActorSnapshot> &lastCandidates() const { return candidates_; }
    void seedRng(std::uint32_t seed) { rng_.seed(seed); }

private:
    AuraSettings settings_;
    CpsChannel cps_{};
    std::mt19937 rng_{std::random_device{}()};
    std::vector<ActorSnapshot> candidates_;
    bool rotationActive_{false};
    RotationUpdate rotation_{};
};

} // namespace kaura
