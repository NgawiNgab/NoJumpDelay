#pragma once

// ApollonKillAura — native LeviLauncher mod (pl::Mod / PL_REGISTER_MOD).
// 1:1 port of the libReverseSigma KillAura + killItems + antiBot modules for
// Minecraft Bedrock 1.26.60 (arm64-v8a).
//
// No Dear ImGui, no JNI menu layer, no JNI_OnLoad export: all user surface is
// pl::modmenu modules + a floating SVG shortcut button (ButtonBuilder).

#include <string>

#include "kaura/AuraCore.h"
#include "kaura/AuraTypes.h"
#include "pl/Mod.hpp"

namespace kaura {

class KillAuraMod {
public:
    static KillAuraMod &instance();

    bool load(pl::mod::ModContext &context);
    bool enable(pl::mod::ModContext &context);
    bool disable(pl::mod::ModContext &context);
    bool unload(pl::mod::ModContext &context);

    [[nodiscard]] pl::mod::NativeMod &self() { return self_; }
    AuraSettings &settings() { return settings_; }

private:
    KillAuraMod() : self_(*pl::mod::NativeMod::current()) {}

    pl::mod::NativeMod &self_;
    AuraSettings settings_{};
    bool hooksInstalled_{false};
    bool menuRegistered_{false};

    bool loadConfigFile();
    bool saveConfigFile();
    void registerMenu();
    void unregisterMenu();
    void pushSettingsToEngine();
};

} // namespace kaura
