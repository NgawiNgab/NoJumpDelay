#include "ComponentLookup.hpp"
#include <cstddef>
#include <cstring>

namespace nojump {
namespace {
template<class T> T read(std::uintptr_t p) noexcept {
    T value{};
    std::memcpy(&value, reinterpret_cast<const void*>(p), sizeof(value));
    return value;
}
}

bool write_component_value(void* context, std::uint32_t value) noexcept {
    // Preconditions and array validity are established by the original game
    // context. Null checks here do not make stale or foreign pointers safe.
    if (!context) return false;
    const auto p = reinterpret_cast<std::uintptr_t>(context);
    const auto registry = read<std::uintptr_t>(p + 8);
    if (!registry) return false;
    const auto begin = read<std::uintptr_t>(registry + 0x38);
    const auto end = read<std::uintptr_t>(registry + 0x40);
    if (!begin) return false;
    const std::uint64_t bucket_offset =
        ((end - begin) + 0x7fffffff8ULL) & 0x629647bc0ULL;
    auto index = read<std::uint64_t>(begin + bucket_offset);
    if (index == UINT64_MAX) return false;
    const auto entity = read<std::uint32_t>(p + 0x10);
    const auto nodes = read<std::uintptr_t>(registry + 0x50);
    const auto nodes_end = read<std::uintptr_t>(registry + 0x58);
    if (!nodes) return false;
    for (;;) {
        const auto node = nodes + index * 0x20ULL;
        if (read<std::uint32_t>(node + 8) == 0xc52c8f78U) {
            if (node == nodes_end) return false;
            const auto store = read<std::uintptr_t>(node + 0x10);
            if (!store) return false;
            const auto sparse_begin = read<std::uintptr_t>(store + 8);
            const auto sparse_end = read<std::uintptr_t>(store + 0x10);
            const auto page_index = (entity >> 11) & 0x7fU;
            if ((sparse_end - sparse_begin) / 8 <= page_index) return false;
            const auto page = read<std::uintptr_t>(sparse_begin + page_index * 8ULL);
            if (!page) return false;
            const auto packed = read<std::uint32_t>(page + (entity & 0x7ffU) * 4ULL);
            if (((entity & 0xfffc0000ULL) ^ std::uint64_t{packed}) > 0x3fffeULL) return false;
            const auto chunks = read<std::uintptr_t>(store + 0x50);
            const auto chunk = read<std::uintptr_t>(chunks + ((packed >> 4) & 0x3ff8U));
            if (!chunk) return false;
            const auto destination = chunk + (packed & 0x7fU) * 0x14ULL + 0x10;
            std::memcpy(reinterpret_cast<void*>(destination), &value, sizeof(value));
            return true;
        }
        index = read<std::uint64_t>(node);
        if (index == UINT64_MAX) return false;
    }
}
}
