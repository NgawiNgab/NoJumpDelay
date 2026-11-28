#pragma once
// Independently written, host-testable model. Not a complete Sigma port.
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace aura {
struct Vec3 { float x{}, y{}, z{}; };
enum class Mode : int { Single = 0, Multi = 1, Switch = 2 };
struct Config {
    bool killAura = false, killItems = false, antiBot = false;
    bool killAuraShortcut = false, killItemsShortcut = false;
    bool rotations = false, swing = true, hurtCheck = false, emulateNativeAttack = false;
    Mode auraType = Mode::Single;
    float killRange = 4.0f, rotationsRange = 4.0f;
    int minKillAuraCPS = 8, maxKillAuraCPS = 12;
    std::vector<std::string> friendList, botList;
};
struct Candidate {
    std::uint64_t id{};
    std::string name;
    Vec3 origin{};
    float width{}, height{};
    int hurtTime{};
    bool player{}, item{}, self{}, alive{true};
};

// Position has already been adjusted by the caller as required by the path.
float distanceToBox(Vec3 eye, Vec3 upperCenter, float width, float height);
bool playerEligible(const Config&, const Candidate&);
bool itemEligible(const Config&, const Candidate&, float rawDistance);
std::vector<std::size_t> selectPlayers(const Config&, Vec3 eye, std::span<const Candidate>, std::uint64_t previousSwitch = 0);
int intervalMs(int minCps, int maxCps, std::mt19937& rng);
} // namespace aura
