#include "hooks.hpp"
#include "verification.hpp"
#include <pl/memory/Hook.hpp>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <vector>

namespace {
std::uintptr_t fakeBase{};
int hookCalls{},unhookCalls{},failHookAt{},ticks{},rotations{};
void* tickDetour{};
void* rotationDetour{};
std::vector<void*> removed;
void originalTick(void*) {++ticks;}
void originalRotation(void*,void*) {++rotations;}
aura::Config snapshot() {return {};}
}
extern "C" int __android_log_print(int,const char*,const char*,...) {return 0;}
namespace aura {VerificationResult verifyLoadedGame() {return fakeBase ? VerificationResult{VerificationStatus::Matched,fakeBase,0} : VerificationResult{};}}
namespace pl::memory {
int hook(FuncPtr target, FuncPtr detour, FuncPtr* orig, HookPriority) {
 ++hookCalls;
 if(hookCalls==failHookAt) return -1;
 if(reinterpret_cast<std::uintptr_t>(target)==fakeBase+0x0af5e460) {
  tickDetour=detour; *orig=reinterpret_cast<void*>(&originalTick);
 } else {
  assert(reinterpret_cast<std::uintptr_t>(target)==fakeBase+0x10383480);
  rotationDetour=detour; *orig=reinterpret_cast<void*>(&originalRotation);
 }
 return 0;
}
bool unhook(FuncPtr target, FuncPtr detour) {
 ++unhookCalls; removed.push_back(target);
 assert(detour==tickDetour || detour==rotationDetour); return true;
}
}
int main() {
 assert(!aura::installHooks(snapshot));assert(hookCalls==0);
 fakeBase=0x100000000;
 failHookAt=2;assert(!aura::installHooks(snapshot));
 assert(hookCalls==2 && unhookCalls==1 && removed[0]==reinterpret_cast<void*>(fakeBase+0x0af5e460));
 hookCalls=0;failHookAt=0;assert(aura::installHooks(snapshot));
 assert(!aura::installHooks(snapshot));
 reinterpret_cast<void(*)(void*)>(tickDetour)(nullptr);
 reinterpret_cast<void(*)(void*,void*)>(rotationDetour)(nullptr,nullptr);
 assert(ticks==1 && rotations==1);
 assert(aura::removeHooks()); assert(unhookCalls==3);
 assert(removed[1]==reinterpret_cast<void*>(fakeBase+0x10383480));
 assert(removed[2]==reinterpret_cast<void*>(fakeBase+0x0af5e460));
 assert(aura::removeHooks());assert(unhookCalls==3);
 hookCalls=0; assert(aura::installHooks(snapshot));assert(aura::removeHooks());
}
