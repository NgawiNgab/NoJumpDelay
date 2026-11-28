#pragma once

// Detour implementations + hook installation with in-memory prologue
// verification. 1:1 port of sigma's hook layout:
//   - NormalTick detour  (sigma detour 0x0ABB30, target 0x0AF5E460,
//     orig slot 0x035AEB0): runs the KillAura engine, then calls the
//     original tick (sigma calls the trampoline last).
//   - Rotation injector  (sigma detour 0x0AB7D4, target 0x10383480,
//     orig slot 0x035AEA8): writes the engine-computed yaw/pitch into
//     actor+0x238, then calls the original.

#include "kaura/AuraCore.h"
#include "kaura/AuraTypes.h"

namespace kaura::re {

class HookManager {
public:
    static HookManager &instance();

    // Resolves the game API, verifies prologues and installs both hooks.
    // Returns false when the game does not match 1.26.60 (fail-safe: nothing
    // is hooked).
    bool install(const AuraSettings &settings);
    void uninstall();

    void updateSettings(const AuraSettings &settings) { engine_.setSettings(settings); }
    AuraEngine &engine() { return engine_; }
    void *origNormalTick() const { return origNormalTick_; }
    void *origRotationApply() const { return origRotationApply_; }
    static std::int64_t nowSteadyNs();

private:
    HookManager() : engine_(AuraSettings{}) {}

    AuraEngine engine_;
    void *origNormalTick_{nullptr};
    void *origRotationApply_{nullptr};
    bool installed_{false};
};

} // namespace kaura::re
