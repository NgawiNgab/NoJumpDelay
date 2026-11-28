#include "kaura/AuraCore.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace kaura {

namespace {

// Per-axis signed delta eye - clamp(eye, lo, hi) — the asymmetric clamp sigma
// compiles to (FUN_001AEDE8): above -> positive (eye-hi), below -> negative
// (eye-lo), inside -> 0.
float axisDelta(float eye, float lo, float hi) {
    float v = eye;
    if (v > hi) v = hi;
    if (v < lo) v = lo;
    return eye - v;
}

} // namespace

// ============================================================================
// Math helpers (exact FUN_001AEDE8 / FUN_001AEF80 / FUN_001AFA60 semantics)
// ============================================================================

float closestDistanceAABB(const Vec3 &eye, const Vec3 &feet, const AabbBox &box) {
    // param_3[0] = aabb+0x18 (raw width) — half extent = width * 0.5.
    const float half = box.rawWidth * 0.5f;
    const float dx = axisDelta(eye.x, feet.x - half, feet.x + half);
    // Y range = [feet.y - height, feet.y] (sigma: fVar4 = fVar5 - param_3[1]).
    const float dy = axisDelta(eye.y, feet.y - box.rawHeight, feet.y);
    const float dz = axisDelta(eye.z, feet.z - half, feet.z + half);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

RotationUpdate computeRotations(const Vec3 &eye, const Vec3 &feet,
                                const AabbBox &box) {
    // FUN_001AEF80: clamped horizontal distance + raw vertical delta.
    const float half = box.rawWidth * 0.5f;
    const float dxc = -axisDelta(eye.x, feet.x - half, feet.x + half); // lo'-eye.x
    const float dzc = -axisDelta(eye.z, feet.z - half, feet.z + half);
    const float hdist = std::sqrt(dxc * dxc + dzc * dzc);
    const float dyRaw = feet.y - eye.y; // raw (unclamped) vertical delta

    RotationUpdate r;
    if (hdist <= 0.001f) {
        r.pitch = (dyRaw <= 0.0f) ? 90.0f : -90.0f;
    } else {
        r.pitch = atan2f(dyRaw, hdist) * -57.295776f;
    }
    r.yaw = atan2f(dxc, dzc) * 57.295776f;
    return r;
}

float jitterHitPoint(std::mt19937 &rng, Vec3 &pos, const AabbBox &box) {
    // FUN_001AFA60: u01 in [0,1) -> r = u01*2-1 in [-1,1); per axis:
    //   x/z scaled by aabb+0x18 (width), y scaled by aabb+0x1C (height).
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float rx = dist(rng) * 2.0f - 1.0f;
    const float ry = dist(rng) * 2.0f - 1.0f;
    const float rz = dist(rng) * 2.0f - 1.0f;
    pos.x += rx * box.rawWidth;
    pos.y += ry * box.rawHeight;
    pos.z += rz * box.rawWidth;
    return pos.y;
}

std::uint32_t fnv1a(const std::string &s) {
    // sigma: h = 0x811C9DC5; do { h ^= byte; h *= 0x01000193; } while (next);
    // — the terminating NUL is NOT hashed (loop exits after loading it).
    std::uint32_t h = 0x811C9DC5u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 0x01000193u;
    }
    return h;
}

// ============================================================================
// CPS scheduler (FUN_001AE86C channel A)
// ============================================================================

static int defaultRoll(int lo, int hi) {
    if (hi <= lo) return lo;
    return std::rand() % ((hi - lo) + 1) + lo;
}

bool updateCpsChannel(CpsChannel &ch, std::int64_t nowNs, int minCps, int maxCps,
                      int (*roll)(int, int)) {
    if (roll == nullptr) roll = &defaultRoll;
    const int cps = std::max(1, ch.rolledCps);
    const std::int64_t intervalNs = 1'000'000'000LL / cps;
    if (ch.lastFireNs != 0 && nowNs - ch.lastFireNs < intervalNs) {
        ch.tick = false;
        return false;
    }
    ch.tick = true;
    ch.lastFireNs = nowNs;
    const int lo = std::max(1, minCps);
    const int hi = std::max(lo, maxCps);
    int v = roll(lo, hi);
    if (v < 1) v = 1;
    if (v > 20) v = 20; // sigma clamp [1, 20]
    ch.rolledCps = v;
    return true;
}

// ============================================================================
// Engine (FUN_001ABB30)
// ============================================================================

