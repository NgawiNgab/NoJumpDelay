#include "config.hpp"
#include "hooks.hpp"
#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <android/log.h>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace {
constexpr std::string_view moduleId="ApollonKillAura.killAura";
constexpr std::string_view auraButton="ApollonKillAura.shortcut";
constexpr std::string_view itemsButton="ApollonKillAura.itemsShortcut";
constexpr const char* icon=R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64"><circle cx="32" cy="32" r="25" fill="#222b38" stroke="#ee475c" stroke-width="4"/><path d="M32 8v17M32 39v17M8 32h17M39 32h17" stroke="#fff" stroke-width="4"/><circle cx="32" cy="32" r="7" fill="#ee475c"/></svg>)";
void log(const char* msg) { __android_log_print(ANDROID_LOG_WARN,"ApollonKillAura","%s",msg); }
class Mod {
 std::mutex mutex_;
 aura::Config config_;
 std::filesystem::path path_;
 std::string modId_;
 bool registered_ = false;
 void saveLocked() { if(!path_.empty() && !aura::saveConfig(path_,config_)) log("Cannot save config.json"); }
 void updateButtons() {
  aura::Config c; std::string id;
  {std::lock_guard lock(mutex_); c=config_; id=modId_;}
  pl::modmenu::unregisterButton(auraButton);
  pl::modmenu::unregisterButton(itemsButton);
  if(c.killAuraShortcut) {
   bool ok=pl::modmenu::ButtonBuilder(std::string(auraButton),"Kill Aura")
    .moduleId(std::string(moduleId)).modId(id).behavior(pl::modmenu::ButtonBehavior::Toggle).svgIcon(icon)
    .onEvent([](std::string_view,pl::modmenu::ButtonEvent e,float value){if(e==pl::modmenu::ButtonEvent::StateChanged) pl::modmenu::setModuleEnabled(moduleId,value>0.5f);}).registerButton();
   if(!ok) log("Could not register aura shortcut");
  }
  if(c.killItemsShortcut) {
   bool ok=pl::modmenu::ButtonBuilder(std::string(itemsButton),"Kill Items")
    .moduleId(std::string(moduleId)).modId(id).behavior(pl::modmenu::ButtonBehavior::Toggle).svgIcon(icon)
    .onEvent([this](std::string_view,pl::modmenu::ButtonEvent e,float value){if(e==pl::modmenu::ButtonEvent::StateChanged){std::lock_guard lock(mutex_); config_.killItems=value>0.5f; saveLocked();}}).registerButton();
   if(!ok) log("Could not register items shortcut");
  }
 }
public:
 static Mod& instance() {static Mod mod;return mod;}
 static aura::Config snapshot() { auto& m=instance(); std::lock_guard lock(m.mutex_); return m.config_; }
 bool load(pl::mod::ModContext& context) {
  std::lock_guard lock(mutex_); modId_=context.id(); path_=context.configDir()/"config.json";
  if(std::filesystem::exists(path_) && !aura::loadConfig(path_,config_)) log("Could not read config.json; using defaults without overwriting it");
  return true;
 }
 bool enable(pl::mod::ModContext&) {
  aura::Config c; std::string id;
  {std::lock_guard lock(mutex_); c=config_; id=modId_;}
  using T=pl::modmenu::ConfigType;
  auto builder=pl::modmenu::ModuleBuilder(std::string(moduleId),"Kill Aura");
  builder.modId(id).description("Native Kill Aura for Minecraft Bedrock 1.26.60.4.").defaultEnabled(c.killAura)
   .onToggle([this](std::string_view,bool enabled){std::lock_guard lock(mutex_); config_.killAura=enabled; saveLocked();})
   .onConfigChanged([this](std::string_view,std::string_view key,std::string_view value){bool refresh=false; {std::lock_guard lock(mutex_); if(!aura::updateConfig(config_,key,value)){log("Invalid configuration value");return;} saveLocked(); refresh=key=="killAuraShortcut"||key=="killItemsShortcut";} if(refresh)updateButtons();})
   .config("auraType","Aura type",T::Radio,c.auraType==aura::Mode::Single?"Single":c.auraType==aura::Mode::Multi?"Multi":"Switch","Single,Multi,Switch")
   .config("killRange","Kill range",T::SliderFloat,std::to_string(c.killRange),"1","8")
   .config("minKillAuraCPS","Minimum CPS",T::SliderInt,std::to_string(c.minKillAuraCPS),"1","20")
   .config("maxKillAuraCPS","Maximum CPS",T::SliderInt,std::to_string(c.maxKillAuraCPS),"1","20")
   .config("rotations","Rotations",T::Toggle,c.rotations?"true":"false")
   .config("rotationsRange","Rotations range",T::SliderFloat,std::to_string(c.rotationsRange),"1","8")
   .config("swing","Swing",T::Toggle,c.swing?"true":"false")
   .config("hurtCheck","Hurt check",T::Toggle,c.hurtCheck?"true":"false")
   .config("emulateNativeAttack","Emulate native attack",T::Toggle,c.emulateNativeAttack?"true":"false")
   .config("killItems","Kill nearby items",T::Toggle,c.killItems?"true":"false")
   .config("antiBot","Anti bot",T::Toggle,c.antiBot?"true":"false")
   .config("killAuraShortcut","Aura shortcut",T::Toggle,c.killAuraShortcut?"true":"false")
   .config("killItemsShortcut","Items shortcut",T::Toggle,c.killItemsShortcut?"true":"false")
   .config("friendList","Friend list (JSON array)",T::Text,nlohmann::json(c.friendList).dump())
   .config("botList","Bot list (JSON array)",T::Text,nlohmann::json(c.botList).dump());
  registered_=builder.registerModule(); if(!registered_){log("Could not register module menu");return false;}
  if(!aura::installHooks(&Mod::snapshot)) {
   log("Could not install combat hooks");
   pl::modmenu::unregisterModule(moduleId); registered_=false;
   return false;
  }
  updateButtons();
  return true;
 }
 bool disable(pl::mod::ModContext&) {
  const bool hooksRemoved=aura::removeHooks();
  pl::modmenu::unregisterButton(auraButton); pl::modmenu::unregisterButton(itemsButton);
  if(registered_){pl::modmenu::unregisterModule(moduleId);registered_=false;}
  return hooksRemoved;
 }
 bool unload(pl::mod::ModContext& context){return disable(context);}
};
}
PL_REGISTER_MOD(Mod,Mod::instance())
