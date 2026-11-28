#include "watcher.hpp"
#include "verification.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
using namespace std::chrono_literals;
namespace {
std::atomic<int> state{0}, polls{0}, installs{0};
std::mutex mutex;
std::condition_variable cv;
aura::Config snapshot() {return {};}
bool waitFor(const std::atomic<int>& value,int goal) {
 std::unique_lock lock(mutex);
 return cv.wait_for(lock,2s,[&]{return value.load()>=goal;});
}
}
extern "C" int __android_log_print(int,const char*,const char*,...) {return 0;}
namespace aura {
VerificationResult verifyLoadedGame() {
 ++polls; cv.notify_all();
 switch(state.load()) {
 case 1: return {VerificationStatus::Matched,0x100000000,0};
 case 2: return {VerificationStatus::Mismatch,0,0x1234};
 default: return {};
 }
}
bool installHooks(ConfigSnapshot) {++installs;cv.notify_all();return true;}
}
int main() {
 aura::HookWatcher watcher;
 assert(watcher.start(snapshot));assert(!watcher.start(snapshot));assert(waitFor(polls,1));
 assert(installs==0);state=1;assert(waitFor(installs,1));watcher.stop();
 assert(installs==1);
 state=2;polls=0;assert(watcher.start(snapshot));assert(waitFor(polls,1));watcher.stop();assert(installs==1);
 state=0;polls=0;assert(watcher.start(snapshot));assert(waitFor(polls,1));watcher.stop();
 state=1;assert(installs==1);
 watcher.stop();
}
