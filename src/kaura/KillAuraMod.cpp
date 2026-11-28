#include "kaura/KillAuraMod.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <utility>

#include "kaura/AuraConfig.h"
#include "kaura/re/Hooks.h"
#include "kaura/re/Logger.h"
#include "pl/ModMenu.hpp"

namespace kaura {

namespace {
constexpr const char *kModId = "apollon_killaura";
constexpr const char *kModuleId = "apollon_killaura.main";
constexpr const char *kButtonId = "apollon_killaura.aura_btn";

// Minimal sword SVG used by the floating shortcut buttons.
constexpr const char *kSwordSvg =
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' width='24' "
    "height='24'><path fill='%23ffffff' d='M6.92 5 5 6.92l3.16 3.16-2.34 "
    "2.34 5.02 5.02 2.34-2.34L16.34 18 18.26 16.08 6.92 5zM19 5l-2.34 "
    "2.34-1.42-1.42-1.18 1.18 4.24 4.24 1.18-1.18-1.42-1.42L19 5z'/></svg>";

std::filesystem::path configFilePath(pl::mod::NativeMod &self) {
    return self.getConfigDir() / kConfigFileName;
}
} // namespace

KillAuraMod &KillAuraMod::instance() {
    static KillAuraMod inst;
    return inst;
}

bool KillAuraMod::loadConfigFile() {
    std::error_code ec;
    std::filesystem::create_directories(self_.getConfigDir(), ec);
    const auto path = configFilePath(self_);
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) {
        // First run: write the defaults.
        saveConfigFile();
        return true;
    }
    std::string text((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    if (!deserializeConfig(text, settings_)) {
        re::logWarn("config.json parse failed; keeping defaults");
        return false;
    }
    return true;
}

bool KillAuraMod::saveConfigFile() {
    std::error_code ec;
    std::filesystem::create_directories(self_.getConfigDir(), ec);
    std::ofstream out(configFilePath(self_), std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    out << serializeConfig(settings_);
    return true;
}

void KillAuraMod::pushSettingsToEngine() { re::HookManager::instance().updateSettings(settings_); }

void KillAuraMod::registerMenu() {
    if (menuRegistered_) return;
    using namespace pl::modmenu;

    ModuleBuilder b(kModuleId, "Apollon KillAura");
    b.description("KillAura / killItems / antiBot ported 1:1 from Apollon "
                  "(libReverseSigma) for Minecraft 1.26.60")
        .modId(kModId)
        .defaultEnabled(true)
        .onConfigChanged(
            [this](std::string_view, std::string_view key, std::string_view value) {
                applyConfigKey(settings_, std::string(key), std::string(value));
                pushSettingsToEngine();
                saveConfigFile();
            })
        .onToggle([this](std::string_view, bool enabled) {
            settings_.killAura = enabled;
            pushSettingsToEngine();
            saveConfigFile();
        })
        // Core controls — keys mirror sigma's config parser.
        .config("killAura", "Kill Aura", ConfigType::Toggle, "0")
        .config("auraType", "Aura Type", ConfigType::Radio, "0", "Single,Switch,Multi",
                "killAura")
        .config("killRange", "Kill Range", ConfigType::SliderFloat, "3.0", "1.0", "8.0",
                "killAura")
        .config("minKillAuraCPS", "Min CPS", ConfigType::SliderInt, "8", "1", "30",
                "killAura")
        .config("maxKillAuraCPS", "Max CPS", ConfigType::SliderInt, "12", "1", "30",
                "killAura")
        .config("rotations", "Rotations", ConfigType::Toggle, "1", "", "", "killAura")
        .config("rotationsRange", "Rotations Range", ConfigType::SliderFloat, "3.0",
                "0.0", "180.0", "rotations")
        .config("swing", "Swing", ConfigType::Toggle, "1", "", "", "killAura")
        .config("hurtCheck", "Hurt Check", ConfigType::Toggle, "1", "", "", "killAura")
        .config("emulateNativeAttack", "Emulate Native Attack", ConfigType::Toggle, "0",
                "", "", "killAura")
        // Sub-modules.
        .config("killItems", "Kill Near Items", ConfigType::Toggle, "0")
        .config("antiBot", "Anti Bot", ConfigType::Toggle, "0")
        .config("friendList", "Friend List", ConfigType::Text, "")
        .config("botList", "Bot List", ConfigType::Text, "", "", "", "antiBot")
        .config("killAuraShortcut", "Kill Aura Shortcut Button", ConfigType::Toggle, "1")
        .config("killItemsShortcut", "Kill Items Shortcut Button", ConfigType::Toggle,
                "1");

    menuRegistered_ = b.registerModule();

    // Floating SVG shortcut button (replaces sigma's nativeKeyHandler hook).
    ButtonBuilder btn(kButtonId, "Kill Aura Shortcut");
    btn.moduleId(kModuleId)
        .modId(kModId)
        .label("Aura")
        .behavior(ButtonBehavior::Toggle)
        .svgIcon(kSwordSvg)
        .onEvent([this](std::string_view, ButtonEvent event, float value) {
            if (event == ButtonEvent::StateChanged) {
                settings_.killAura = value != 0.0f;
                pushSettingsToEngine();
                saveConfigFile();
            }
        });
    if (!btn.registerButton()) {
        re::logError("failed to register the floating shortcut button");
    }
}

void KillAuraMod::unregisterMenu() {
    if (!menuRegistered_) return;
    pl::modmenu::unregisterModule(kModuleId);
    pl::modmenu::unregisterButton(kButtonId);
    menuRegistered_ = false;
}

bool KillAuraMod::load(pl::mod::ModContext &context) {
    self_.getLogger().info("ApollonKillAura loading (1.26.60 port of libReverseSigma)");

    std::error_code ec;
    std::filesystem::create_directories(self_.getConfigDir(), ec);
    std::filesystem::create_directories(self_.getDataDir(), ec);
    loadConfigFile();
    return true;
}

bool KillAuraMod::enable(pl::mod::ModContext &context) {
    pushSettingsToEngine();
    re::HookManager::instance().install(settings_);
    hooksInstalled_ = true;
    registerMenu();
    self_.getLogger().info("ApollonKillAura enabled");
    return true;
}

bool KillAuraMod::disable(pl::mod::ModContext &context) {
    unregisterMenu();
    if (hooksInstalled_) {
        re::HookManager::instance().uninstall();
        hooksInstalled_ = false;
    }
    self_.getLogger().info("ApollonKillAura disabled");
    return true;
}

bool KillAuraMod::unload(pl::mod::ModContext &context) {
    saveConfigFile();
    self_.getLogger().info("ApollonKillAura unloaded");
    return true;
}

} // namespace kaura
