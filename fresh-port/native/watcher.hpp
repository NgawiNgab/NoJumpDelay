#pragma once
#include "hooks.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace aura {
class HookWatcher {
 std::atomic<bool> stopRequested_{false};
 std::mutex mutex_;
 std::condition_variable wake_;
 std::thread thread_;
public:
 HookWatcher() = default;
 HookWatcher(const HookWatcher&) = delete;
 HookWatcher& operator=(const HookWatcher&) = delete;
 ~HookWatcher();
 bool start(ConfigSnapshot);
 void stop();
};
}
