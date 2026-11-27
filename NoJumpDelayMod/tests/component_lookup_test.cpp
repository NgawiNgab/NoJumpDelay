#include "../src/ComponentLookup.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

template<class T> void put(void* p, std::size_t offset, T value) {
    std::memcpy(static_cast<unsigned char*>(p) + offset, &value, sizeof(value));
}
int main() {
    alignas(8) unsigned char context[32]{}, registry[128]{}, node[32]{}, store[96]{}, chunk[64]{};
    std::uint64_t buckets[1]{0};
    std::uint32_t page[1]{0};
    std::uintptr_t pages[1]{reinterpret_cast<std::uintptr_t>(page)};
    std::uintptr_t chunks[1]{reinterpret_cast<std::uintptr_t>(chunk)};
    auto ptr = [](void* p) { return reinterpret_cast<std::uintptr_t>(p); };
    put(context, 8, ptr(registry)); put(context, 16, std::uint32_t{0});
    put(registry, 0x38, ptr(buckets)); put(registry, 0x40, ptr(buckets) + 8);
    put(registry, 0x50, ptr(node)); put(registry, 0x58, ptr(node) + 32);
    put(node, 0, std::numeric_limits<std::uint64_t>::max());
    put(node, 8, std::uint32_t{0xc52c8f78}); put(node, 16, ptr(store));
    put(store, 8, ptr(pages)); put(store, 16, ptr(pages) + 8); put(store, 0x50, ptr(chunks));
    put(chunk, 0x10, std::uint32_t{42});
    assert(nojump::write_component_value(context, 0));
    std::uint32_t result; std::memcpy(&result, chunk + 0x10, 4); assert(result == 0);
    put(node, 8, std::uint32_t{0});
    assert(!nojump::write_component_value(context, 0));
    put(node, 8, std::uint32_t{0xc52c8f78});
    put(store, 16, ptr(pages));
    assert(!nojump::write_component_value(context, 0));
    put(store, 16, ptr(pages) + 8);
    page[0] = 0x40000;
    assert(!nojump::write_component_value(context, 0));
    page[0] = 0; chunks[0] = 0;
    assert(!nojump::write_component_value(context, 0));
    assert(!nojump::write_component_value(nullptr, 0));
    std::cout << "component lookup synthetic tests passed\n";
}
