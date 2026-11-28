#include "watcher.hpp"
#include "verification.hpp"
#include <android/log.h>
#include <chrono>

namespace aura {
HookWatcher::~HookWatcher() {stop();}
bool HookWatcher::start(ConfigSnapshot snapshot) {
 if(thread_.joinable() || !snapshot) return false;
 stopRequested_.store(false,std::memory_order_release);
 try {
  thread_=std::thread([this,snapshot] {
   __android_log_print(ANDROID_LOG_INFO,"ApollonKillAura","Waiting for libminecraftpe.so");
   try {
    while(!stopRequested_.load(std::memory_order_acquire)) {
     const auto result=verifyLoadedGame();
     if(result.status==VerificationStatus::Mismatch) {
      __android_log_print(ANDROID_LOG_ERROR,"ApollonKillAura","Game prologue mismatch at RVA 0x%zx; no hooks installed",static_cast<std::size_t>(result.mismatchedRva));
      break;
     }
     if(result.status==VerificationStatus::Matched) {
      __android_log_print(ANDROID_LOG_INFO,"ApollonKillAura","Verified game base %p",reinterpret_cast<void*>(result.base));
      if(!stopRequested_.load(std::memory_order_acquire) && !installHooks(snapshot))
       __android_log_print(ANDROID_LOG_ERROR,"ApollonKillAura","Hook installation failed");
      break;
     }
     std::unique_lock lock(mutex_);
     wake_.wait_for(lock,std::chrono::milliseconds(100),[this]{return stopRequested_.load(std::memory_order_acquire);});
    }
   } catch(...) {__android_log_print(ANDROID_LOG_ERROR,"ApollonKillAura","Hook watcher failed");}
  });
 } catch(...) {return false;}
 return true;
}
void HookWatcher::stop() {
 stopRequested_.store(true,std::memory_order_release);
 wake_.notify_all();
 if(thread_.joinable()) thread_.join();
}
}
