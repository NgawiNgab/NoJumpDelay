#include "hooks.hpp"
#include "combat.hpp"
#include "verification.hpp"
#include "rotation.hpp"
#include <pl/memory/Hook.hpp>
#include <android/log.h>
#include <atomic>
#include <chrono>
#include <cstring>

namespace aura {
namespace {
using TickFn=void (*)(void*);
using RotationFn=void (*)(void*,void*);
void* originalTick{};
void* originalRotation{};
void* tickTarget{};
void* rotationTarget{};
bool tickInstalled{}, rotationInstalled{};
std::atomic<bool> active{};
ConfigSnapshot snapshot{};
GameApi api{};
Combat combat;
void error(const char* text) {__android_log_print(ANDROID_LOG_ERROR,"ApollonKillAura","%s",text);}
void tickDetour(void* player) {
 try {
  if(active.load(std::memory_order_acquire)) combat.tick(player,snapshot(),api,std::chrono::steady_clock::now());
 } catch(...) {error("Combat tick threw an exception");}
 if(originalTick) reinterpret_cast<TickFn>(originalTick)(player);
}
void rotationDetour(void* a0,void* actor) {
 try {
  if(active.load(std::memory_order_acquire) && actor) {
   const Config c=snapshot();
   const Rotation r=combat.rotation();
   if(actor==r.player && r.locked && (c.rotations||c.emulateNativeAttack)) {
    injectRotation(actor,r.pitch,r.yaw);
   }
  }
 } catch(...) {error("Rotation injector threw an exception");}
 if(originalRotation) reinterpret_cast<RotationFn>(originalRotation)(a0,actor);
}
}
bool installHooks(ConfigSnapshot getter) {
 if(tickInstalled||rotationInstalled||!getter) {error("Hooks already installed or missing config callback"); return false;}
 const auto verified=verifyLoadedGame();
 const auto base=verified.base;
 if(verified.status!=VerificationStatus::Matched || !base) {error("Game library missing or one of twelve prologues mismatched; no hooks installed"); return false;}
 snapshot=getter; api=GameApi::fromBase(base);
 tickTarget=reinterpret_cast<void*>(base+0x0af5e460);
 rotationTarget=reinterpret_cast<void*>(base+0x10383480);
 if(pl::memory::hook(tickTarget,reinterpret_cast<void*>(&tickDetour),&originalTick)!=0) {
  error("NormalTick hook failed"); return false;
 }
 tickInstalled=true;
 if(pl::memory::hook(rotationTarget,reinterpret_cast<void*>(&rotationDetour),&originalRotation)!=0) {
  error("Rotation hook failed; rolling back NormalTick");
  if(pl::memory::unhook(tickTarget,reinterpret_cast<void*>(&tickDetour))) tickInstalled=false;
  else error("NormalTick rollback failed");
  return false;
 }
 rotationInstalled=true;
 active.store(true,std::memory_order_release);
 __android_log_print(ANDROID_LOG_INFO,"ApollonKillAura","Installed tick and rotation hooks at game base %p",reinterpret_cast<void*>(base));
 return true;
}
bool removeHooks() {
 active.store(false,std::memory_order_release);
 bool ok=true;
 if(rotationInstalled) {
  if(pl::memory::unhook(rotationTarget,reinterpret_cast<void*>(&rotationDetour))) rotationInstalled=false;
  else {error("Could not remove rotation hook");ok=false;}
 }
 if(tickInstalled) {
  if(pl::memory::unhook(tickTarget,reinterpret_cast<void*>(&tickDetour))) tickInstalled=false;
  else {error("Could not remove NormalTick hook");ok=false;}
 }
 combat.reset();
 return ok;
}
} // namespace aura
