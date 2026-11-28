#pragma once
namespace aura {
enum class Verification { MissingLibrary, Mismatch, Matched };
// Compares code bytes only; a match does NOT establish ABI or safe hooks.
Verification verifyLoadedGame();
}
