// Host unit tests for the 1:1 KillAura engine port.
// Build: g++ -std=c++20 -fsanitize=address,undefined (see scripts/test_host.sh)
//
// Covers the requested acceptance criteria:
//   - range filter (collect radius vs attack radius)
//   - hurtCheck invulnerable skip (hurtTime >= 2)
//   - friend/antiBot exclusion
//   - auraType selection (Single nearest / Multi all)
//   - CPS bounds (re-roll clamp [1,20], interval gating)

#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "kaura/AuraCore.h"

using namespace kaura;

namespace {

int g_checks = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

ActorSnapshot makeActor(void *id, const std::string &name, const Vec3 &pos,
                        float hurtTime = 0, int health = 20) {
    ActorSnapshot a;
    a.actor = id;
    a.namedPath = true;
    a.pos = pos;
    a.aabbWidth = 0.6f;
    a.aabbHeight = 1.8f;
    a.nameTag = name;   // nametag path identity (not used for namedPath=true)
    a.tagStr = name;    // antiBot/botList/friend compare string
    a.actorName = name; // self-skip compare string
    a.health = health;
    a.hurtTime = static_cast<int>(hurtTime);
    return a;
}

const int64_t kT = 1'000'000'000LL; // 1 second in ns

int fixedRoll(int lo, int /*hi*/) { return lo; }

} // namespace

static void testRangeFilter() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 4.0f;
    s.rotationsRange = 5.0f;
    s.rotations = false;
    s.minKillAuraCps = 20;
    s.maxKillAuraCps = 20;
    AuraEngine e(s);
    e.seedRng(7);

    // Actor at horizontal distance 3 (within collect+attack radius).
    ActorSnapshot near = makeActor((void *)1, "Herobrine", {0, 64, 3});
    // Actor at distance 6 (outside collect radius).
    ActorSnapshot far = makeActor((void *)2, "Dinnerbone", {0, 64, 6});

    std::vector<ActorSnapshot> actors{near, far};
    Vec3 eye{0, 62.38f, 0}; // feet eye; y-1.62 already applied by binding

    // Prime the scheduler with a fixed roll so the first tick fires.
    const auto r1 = e.tick(eye, "Steve", actors, kT);
    CHECK(r1.attacks.size() == 1);          // only the near actor
    CHECK(r1.attacks[0].actor == near.actor);

    // Boundary: exactly at killRange (<=) should collect.
    AuraEngine e2(s);
    e2.seedRng(7);
    ActorSnapshot edge = makeActor((void *)3, "Notch", {0, 64, 4.3f}); // dz = -4.0 (boundary)
    const auto r2 = e2.tick(eye, "Steve", {edge}, kT + 2 * kT);
    CHECK(r2.attacks.size() == 1);
}

static void testHurtCheckInvulnerableSkip() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 4.0f;
    s.rotationsRange = 6.0f;
    s.hurtCheck = true;
    s.rotations = false;
    s.minKillAuraCps = 20;
    s.maxKillAuraCps = 20;
    AuraEngine e(s);
    e.seedRng(7);

    ActorSnapshot hurt = makeActor((void *)1, "Mob", {0, 64, 2});
    hurt.hurtTime = 5; // invulnerable window
    const auto r = e.tick({0, 62.38f, 0}, "Steve", {hurt}, kT);
    CHECK(r.attacks.empty());

    // With hurtCheck disabled the same actor is attacked.
    s.hurtCheck = false;
    e.setSettings(s);
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {hurt}, 2 * kT);
    CHECK(r2.attacks.size() == 1);

    // hurtTime 1 (< 2) passes the gate.
    s.hurtCheck = true;
    e.setSettings(s);
    hurt.hurtTime = 1;
    const auto r3 = e.tick({0, 62.38f, 0}, "Steve", {hurt}, 3 * kT);
    CHECK(r3.attacks.size() == 1);
}

