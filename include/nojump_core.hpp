#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace nojump {
inline constexpr std::uintptr_t tick_rva = 0x0af5e460;
inline constexpr std::uintptr_t health_get_rva = 0x0fdabbfc;
inline constexpr std::uintptr_t health_attr_rva = 0x141d8f48;
inline constexpr std::uint32_t jump_hash = 0xc52c8f78;
inline constexpr std::uint32_t attributes_hash = 0xfd3b0613;
inline constexpr std::size_t jump_stride = 0x14;
inline constexpr bool default_enabled = false;

inline bool add(std::uintptr_t a, std::uintptr_t b, std::uintptr_t &out) {
    if (b > std::numeric_limits<std::uintptr_t>::max() - a) return false;
    out = a + b; return true;
}

template <typename T, typename Reader>
bool read_at(const Reader &reader, std::uintptr_t base, std::uintptr_t offset, T &out) {
    std::uintptr_t address;
    return add(base, offset, address) && reader.read(address, out);
}

// Mirror the verified sparse/dense lookup, but fail closed on malformed pointers,
// sizes or chains instead of entering the game's assert/trap path.
// Reader must validate readability and provide read(addr, value) and
// can_write(addr, bytes). Returns the component element, never creates one.
template <typename Reader>
std::uintptr_t component(const Reader &r, std::uintptr_t ctx, std::uint32_t hash, std::size_t stride) {
    if (!ctx || stride == 0) return 0;
    std::uintptr_t registry=0, begin=0, end=0, entries=0, entries_end=0;
    std::uint32_t entity=0;
    if (!read_at(r, ctx, 8, registry) || !registry || !read_at(r, ctx, 0x10, entity) ||
        !read_at(r, registry, 0x38, begin) || !read_at(r, registry, 0x40, end) ||
        !read_at(r, registry, 0x50, entries) || !read_at(r, registry, 0x58, entries_end) ||
        !begin || !entries || end < begin || entries_end < entries || (end-begin)%8 ||
        (entries_end-entries)%0x20) return 0;
    auto bucket_count = (end-begin)/8;
    auto count = (entries_end-entries)/0x20;
    if (!bucket_count || (bucket_count & (bucket_count-1)) || bucket_count > (1u<<24) || count > (1u<<24)) return 0;
    std::uint64_t index=0;
    if (!read_at(r, begin, ((bucket_count-1) & hash)*8, index)) return 0;
    std::uintptr_t storage=0;
    for (std::size_t steps=0; index != UINT64_MAX && steps < count; ++steps) {
        if (index >= count) return 0;
        std::uintptr_t entry=0; std::uint32_t candidate=0;
        if (!add(entries, index*0x20, entry) || !read_at(r, entry, 8, candidate)) return 0;
        if (candidate == hash) {
            if (!read_at(r, entry, 0x10, storage)) return 0;
            break;
        }
        if (!r.read(entry, index)) return 0;
    }
    if (!storage) return 0;
    std::uintptr_t sparse=0, sparse_end=0, page=0, dense=0, block=0;
    if (!read_at(r, storage, 8, sparse) || !read_at(r, storage, 0x10, sparse_end) ||
        !sparse || sparse_end < sparse || (sparse_end-sparse)%8) return 0;
    auto page_index = (entity >> 11) & 0x7f;
    if (page_index >= (sparse_end-sparse)/8 || !read_at(r, sparse, page_index*8, page) || !page) return 0;
    std::uint32_t packed=0;
    if (!read_at(r, page, (entity & 0x7ff)*4, packed) ||
        ((entity & 0xfffc0000u) ^ packed) > 0x3fffeu ||
        !read_at(r, storage, 0x50, dense) || !dense ||
        !read_at(r, dense, (packed >> 4) & 0x3ff8, block) || !block) return 0;
    std::uintptr_t result=0;
    if (!add(block, static_cast<std::uintptr_t>(packed & 0x7f) * stride, result)) return 0;
    return result;
}
} // namespace nojump
