#pragma once
#include <cstdint>

namespace nojump {
// Recovered helper's ABI: `context` is (hook's this pointer + 8), not the
// address of a jumpTicks field. Only call on the exact verified build with a
// live game-owned context and the original surrounding preconditions.
// Returns false when the lookup takes one of the recovered early returns.
bool write_component_value(void* context, std::uint32_t value) noexcept;
}