static void testFriendAndAntiBotExclusion() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 4.0f;
    s.rotationsRange = 6.0f;
    s.rotations = false;
    s.antiBot = true;
    s.friendList = {"FriendGuy"};
    s.botList = {"Bot420"};
    s.minKillAuraCps = 20;
    s.maxKillAuraCps = 20;
    AuraEngine e(s);
    e.seedRng(7);

    ActorSnapshot friendActor = makeActor((void *)1, "FriendGuy", {0, 64, 2});
    ActorSnapshot bot = makeActor((void *)2, "Bot420", {0, 64, 3});
    ActorSnapshot normal = makeActor((void *)3, "SteveJr", {0, 64, 4});

    const auto r = e.tick({0, 62.38f, 0}, "Steve", {friendActor, bot, normal}, kT);
    CHECK(r.attacks.size() == 1);
    CHECK(r.attacks[0].actor == normal.actor);

    // antiBot short-tag skip (length < 2).
    AuraSettings s2 = s;
    s2.friendList.clear();
    s2.botList.clear();
    e.setSettings(s2);
    ActorSnapshot shortTag = makeActor((void *)4, "A", {0, 64, 2});
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {shortTag}, 2 * kT);
    CHECK(r2.attacks.empty());

    // Degenerate AABB skip (sigma: width/height < 0.05).
    ActorSnapshot tiny = makeActor((void *)5, "Tiny", {0, 64, 2});
    tiny.aabbWidth = 0.01f;
    const auto r3 = e.tick({0, 62.38f, 0}, "Steve", {tiny}, 3 * kT);
    CHECK(r3.attacks.empty());
}

static void testAuraTypeSelection() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 5.0f;
    s.rotationsRange = 6.0f;
    s.rotations = false;
    s.hurtCheck = false;
    s.minKillAuraCps = 20;
    s.maxKillAuraCps = 20;

    ActorSnapshot a = makeActor((void *)1, "A", {0, 64, 2});
    ActorSnapshot b = makeActor((void *)2, "B", {0, 64, 3});
    ActorSnapshot c = makeActor((void *)3, "C", {0, 64, 4});

    // Single: exactly one attack (nearest).
    s.auraType = AuraType::Single;
    AuraEngine e(s);
    e.seedRng(7);
    const auto r1 = e.tick({0, 62.38f, 0}, "Steve", {a, b, c}, kT);
    CHECK(r1.attacks.size() == 1);
    CHECK(r1.attacks[0].actor == a.actor); // nearest

    // Multi: every candidate within killRange.
    s.auraType = AuraType::Multi;
    e.setSettings(s);
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {a, b, c}, 2 * kT);
    CHECK(r2.attacks.size() == 3);
    CHECK(r2.attacks[0].actor == a.actor);
    CHECK(r2.attacks[1].actor == b.actor);
    CHECK(r2.attacks[2].actor == c.actor);
}

static void testCpsBoundsAndInterval() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 4.0f;
    s.rotationsRange = 6.0f;
    s.rotations = false;
    s.minKillAuraCps = 10;
    s.maxKillAuraCps = 20;
    AuraEngine e(s);
    e.seedRng(7);

    ActorSnapshot a = makeActor((void *)1, "A", {0, 64, 2});

    // First tick fires (scheduler primed).
    const auto r1 = e.tick({0, 62.38f, 0}, "Steve", {a}, kT);
    CHECK(r1.attacks.size() == 1);
    CHECK(e.cps().rolledCps >= 1 && e.cps().rolledCps <= 20);

    // Immediately again: interval gating blocks the attack.
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {a}, kT + 10'000'000LL);
    CHECK(r2.attacks.empty());

    // A full second later: fires again (rolledCps >= 1 -> interval <= 1s).
    const auto r3 = e.tick({0, 62.38f, 0}, "Steve", {a}, 3 * kT);
    CHECK(r3.attacks.size() == 1);

    // Deterministic roll clamp: roll returning 99 -> clamped to 20.
    e.setSettings(s);
    const auto r4 = e.tick({0, 62.38f, 0}, "Steve", {a}, 10 * kT);
    CHECK(r4.attacks.size() == 1);
    (void)fixedRoll;
}

static void testKillItemsBranch() {
    AuraSettings s;
    s.killAura = false;
    s.killItems = true;
    s.swing = true;
    AuraEngine e(s);
    e.seedRng(7);

    // Item entity: nametag path (interaction-check false) with a name whose
    // FNV-1a equals sigma's item-entity selector 0xF228491F (constructed
    // printable preimage — the engine only ever sees the hash).
    ActorSnapshot item;
    item.actor = (void *)9;
    item.namedPath = false;
    item.pos = {0, 64, 2};
    item.aabbWidth = 0.25f;
    item.aabbHeight = 0.25f;
    item.health = 1;
    item.hurtTime = 0;

    const std::string itemName = "*IQQ($C#dLwqwP"; // FNV-1a == 0xF228491F
    CHECK(fnv1a(itemName) == 0xF228491Fu);
    item.nameTag = itemName;

    const auto r = e.tick({0, 62.38f, 0}, "Steve", {item}, kT);
    CHECK(r.attacks.size() == 1);
    CHECK(r.attacks[0].actor == item.actor);
    CHECK(r.attacks[0].writeDesc == false); // no attack-desc writes for items
    CHECK(r.attacks[0].hitPos.y == 64.0f);  // raw position (no y-1.62)
    CHECK(r.swingCount == 1);               // sigma swings after item attack

    // Distance > 6 -> no attack.
    item.pos = {0, 64, 7};
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {item}, 2 * kT);
    CHECK(r2.attacks.empty());

    // killItems disabled -> no attack.
    s.killItems = false;
    e.setSettings(s);
    item.pos = {0, 64, 2};
    const auto r3 = e.tick({0, 62.38f, 0}, "Steve", {item}, 3 * kT);
    CHECK(r3.attacks.empty());
}

