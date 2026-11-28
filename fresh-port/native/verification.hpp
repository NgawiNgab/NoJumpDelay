#pragma once
#include <cstdint>
namespace aura {
// Returns the ELF load bias only if every executable 16-byte signature matches.
// Matching prologues alone does not establish class layouts or function ABIs.
std::uintptr_t verifyLoadedGame();
}
