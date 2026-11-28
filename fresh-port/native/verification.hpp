#pragma once
#include <cstdint>
namespace aura {
enum class VerificationStatus { MissingLibrary, Mismatch, Matched };
struct VerificationResult {
 VerificationStatus status{VerificationStatus::MissingLibrary};
 std::uintptr_t base{};
 std::uintptr_t mismatchedRva{};
};
// The base is populated only when all executable 16-byte prologues match.
// Byte matches do not prove ABI compatibility or safe object layouts.
VerificationResult verifyLoadedGame();
}