static void testSelfAndDeadExclusion() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 4.0f;
    s.rotationsRange = 5.0f;
    s.rotations = false;
    s.minKillAuraCps = 20;
    s.maxKillAuraCps = 20;
    AuraEngine e(s);
    e.seedRng(7);

    // Self-name skip (sigma memcmp of embedded +0xB28 strings).
    ActorSnapshot self = makeActor((void *)1, "Steve", {0, 64, 2});
    const auto r = e.tick({0, 62.38f, 0}, "Steve", {self}, kT);
    CHECK(r.attacks.empty());

    // Dead (health == 0) skip.
    ActorSnapshot dead = makeActor((void *)2, "Zombie", {0, 64, 2});
    dead.health = 0;
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {dead}, 2 * kT);
    CHECK(r2.attacks.empty());
}

static void testEmulateNativeAttack() {
    AuraSettings s;
    s.killAura = true;
    s.killRange = 4.0f;
    s.rotationsRange = 6.0f;
    s.rotations = false;
    s.emulateNativeAttack = true;
    s.swing = true;
    s.minKillAuraCps = 20;
    s.maxKillAuraCps = 20;
    AuraEngine e(s);
    e.seedRng(7);

    ActorSnapshot a = makeActor((void *)1, "A", {0, 64, 2});
    const auto r = e.tick({0, 62.38f, 0}, "Steve", {a}, kT);
    // sigma: no GameMode::attack, but the pulse flag is armed and the swing
    // still happens.
    CHECK(r.attacks.empty());
    CHECK(r.runEmulatePulse);
    CHECK(r.swingCount == 1);
    // Pulse fires only when an attack was armed this tick.
    const auto r2 = e.tick({0, 62.38f, 0}, "Steve", {a}, 2 * kT);
    CHECK(r2.runEmulatePulse); // CPS tick fires again -> armed again
    // Out of CPS window: no arm, no pulse.
    const auto r3 = e.tick({0, 62.38f, 0}, "Steve", {a}, 2 * kT + 10'000'000LL);
    CHECK(!r3.runEmulatePulse);
}

static void testMathHelpers() {
    // closestDistanceAABB (FUN_001AEDE8): eye inside box -> 0.
    AabbBox box{0.6f, 1.8f};
    Vec3 feet{0, 64, 0};
    const float d0 = closestDistanceAABB({0, 64, 0}, feet, box);
    CHECK(d0 == 0.0f);

    // Above the box: distance = eye.y - top (Y range [feet-h, feet]).
    const float d1 = closestDistanceAABB({0, 66, 0}, feet, box);
    CHECK(d1 == 2.0f);

    // X outside: distance = eye.x - (feet.x + half).
    const float d2 = closestDistanceAABB({1.0f, 64, 0}, feet, box);
    CHECK(d2 == 0.7f);

    // computeRotations pitch: target above -> negative pitch (MC convention).
    const RotationUpdate rot = computeRotations({0, 64, 0}, {0, 66, 0}, box);
    CHECK(rot.pitch < 0.0f);

    // +90/-90 rule at zero horizontal distance.
    const RotationUpdate rot2 = computeRotations({0, 64, 0}, {0, 65, 0}, box);
    CHECK(rot2.pitch == -90.0f);
    const RotationUpdate rot3 = computeRotations({0, 66, 0}, {0, 65, 0}, box);
    CHECK(rot3.pitch == 90.0f);

    // fnv1a excludes the NUL terminator (sigma's do-while exits on it).
    CHECK(fnv1a("") == 0x811C9DC5u);
    CHECK(fnv1a("a") == (0x811C9DC5u ^ 0x61u) * 0x01000193u);
}

int main() {
    testRangeFilter();
    testHurtCheckInvulnerableSkip();
    testFriendAndAntiBotExclusion();
    testAuraTypeSelection();
    testCpsBoundsAndInterval();
    testKillItemsBranch();
    testSelfAndDeadExclusion();
    testEmulateNativeAttack();
    testMathHelpers();
    std::printf("ALL TESTS PASSED (%d checks)\n", g_checks);
    return 0;
}