AuraEngine::AuraEngine(AuraSettings settings) : settings_(settings) {
    const int lo = std::max(1, settings_.minKillAuraCps);
    const int hi = std::max(lo, settings_.maxKillAuraCps);
    cps_.rolledCps = std::min(20, hi);
}

AuraEngine::TickResult AuraEngine::tick(const Vec3 &eyeIn, const std::string &selfName,
                                        const std::vector<ActorSnapshot> &actors,
                                        std::int64_t nowNs) {
    TickResult out;
    candidates_.clear();
    rotationActive_ = false;

    const bool rotationsOn = settings_.rotations; // targetLock not ported

    // ---- CPS scheduler (FUN_001AE86C channel A, killAura) -------------------
    const bool cpsTick =
        updateCpsChannel(cps_, nowNs, settings_.minKillAuraCps, settings_.maxKillAuraCps,
                         nullptr);

    // ---- per-actor scan (sigma order) ---------------------------------------
    struct Candidate {
        const ActorSnapshot *snap{};
        Vec3 feet; // pos with y-1.62 (sigma local_120 / local_198)
    };
    std::vector<Candidate> collected;

    for (const ActorSnapshot &a : actors) {
        if (a.actor == nullptr) continue;

        // Eye-relative Euclidean distance (sigma fVar48, raw eye already y-1.62).
        const float dx = eyeIn.x - a.pos.x;
        const float dy = eyeIn.y - a.pos.y;
        const float dz = eyeIn.z - a.pos.z;
        const float rawDist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (!a.namedPath) {
            // sigma interaction-check == false branch: name-tag entities.
            // Empty name tag -> mobAura-only branch (not ported).
            if (a.nameTag.empty()) continue;

            const std::uint32_t nameHash = fnv1a(a.nameTag);

            // FNV 0xE1BED951 (fireball): dist <= 10 && antiFireball -> attack.
            // antiFireball is outside this port's scope -> no attack.
            if (nameHash == 0xE1BED951u) continue;

            // FNV 0xF228491F (item entity): dist <= 6 && killItems -> attack
            // at the raw position (no y-1.62, no jitter, no desc writes,
            // sigma 0x1AC194), then optional swing.
            if (nameHash == 0xF228491Fu) {
                if (settings_.killItems && rawDist <= 6.0f) {
                    AttackRequest req;
                    req.actor = a.actor;
                    req.hitPos = a.pos; // raw position (sigma pfVar32)
                    req.range = settings_.killRange;
                    req.dz = dz;
                    req.writeDesc = false;
                                    out.attacks.push_back(req);
                    out.swingCount += 1;
                }
            }
            // other named-tag entities feed sigma's mobAura list (not ported)
            continue;
        }

        // ---- named path (killAura) ------------------------------------------
        if (rawDist > 500.0f) continue; // sigma hard cap (0x43FA0000)

        // Self-skip: memcmp of the embedded +0xB28 strings (sigma 0x1AC0F8).
        if (a.actorName == selfName) continue;

        if (settings_.antiBot) {
            // sigma: tag length < 2 -> skip.
            if (a.tagStr.size() < 2) continue;
            // sigma: degenerate AABB -> skip (comp+0x1C / comp+0x18 < 0.05).
            if (a.aabbHeight < 0.05f || a.aabbWidth < 0.05f) continue;
        }

        // sigma: botList memcmp skip (mutex-guarded vector of strings).
        if (settings_.antiBot) {
            bool isBot = false;
            for (const std::string &bot : settings_.botList) {
                if (bot == a.tagStr) {
                    isBot = true;
                    break;
                }
            }
            if (isBot) continue;
        }

        // sigma: friendList memcmp skip (same tag string fetch, mutex 0x4584C8).
        bool isFriend = false;
        for (const std::string &f : settings_.friendList) {
            if (f == a.tagStr) {
                isFriend = true;
                break;
            }
        }
        if (isFriend) continue;

        // sigma: unconditional AABB-width write point for accepted entities
        // (hitBox ? hitBoxValue : 0.6) — before the health gate.
        if (onEntityAccepted) onEntityAccepted(a);

        // sigma health gate: getComponent(actor, compId[0x457C38]) -> +0x7C.
        if (a.health == 0) continue;

        // sigma writes the AABB width for non-friend/non-bot entities
        // (hitBox ? hitBoxValue : 0.6). hitBox is not part of this port ->
        // 0.6, exactly like sigma with the flag off. Performed by the binding
        // layer right after snapshotting (see Hooks.cpp).

        // Collect radius: rotations|targetLock ? rotationsRange : killRange.
        const Vec3 feet{a.pos.x, a.pos.y - 1.62f, a.pos.z};
        const float collectRadius =
            rotationsOn ? settings_.rotationsRange : settings_.killRange;
        const AabbBox box{a.aabbWidth, a.aabbHeight};
        const float closest = closestDistanceAABB(eyeIn, feet, box);
#ifdef KAURA_TRACE
#ifdef KAURA_TRACE
        std::printf("[scan] closest=%.3f collect=%.3f\n", closest, collectRadius);
#endif
#endif
        if (!(closest <= collectRadius)) continue;
        if (!rotationsOn && !(closest <= settings_.killRange)) continue;

        collected.push_back(Candidate{&a, feet});
    }

    if (collected.empty()) return out;

    const bool emulate = settings_.emulateNativeAttack;
    const Candidate *nearest = nullptr;
    float best = 9999.0f;
    for (const Candidate &c : collected) {
        const AabbBox box{c.snap->aabbWidth, c.snap->aabbHeight};
        const float d = closestDistanceAABB(eyeIn, c.feet, box);
        if (d <= settings_.rotationsRange && d < best) {
            nearest = &c;
            best = d;
        }
    }

    // Rotations (sigma: FUN_001AEF80 on the nearest target).
    if (nearest != nullptr && rotationsOn) {
        const AabbBox box{nearest->snap->aabbWidth, nearest->snap->aabbHeight};
        rotation_ = computeRotations(eyeIn, nearest->feet, box);
        rotationActive_ = true;
    }

    auto makeRequest = [&](const Candidate &c) {
        AttackRequest req;
        req.actor = c.snap->actor;
        req.hitPos = c.feet;
        req.range = settings_.killRange;
        return req;
    };

    // ---- auraType dispatch (sigma DAT_00458030) -----------------------------
    if (nearest != nullptr && settings_.auraType == AuraType::Multi) {
        if (cpsTick) {
            // Multi: attack EVERY candidate with clamped dist <= killRange.
            for (const Candidate &c : collected) {
                const AabbBox box{c.snap->aabbWidth, c.snap->aabbHeight};
                const float d = closestDistanceAABB(eyeIn, c.feet, box);
                if (d > settings_.killRange) continue;
                if (settings_.hurtCheck && c.snap->hurtTime >= 2) continue;

                AttackRequest req = makeRequest(c);
                req.writeDesc = true;
                req.dz = axisDelta(eyeIn.z, c.feet.z - c.snap->aabbWidth * 0.5f,
                                   c.feet.z + c.snap->aabbWidth * 0.5f);
                if (!emulate) {
                    // sigma: jitter before the attack-desc writes + attack.
                    jitterHitPoint(rng_, req.hitPos, box);
                }
                out.attacks.push_back(req);
                out.swingCount += 1;
            }
        }
    } else if (nearest != nullptr && settings_.auraType == AuraType::Single) {
        const AabbBox box{nearest->snap->aabbWidth, nearest->snap->aabbHeight};
        const float closest = closestDistanceAABB(eyeIn, nearest->feet, box);
        if (closest <= settings_.killRange) {
            const bool hurtOk = !settings_.hurtCheck || nearest->snap->hurtTime < 2;
            if (hurtOk && cpsTick) {
                AttackRequest req = makeRequest(*nearest);
                req.writeDesc = true;
                req.dz = eyeIn.z; // sigma: *(lVar22+0x34) = pfVar41[2]
                if (!emulate) {
                    jitterHitPoint(rng_, req.hitPos, box);
                }
                out.attacks.push_back(req);
                out.swingCount += 1;
            }
        }
    }
    // auraType == Switch (2): sigma has no engine branch -> no-op (1:1).

    // ---- emulateNativeAttack tail (sigma DAT_004586C4) ----------------------
    // sigma sets the flag at the attack site and the tick tail calls
    // fn[0x457C10]+fn[0x457C30] on lp[0xCA8] within the SAME invocation.
    if (emulate && !out.attacks.empty()) {
        out.attacks.clear(); // no real GameMode::attack calls
        out.runEmulatePulse = true;
    }

    // Persist candidates for debugging/tests.
    for (const Candidate &c : collected) candidates_.push_back(*c.snap);
    return out;
}

} // namespace kaura
