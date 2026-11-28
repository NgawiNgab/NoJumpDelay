#include "kaura/re/Resolver.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace kaura::re {

std::uintptr_t findModuleBase(const char *name) {
    // FUN_001931B8 semantics: fopen("/proc/self/maps"), fgets 0x400,
    // strstr(line, libname), strtoul(line, base 16).
    FILE *f = std::fopen("/proc/self/maps", "rt");
    if (f == nullptr) return 0;

    char line[1024];
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        if (std::strstr(line, name) != nullptr) {
            // strtoul stops at the '-' of the range token.
            const std::uintptr_t base = std::strtoull(line, nullptr, 16);
            std::fclose(f);
            if (base != 0) return base;
            // keep scanning (a stale mapping matched first)
        }
    }
    std::fclose(f);
    return 0;
}

bool verifyPrologue(std::uintptr_t base, std::uint32_t rva,
                    const unsigned char *expected, std::size_t len,
                    unsigned char *outActual) {
    const unsigned char *live =
        reinterpret_cast<const unsigned char *>(base + rva);
    if (outActual != nullptr) {
        for (std::size_t i = 0; i < len && i < 16; ++i) outActual[i] = live[i];
    }
    return std::memcmp(live, expected, len) == 0;
}

} // namespace kaura::re
