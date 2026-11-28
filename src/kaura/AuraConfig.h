#pragma once

// Config persistence — keys mirror libReverseSigma's parser (FUN_001950F4)
// exactly so an existing Apollon config.json keeps working. Stored as JSON in
// the mod's config directory (LeviLauncher data dir).

#include "kaura/AuraTypes.h"

namespace kaura {

constexpr const char *kConfigFileName = "config.json";

// Applies a single key/value pair (string values from modmenu callbacks).
// Returns true when the key was recognized.
bool applyConfigKey(AuraSettings &s, const std::string &key,
                    const std::string &value);

// Serializes the full settings to JSON text.
std::string serializeConfig(const AuraSettings &s);

// Parses JSON text into settings (missing keys keep their current value).
bool deserializeConfig(const std::string &text, AuraSettings &s);

} // namespace kaura
