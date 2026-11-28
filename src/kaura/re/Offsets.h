#pragma once

// ============================================================================
// ApollonKillAura — reverse-engineered offsets for Minecraft Bedrock 1.26.60
// (arm64-v8a).
//
// Every RVA below was extracted by emulating libReverseSigma.so's obfuscated
// JNI_OnLoad decoder (self-modifying VM blocks) under Unicorn ARM64 and
// validating each decoded value against libminecraftpe.so 1.26.60
// (prologue disassembly + semantic cross-checks). See
// research/KILLAURA_1_26_60_RE.md for the full method and tables.
//
// NOTE: these are build-specific. The resolver verifies the expected prologue
// bytes in memory before any hook is installed (fail-safe if Mojang ships an
// update).
// ============================================================================

#include <cstdint>

namespace kaura::re {

// ---- hooked functions (detour targets) ------------------------------------
// sigma detour 0x0ABB30 (KillAura engine, orig slot sigma+0x35AEB0)
constexpr std::uint32_t RVA_NORMAL_TICK = 0x0AF5E460u;
// sigma detour 0x0AB7D4 (rotations injector, orig slot sigma+0x35AEA8)
constexpr std::uint32_t RVA_ROTATION_APPLY = 0x10383480u;

// ---- called functions (decoded from sigma's fn-pointer table) --------------
constexpr std::uint32_t RVA_ACTOR_LIST_FETCH = 0x10B16414u; // ldr x0,[x0,#0x478]
constexpr std::uint32_t RVA_ACTOR_TYPE_ID = 0x10B13684u;
constexpr std::uint32_t RVA_GAMEMODE_ATTACK = 0x1009CBA4u;
constexpr std::uint32_t RVA_GET_COMPONENT = 0x0FDABBFCu;
constexpr std::uint32_t RVA_INTERACTION_CHECK = 0x0FD8E120u;
constexpr std::uint32_t RVA_GET_NAME_COMPONENT = 0x0FD881D4u;
constexpr std::uint32_t RVA_ACTOR_CLASSIFIER = 0x101C2D7Cu;
constexpr std::uint32_t RVA_SWING = 0x0AF60D78u;
constexpr std::uint32_t RVA_ENA_A = 0x09B6BBD4u;
constexpr std::uint32_t RVA_ENA_B = 0x09B94C98u;

// ---- struct member offsets -------------------------------------------------
constexpr std::uint64_t OFF_LP_LEVEL = 0x1F0;        // LocalPlayer -> Level*
constexpr std::uint64_t OFF_LP_GAMEMODE = 0xA08;     // LocalPlayer -> GameMode*
constexpr std::uint64_t OFF_LP_INTERACTION = 0xCA8;  // interaction component
constexpr std::uint64_t OFF_LP_STATEVECTOR = 0x228;  // Vec3 position
constexpr std::uint64_t OFF_ACTOR_POS = 0x228;       // Vec3 position
constexpr std::uint64_t OFF_ACTOR_AABB = 0x230;      // AABB component ptr (actor[0x46])
constexpr std::uint64_t OFF_ACTOR_NAME = 0xB28;      // embedded libc++ string (actor[0x165])
constexpr std::uint64_t OFF_ACTOR_HURT_TIME = 0x1B4; // int
constexpr std::uint64_t OFF_ACTOR_ROTATION = 0x238;  // Vec2 {yaw, pitch}
constexpr std::uint64_t OFF_ACTOR_UNKNOWN_48C = 0x48C;
constexpr std::uint64_t OFF_LEVEL_ACTOR_SUBSYS = 0x1D8; // level[0x3B] attack-desc owner
constexpr std::uint64_t OFF_LEVEL_ACTORMGR = 0x478;     // Level::mActorManager
constexpr std::uint64_t OFF_AABB_WIDTH = 0x18;          // float (also antiBot check)
constexpr std::uint64_t OFF_AABB_HEIGHT = 0x1C;         // float (also antiBot check)
constexpr std::uint64_t OFF_COMPONENT_HEALTH = 0x7C;    // float inside health comp
constexpr std::uint64_t OFF_NAMECOMP_STRING = 0x48;     // std::string inside name comp

// libc++ std::string layout (SSO): byte0 & 1 ? long {cap,size,data} : short {size<<1, data}
constexpr std::uint64_t STR_LONG_SIZE = 0x8;
constexpr std::uint64_t STR_LONG_DATA = 0x10;
constexpr std::uint64_t STR_SHORT_DATA = 0x1;

// ---- expected in-memory prologues (first 16 bytes, 1.26.60) ----------------
extern const unsigned char PROLOGUE_NORMAL_TICK[16];
extern const unsigned char PROLOGUE_ROTATION_APPLY[16];
extern const unsigned char PROLOGUE_ACTOR_LIST[16];
extern const unsigned char PROLOGUE_GAMEMODE_ATTACK[16];
extern const unsigned char PROLOGUE_SWING[16];
extern const unsigned char PROLOGUE_GET_COMPONENT[16];
extern const unsigned char PROLOGUE_GET_NAME[16];

} // namespace kaura::re
