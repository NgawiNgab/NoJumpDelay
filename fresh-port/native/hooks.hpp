#pragma once
#include "engine.hpp"
namespace aura {
using ConfigSnapshot = Config (*)();
// Verifies all twelve prologues before installing either hook.
bool installHooks(ConfigSnapshot);
// Returns false if the preloader could not remove every installed hook.
bool removeHooks();
}
