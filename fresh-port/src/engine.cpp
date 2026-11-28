#include "engine.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace aura {
namespace {
bool contains(const std::vector<std::string>& list, const std::string& value) {
    return std::find(list.begin(), list.end(), value) != list.end();
}
bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
}
float distanceToBox(Vec3 eye, Vec3 p, float width, float height) {
    if (!finite(eye) || !finite(p) || !std::isfinite(width) || !std::isfinite(height) || width < 0 || height < 0)
        return std::numeric_limits<float>::infinity();
    const float half = width * .5f;
    const float x = std::clamp(eye.x, p.x - half, p.x + half);
    const float y = std::clamp(eye.y, p.y - height, p.y);
    const float z = std::clamp(eye.z, p.z - half, p.z + half);
    return std::hypot(eye.x - x, eye.y - y, eye.z - z);
}
bool playerEligible(const Config& c, const Candidate& t) {
    if (!c.killAura || !t.player || t.self || !t.alive || (c.hurtCheck && t.hurtTime >= 2)) return false;
    if (contains(c.botList, t.name) || contains(c.friendList, t.name)) return false;
    if (c.antiBot && (t.name.size() < 2 || t.width < .05f || t.height < .05f)) return false;
    return true;
}
bool itemEligible(const Config& c, const Candidate& t, float rawDistance) {
    return c.killItems && t.item && !t.self && std::isfinite(rawDistance) && rawDistance >= 0 && rawDistance <= 6.0f;
}
std::vector<std::size_t> selectPlayers(const Config& c, Vec3 eye, std::span<const Candidate> targets, std::uint64_t previousSwitch) {
    std::vector<std::size_t> result;
    if (!std::isfinite(c.killRange) || c.killRange < 0) return result;
    float nearest = std::numeric_limits<float>::infinity();
    for (std::size_t i = 0; i < targets.size(); ++i) {
        const Candidate& t = targets[i];
        if (!playerEligible(c, t)) continue;
        const float d = distanceToBox(eye, t.origin, t.width, t.height);
        if (!(d <= c.killRange)) continue;
        if (c.auraType == Mode::Multi || c.auraType == Mode::Switch) result.push_back(i);
        else if (c.auraType == Mode::Single && d < nearest) { nearest = d; result.assign(1, i); }
    }
    if (c.auraType == Mode::Switch && !result.empty()) {
        std::size_t next=0;
        for (std::size_t i=0; i<result.size(); ++i)
            if (targets[result[i]].id == previousSwitch) {next=(i+1)%result.size();break;}
        const auto selected=result[next]; result.assign(1,selected);
    }
    return result;
}
int intervalMs(int minCps, int maxCps, std::mt19937& rng) {
    minCps = std::clamp(minCps, 1, 20);
    maxCps = std::clamp(maxCps, 1, 20);
    if (minCps > maxCps) std::swap(minCps, maxCps);
    std::uniform_int_distribution<int> dist(minCps, maxCps);
    return 1000 / dist(rng);
}
} // namespace aura
