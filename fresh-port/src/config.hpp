#pragma once
#include "engine.hpp"
#include <filesystem>
#include <string_view>
namespace aura {
// Load is transactional: malformed JSON leaves the caller's configuration unchanged.
bool loadConfig(const std::filesystem::path& path, Config& out);
bool saveConfig(const std::filesystem::path& path, const Config& config);
bool updateConfig(Config& config, std::string_view key, std::string_view value);
} // namespace aura
