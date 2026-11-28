#pragma once

// ============================================================================
// Shared value types + configuration state for ApollonKillAura.
// Config keys and defaults mirror libReverseSigma.so's parser (FUN_001950F4).
// ============================================================================

#include <string>
#include <vector>

namespace kaura {

struct Vec3 {
    float x{}, y{}, z{};
};

struct Vec2 {
    float x{}, y{};
};

// Aura selection mode — sigma only implements 0 (Single) and 1 (Multi);
// value 2 ("Switch") has no engine branch in the original binary either.
enum class AuraType { Single = 0, Multi = 1, Switch = 2 };

struct AuraSettings {
    bool killAura{false};
    AuraType auraType{AuraType::Single};
    float killRange{3.0f};
    int minKillAuraCps{8};
    int maxKillAuraCps{12};
    bool rotations{true};
    float rotationsRange{3.0f};
    bool swing{true};
    bool hurtCheck{true};
    bool emulateNativeAttack{false};
    bool killItems{false}; // "Kill Near Items"
    bool antiBot{false};
    std::vector<std::string> friendList;
    std::vector<std::string> botList;
    bool killAuraShortcut{true};
    bool killItemsShortcut{true};
};

} // namespace kaura
