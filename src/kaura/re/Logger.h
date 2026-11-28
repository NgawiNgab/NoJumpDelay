#pragma once

// Small printf-style logger used by the RE binding layers. The mod-level
// logging goes through pl::log::Logger (LeviLauncher SDK); these helpers
// avoid pulling fmt into every TU.
//
// __android_log_* are only weak fallback definitions here: on a real device
// the strong symbols from the game's liblog.so win; for offline/host builds
// there is no NDK liblog to link against.

#include <android/log.h>
#include <cstdarg>

#if defined(__ANDROID__) && !defined(KAURA_NO_LOG_STUB)
#define KAURA_LOG_STUB_ATTR __attribute__((weak))
#else
#define KAURA_LOG_STUB_ATTR
#endif

#if !defined(__ANDROID__) || !defined(KAURA_NO_LOG_STUB)
extern "C" KAURA_LOG_STUB_ATTR inline int __android_log_print(int prio,
                                                       const char *tag,
                                                       const char *fmt,
                                                       ...) {
    (void)prio;
    (void)tag;
    (void)fmt;
    return 0;
}
extern "C" KAURA_LOG_STUB_ATTR inline int __android_log_vprint(int prio,
                                                         const char *tag,
                                                         const char *fmt,
                                                         va_list ap) {
    (void)prio;
    (void)tag;
    (void)fmt;
    (void)ap;
    return 0;
}
#endif

namespace kaura::re {

inline void vlog(int prio, const char *fmt, va_list ap) {
    __android_log_vprint(prio, "ApollonKillAura", fmt, ap);
}

__attribute__((format(printf, 1, 2)))
inline void logInfo(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(ANDROID_LOG_INFO, fmt, ap);
    va_end(ap);
}

__attribute__((format(printf, 1, 2)))
inline void logWarn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(ANDROID_LOG_WARN, fmt, ap);
    va_end(ap);
}

__attribute__((format(printf, 1, 2)))
inline void logError(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(ANDROID_LOG_ERROR, fmt, ap);
    va_end(ap);
}

} // namespace kaura::re
