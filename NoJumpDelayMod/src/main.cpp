#include <android/log.h>
#include <pl/Mod.hpp>

// Fail closed: the live detour is intentionally NOT installed. The recovered
// ECS lookup is available in ComponentLookup.cpp, but reproducing ReverseSigma
// also requires its complete outer guards and server/anti-cheat state.
class NoJumpDelayMod {
public:
    bool load(pl::mod::ModContext&) {
        __android_log_print(ANDROID_LOG_WARN, "NoJumpDelay", "Forensic prototype loaded; no hook installed");
        return true;
    }
    bool enable(pl::mod::ModContext&) {
        __android_log_print(ANDROID_LOG_ERROR, "NoJumpDelay", "Not enabled: original hook guards are not fully verified");
        return false;
    }
    bool disable(pl::mod::ModContext&) { return true; }
    bool unload(pl::mod::ModContext&) { return true; }
};
PL_REGISTER_MOD(NoJumpDelayMod, NoJumpDelayMod{})
