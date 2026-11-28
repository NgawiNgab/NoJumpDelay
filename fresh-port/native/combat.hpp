#pragma once
#include "engine.hpp"
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <vector>

namespace aura {
struct GameApi {
 std::vector<void*> (*actors)(void*);
 void* (*component)(void*, const void*);
 bool (*isPlayer)(void*);
 void* (*nameComponent)(void*);
 void (*attack)(void*, void*, bool, const Vec3*);
 void (*swing)(void*, int, int);
 void (*nativeBegin)(void*);
 void (*nativeEnd)(void*);
 const void* aliveKey;
 static GameApi fromBase(std::uintptr_t);
};
struct Rotation { void* player{}; bool locked{}; float yaw{}, pitch{}; };
class Combat {
 std::mutex tickMutex_, rotationMutex_;
 Rotation rotation_{};
 std::chrono::steady_clock::time_point nextAttack_{};
 std::mt19937 random_{std::random_device{}()};
 void setRotation(Rotation);
public:
 void tick(void*, const Config&, const GameApi&, std::chrono::steady_clock::time_point now);
 Rotation rotation();
 void reset();
};
} // namespace aura
