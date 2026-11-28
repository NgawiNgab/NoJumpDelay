#include "kaura/re/GameAccess.h"

#include <cstring>

#include "kaura/re/Logger.h"
#include "kaura/re/Offsets.h"
#include "kaura/re/Resolver.h"

namespace kaura::re {

namespace {

template <typename Fn>
inline Fn fnAt(void *slot) {
    return reinterpret_cast<Fn>(slot);
}

// std::vector<void*> sret thunk: AArch64 AAPCS passes the sret buffer in x8
// for non-trivial returns; casting to a function returning the vector by
// value reproduces the call exactly.
using FetchActors = std::vector<void *> (*)(void *level);
using GetComponentFn = void *(*)(const void *actor, std::uint32_t compId);
using GetNameCompFn = void *(*)(const void *actor);
using InteractionCheckFn = bool (*)(const void *actor);
using AttackFn = void *(*)(void *gameMode, void *actor, std::uint32_t attack,
                           const Vec3 *pos);
using SwingFn = void *(*)(void *lp, std::uint32_t a, std::uint32_t b);
using EnaFn = void (*)(void *interaction);

inline std::uint32_t readU32(const void *base, std::uint64_t off) {
    std::uint32_t v;
    std::memcpy(&v, reinterpret_cast<const char *>(base) + off, sizeof(v));
    return v;
}

inline std::uintptr_t readUptr(const void *base, std::uint64_t off) {
    std::uintptr_t v;
    std::memcpy(&v, reinterpret_cast<const char *>(base) + off, sizeof(v));
    return v;
}

} // namespace

std::string readCppString(const void *strObj) {
    if (strObj == nullptr) return {};
    const auto *p = static_cast<const unsigned char *>(strObj);
    const unsigned char flag = p[0];
    if ((flag & 1) == 0) {
        const std::size_t len = flag >> 1;
        if (len == 0) return {};
        return std::string(reinterpret_cast<const char *>(p + 1),
                           reinterpret_cast<const char *>(p + 1) + len);
    }
    std::size_t len;
    const char *data;
    std::memcpy(&len, p + STR_LONG_SIZE, sizeof(len));
    std::memcpy(&data, p + STR_LONG_DATA, sizeof(data));
    if (data == nullptr || len == 0) return {};
    return std::string(data, data + len);
}

Vec3 readVec3(const void *ptr) {
    Vec3 v;
    std::memcpy(&v, ptr, sizeof(v));
    return v;
}

void *readPtr(const void *base, std::uint64_t off) {
    void *v;
    std::memcpy(&v, reinterpret_cast<const char *>(base) + off, sizeof(v));
    return v;
}

std::int32_t readInt32(const void *base, std::uint64_t off) {
    std::int32_t v;
    std::memcpy(&v, reinterpret_cast<const char *>(base) + off, sizeof(v));
    return v;
}

float readFloat(const void *base, std::uint64_t off) {
    float v;
    std::memcpy(&v, reinterpret_cast<const char *>(base) + off, sizeof(v));
    return v;
}

const std::string *fetchTagString(const GameApi &api, const void *actor) {
    // FUN_001AECD0 1:1:
    //   vec = *(vector**)(actor + 0xA00)
    //   if (vec && vec->begin && (byte)(vec->end - vec->begin) > 0x20)
    //     e = *(void**)(vec->begin + 0x20)
    //     if (e && *(char*)(e+8) == 4) return string at e + 0x10
    //   return static empty string
    const void *vec = readPtr(actor, 0xA00);
    if (vec != nullptr) {
        const std::uintptr_t begin = readUptr(vec, 0x0);
        const std::uintptr_t end = readUptr(vec, 0x8);
        if (begin != 0 && (end - begin) > 0x20) {
            const std::uintptr_t e = readUptr(reinterpret_cast<const void *>(begin), 0x20);
            if (e != 0) {
                const unsigned char tag =
                    *reinterpret_cast<const unsigned char *>(e + 8);
                if (tag == 4) {
                    return reinterpret_cast<const std::string *>(e + 0x10);
                }
            }
        }
    }
    static const std::string kEmpty;
    return &kEmpty;
}

bool resolveGameApi(GameApi &api) {
    api.base = findModuleBase("libminecraftpe.so");
    if (api.base == 0) {
        logError("libminecraftpe.so not found in /proc/self/maps");
        return false;
    }
    logInfo("libminecraftpe.so base = 0x%lx", (unsigned long)api.base);

    struct Entry {
        std::uint32_t rva;
        const unsigned char *prologue;
        void **slot;
        const char *name;
    };
    Entry entries[] = {
        {RVA_ACTOR_LIST_FETCH, PROLOGUE_ACTOR_LIST, &api.fetchActorsFn,
         "fetchActors"},
        {RVA_GET_COMPONENT, PROLOGUE_GET_COMPONENT, &api.getComponentFn,
         "getComponent"},
        {RVA_GET_NAME_COMPONENT, PROLOGUE_GET_NAME, &api.getNameComponentFn,
         "getNameComponent"},
        {RVA_INTERACTION_CHECK, nullptr, &api.interactionCheckFn,
         "interactionCheck"},
        {RVA_GAMEMODE_ATTACK, PROLOGUE_GAMEMODE_ATTACK, &api.attackFn,
         "GameMode::attack"},
        {RVA_SWING, PROLOGUE_SWING, &api.swingFn, "swing"},
        {RVA_ENA_A, nullptr, &api.enaAFn, "enaA"},
        {RVA_ENA_B, nullptr, &api.enaBFn, "enaB"},
    };

    bool ok = true;
    for (Entry &e : entries) {
        if (e.prologue != nullptr) {
            unsigned char actual[16];
            if (!verifyPrologue(api.base, e.rva, e.prologue, 16, actual)) {
                logError("prologue mismatch at %s (rva 0x%x) — refusing to bind",
                         e.name, e.rva);
                *e.slot = nullptr;
                ok = false;
                continue;
            }
        }
        *e.slot = reinterpret_cast<void *>(api.base + e.rva);
        logInfo("bound %s at base+0x%x", e.name, e.rva);
    }

    // Component ID constants (sigma slots 0x457C38/40/48 — data, not code).
    api.compIdA = 0x141D8F48u; // sigma 0x457C38 decoded value
    api.compIdB = 0x141D8D68u; // sigma 0x457C40 decoded value
    api.compIdC = 0x141D9E30u; // sigma 0x457C48 decoded value

    return ok;
}

bool buildSnapshot(const GameApi &api, const void *lp, const void *actor,
                   const std::string &selfName, ActorSnapshot &out) {
    out = ActorSnapshot{};
    out.actor = const_cast<void *>(actor);

    // sigma: skip null pos (actor[0x45]) / null aabb (actor[0x46]).
    const void *pos = readPtr(actor, OFF_ACTOR_POS);
    const void *aabb = readPtr(actor, OFF_ACTOR_AABB);
    if (pos == nullptr || aabb == nullptr) return false;

    out.pos = readVec3(pos);
    out.aabbWidth = readFloat(aabb, OFF_AABB_WIDTH);
    out.aabbHeight = readFloat(aabb, OFF_AABB_HEIGHT);
    out.hurtTime = readInt32(actor, OFF_ACTOR_HURT_TIME);

    // sigma interaction-check(actor) selects the path.
    if (api.interactionCheckFn != nullptr) {
        const auto check = fnAt<InteractionCheckFn>(api.interactionCheckFn);
        out.namedPath = check(actor);
    } else {
        out.namedPath = true;
    }

    if (!out.namedPath) {
        // nametag path: fetch the name-tag component string (sigma 0x457BB0).
        if (api.getNameComponentFn != nullptr) {
            const auto fn = fnAt<GetNameCompFn>(api.getNameComponentFn);
            const void *comp = fn(actor);
            if (comp != nullptr) {
                out.nameTag = readCppString(
                    reinterpret_cast<const char *>(comp) + OFF_NAMECOMP_STRING);
            }
        }
        return true;
    }

    // named path
    out.actorName = readCppString(reinterpret_cast<const char *>(actor) +
                                  OFF_ACTOR_NAME);
    const std::string *tag = fetchTagString(api, actor);
    out.tagStr = tag != nullptr ? *tag : std::string();

    // health via getComponent(actor, compIdA) -> +0x7C (sigma).
    if (api.getComponentFn != nullptr) {
        const auto fn = fnAt<GetComponentFn>(api.getComponentFn);
        const void *comp = fn(actor, api.compIdA);
        if (comp != nullptr) {
            out.health = static_cast<int>(readFloat(comp, OFF_COMPONENT_HEALTH));
        }
    }
    (void)selfName;
    return true;
}

} // namespace kaura::re
