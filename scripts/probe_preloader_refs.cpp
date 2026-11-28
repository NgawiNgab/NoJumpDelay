// Probe: references every PL_EXPORT runtime API (from the vendored official
// LiteLDev/preloader-android SDK headers) that a mod library is allowed to
// leave undefined for dlopen-time resolution against libpreloader.so.
//
// The symbol audit (scripts/audit_preloader_symbols.sh) compiles this TU and
// reads its undefined pl::/ll:: symbols to build the ground-truth allowlist;
// any undefined pl::/ll:: symbol in the built mod that is NOT in that set
// fails the build (would dlopen-fail on device like the NativeMod bug did).

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/memory/Hook.hpp>

namespace {

const void *const kRuntimeRefs[] = {
    // Mod lifecycle (ll::mod — the libpreloader.so device-runtime ABI).
    reinterpret_cast<const void *>(&ll::mod::NativeMod::current),

    // pl::modmenu registry. registerModule/registerButton take (const
    // ModuleInfo&) / (const ButtonInfo&) — std-type-free mangles, stable
    // across libc++ flavors. The unregister* pair takes std::string_view
    // (ABI-flavor-dependent mangle) and is resolved via dlsym at runtime
    // instead (see KillAuraMod.cpp unregisterModmenuModule/Button).
    reinterpret_cast<const void *>(&pl::modmenu::registerModule),
    reinterpret_cast<const void *>(&pl::modmenu::registerButton),

    // pl::memory hooking.
    reinterpret_cast<const void *>(&pl::memory::hook),
    reinterpret_cast<const void *>(&pl::memory::unhook),
};

} // namespace

extern "C" void *kauraPreloaderProbeRefs() { return const_cast<void **>(kRuntimeRefs); }
