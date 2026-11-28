#pragma once

// Runtime module-base lookup (same /proc/self/maps approach the original
// libReverseSigma uses in FUN_001931B8) and in-memory prologue verification
// performed before every hook installation.

#include <cstddef>
#include <cstdint>

namespace kaura::re {

// Returns the load base of the module whose path contains `name`, or 0.
std::uintptr_t findModuleBase(const char *name);

// Verifies that the first `len` bytes at base+rva match `expected`.
// Returns true on match; on mismatch `outActual` (optional, 16 bytes max)
// receives the live bytes for logging.
bool verifyPrologue(std::uintptr_t base, std::uint32_t rva,
                    const unsigned char *expected, std::size_t len,
                    unsigned char *outActual = nullptr);

} // namespace kaura::re
