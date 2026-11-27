#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/memory/Hook.hpp>
#include <nlohmann/json.hpp>
#include "nojump_core.hpp"

#include <android/log.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <link.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr std::array<unsigned char,20> kBuildId{0xeb,0x29,0x1b,0x88,0x3d,0xfd,0xfa,0xff,0x70,0xc4,0x99,0x9c,0x2e,0xab,0xd6,0x18,0x85,0xae,0x3e,0x15};
constexpr std::array<unsigned char,16> kTickBytes{0xe8,0x0f,0x19,0xfc,0xfd,0x7b,0x01,0xa9,0xfc,0x6f,0x02,0xa9,0xfa,0x67,0x03,0xa9};
constexpr std::array<unsigned char,16> kHealthBytes{0xfd,0x7b,0xbf,0xa9,0xfd,0x03,0x00,0x91,0x09,0x08,0x40,0xf9,0x6a,0xc2,0x80,0x52};
constexpr const char* kMenuId = "no_jump_delay";
using TickFn = void (*)(void*);
using HealthFn = void* (*)(void*, const void*);
static_assert(std::is_same_v<TickFn, void (*)(void*)>);
static_assert(std::is_same_v<HealthFn, void* (*)(void*, const void*)>);

struct Range { std::uintptr_t start, end; bool writable; };
class MemoryView {
    std::vector<Range> ranges_;
public:
    MemoryView() {
        FILE *f=std::fopen("/proc/self/maps","r");
        if (!f) return;
        char line[1024], perms[5]; unsigned long long lo,hi;
        while (std::fgets(line,sizeof(line),f)) {
            if (std::sscanf(line,"%llx-%llx %4s",&lo,&hi,perms)==3 && perms[0]=='r' && hi>lo)
                ranges_.push_back({static_cast<std::uintptr_t>(lo),static_cast<std::uintptr_t>(hi),perms[1]=='w'});
        }
        std::fclose(f);
    }
    bool valid(std::uintptr_t a,std::size_t n,bool write=false) const {
        if (!a || !n || n > UINTPTR_MAX-a) return false;
        for (auto const &r:ranges_) if (a>=r.start && a+n<=r.end && (!write || r.writable)) return true;
        return false;
    }
    template<class T> bool read(std::uintptr_t a,T &value) const {
        if (!valid(a,sizeof(T))) return false;
        std::memcpy(&value,reinterpret_cast<const void*>(a),sizeof(T));return true;
    }
    bool can_write(std::uintptr_t a,std::size_t n) const { return valid(a,n,true); }
};

struct Game { std::uintptr_t base{}; bool found{}, build_match{}; };
int findGame(dl_phdr_info* info, size_t, void* data) {
    auto *game=static_cast<Game*>(data);
    const char* name=info->dlpi_name;
    if (!name) return 0;
    const char* slash=std::strrchr(name,'/');
    if (std::strcmp(slash?slash+1:name,"libminecraftpe.so")!=0) return 0;
    game->found=true;game->base=static_cast<std::uintptr_t>(info->dlpi_addr);
    for (std::size_t i=0;i<info->dlpi_phnum;i++) {
        const auto &p=info->dlpi_phdr[i]; if(p.p_type!=PT_NOTE || p.p_memsz<sizeof(ElfW(Nhdr))) continue;
        // PT_NOTE must be backed by a readable PT_LOAD, not just listed in PHDRs.
        bool covered=false;
        for (std::size_t j=0;j<info->dlpi_phnum;j++) {
            const auto &load=info->dlpi_phdr[j];
            if (load.p_type==PT_LOAD && (load.p_flags&PF_R) && p.p_vaddr>=load.p_vaddr &&
                p.p_memsz<=load.p_memsz && p.p_vaddr-load.p_vaddr<=load.p_memsz-p.p_memsz) covered=true;
        }
        if (!covered) continue;
        auto *buf=reinterpret_cast<const unsigned char*>(game->base+p.p_vaddr);
        std::size_t off=0, size=static_cast<std::size_t>(p.p_memsz);
        while (off<=size-sizeof(ElfW(Nhdr))) {
            ElfW(Nhdr) n{};std::memcpy(&n,buf+off,sizeof(n));off+=sizeof(n);
            std::size_t names=(static_cast<std::size_t>(n.n_namesz)+3)&~std::size_t(3);
            std::size_t desc=(static_cast<std::size_t>(n.n_descsz)+3)&~std::size_t(3);
            if(names>size-off || desc>size-off-names) break;
            if(n.n_type==NT_GNU_BUILD_ID && n.n_namesz==4 && n.n_descsz==kBuildId.size() &&
               std::memcmp(buf+off,"GNU\0",4)==0 && std::memcmp(buf+off+names,kBuildId.data(),kBuildId.size())==0)
                game->build_match=true;
            off+=names+desc;
        }
    }
    return 1;
}

class Mod {
    std::atomic_bool active_{false}, enabled_{nojump::default_enabled}, stop_{false};
    std::atomic_uint in_tick_{0};
    std::mutex mutex_, log_mutex_, config_mutex_;
    std::thread worker_;
    void *original_{};
    void *target_{};
    std::uintptr_t base_{};
    bool menu_registered_{};
    FILE *log_{};
    std::filesystem::path config_path_;
    static Mod* self_;

    void log(const char* message) {
        std::lock_guard lock(log_mutex_);
        if (log_) { std::fprintf(log_,"%s\n",message);std::fflush(log_); }
        else __android_log_write(ANDROID_LOG_WARN,"NoJumpDelay",message);
    }
    void saveConfig(bool value) {
        std::lock_guard lock(config_mutex_);
        try {
            const auto temp=config_path_.string()+".tmp";
            std::ofstream out(temp,std::ios::trunc);
            out << nlohmann::json{{"noJumpDelay",value}}.dump(2) << '\n';
            out.close();
            if(!out || std::rename(temp.c_str(),config_path_.string().c_str())!=0) log("ERROR: could not save config.json");
        } catch (...) { log("ERROR: exception while saving config.json"); }
    }
    void setOption(bool value) {
        enabled_.store(value,std::memory_order_release);
        saveConfig(value);
        log(value?"noJumpDelay=true":"noJumpDelay=false");
    }
    void attemptInstall() {
        Game game{}; dl_iterate_phdr(findGame,&game);
        if (!game.found) return;
        if (!game.base || !game.build_match) {log("ERROR: 1.26.60 build ID mismatch; hook not installed");stop_=true;return;}
        MemoryView view;
        std::uintptr_t tick=0,health=0,attr=0;
        if (!nojump::add(game.base,nojump::tick_rva,tick) || !nojump::add(game.base,nojump::health_get_rva,health) ||
            !nojump::add(game.base,nojump::health_attr_rva,attr) || !view.valid(tick,kTickBytes.size()) ||
            !view.valid(health,kHealthBytes.size()) || !view.valid(attr,8) ||
            std::memcmp(reinterpret_cast<void*>(tick),kTickBytes.data(),kTickBytes.size()) ||
            std::memcmp(reinterpret_cast<void*>(health),kHealthBytes.data(),kHealthBytes.size())) {
            log("ERROR: expected 1.26.60 code/data bytes not present; hook not installed");stop_=true;return;
        }
        std::lock_guard lock(mutex_);
        if (stop_.load()) return;
        base_=game.base;target_=reinterpret_cast<void*>(tick);
        char buf[256];std::snprintf(buf,sizeof(buf),"Game 1.26.60 build ID verified; base=%p tick=%p health=%p attr=%p",reinterpret_cast<void*>(base_),target_,reinterpret_cast<void*>(health),reinterpret_cast<void*>(attr));log(buf);
        const int hook_result = pl::memory::hook(target_,reinterpret_cast<void*>(&Mod::tick),&original_);
        if (hook_result != 0) {
            log("ERROR: tick hook installation failed");target_=nullptr;stop_=true;return;
        }
        if (!original_) {
            active_=false;
            log("ERROR: hook installed without an original; restart required");stop_=true;return;
        }
        log("tick hook installed; original prologue e80f19fcfd7b01a9fc6f02a9fa6703a9");
        stop_=true;
    }
    void run() {
        while (!stop_.load(std::memory_order_acquire)) {
            attemptInstall();
            if (!stop_.load()) std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
    void apply(void *raw) {
        if (!active_.load(std::memory_order_acquire) || !enabled_.load(std::memory_order_acquire) || !raw) return;
        const auto player=reinterpret_cast<std::uintptr_t>(raw);
        MemoryView r;std::uintptr_t pos=0, mode=0, dimension=0, ctx=0, health_comp=0, jump=0, field=0;
        if (!nojump::read_at(r,player,0x228,pos) || !pos || !r.valid(pos,12) ||
            !nojump::read_at(r,player,0xa08,mode) || !mode || !r.valid(mode,8) ||
            !nojump::read_at(r,player,0x1e0,dimension) || !dimension || !r.valid(dimension,8) ||
            !nojump::add(player,8,ctx)) return;
        // Check the game's required attribute-map component exists before using the
        // game getter, which traps on missing components. A map can still change concurrently.
        health_comp=nojump::component(r,ctx,nojump::attributes_hash,0x58);
        jump=nojump::component(r,ctx,nojump::jump_hash,nojump::jump_stride);
        if (!health_comp || !jump || !r.valid(health_comp,0x58) || !nojump::add(jump,0x10,field) || !r.can_write(field,4)) return;
        const auto getter=reinterpret_cast<HealthFn>(base_+nojump::health_get_rva);
        const void *attribute=reinterpret_cast<const void*>(base_+nojump::health_attr_rva);
        const auto instance=reinterpret_cast<std::uintptr_t>(getter(raw,attribute));
        float health=0;
        if (!instance || !nojump::read_at(r,instance,0x7c,health) || !std::isfinite(health) || health<=0.0f) return;
        const std::int32_t zero=0; std::memcpy(reinterpret_cast<void*>(field),&zero,sizeof(zero));
    }
    static void tick(void *player) {
        Mod *s=self_; if(!s) return;
        s->in_tick_.fetch_add(1,std::memory_order_acq_rel);
        TickFn original;
        { std::lock_guard lock(s->mutex_); original=reinterpret_cast<TickFn>(s->original_); }
        if (original) {
            try { s->apply(player); } catch (...) { s->log("ERROR: exception in tick guard; original will still run"); }
            original(player);
        } else s->log("ERROR: missing original tick trampoline");
        s->in_tick_.fetch_sub(1,std::memory_order_acq_rel);
    }
public:
    static Mod& instance() { static Mod m; return m; }
    bool load(pl::mod::ModContext &ctx) {
        self_=this;
        try {
            std::filesystem::create_directories(ctx.dataDir());
            std::filesystem::create_directories(ctx.configDir());
            log_=std::fopen((ctx.dataDir()/"nojumpdelay.log").string().c_str(),"a");
            config_path_=ctx.configDir()/"config.json";
            bool value=nojump::default_enabled;
            if (std::filesystem::exists(config_path_)) {
                try { std::ifstream in(config_path_);auto j=nlohmann::json::parse(in);if(j.contains("noJumpDelay") && j["noJumpDelay"].is_boolean()) value=j["noJumpDelay"].get<bool>(); else log("ERROR: invalid noJumpDelay value; defaulting false"); }
                catch (...) { log("ERROR: invalid config.json; defaulting false"); }
            } else saveConfig(value);
            enabled_.store(value);
            log(value?"load: noJumpDelay=true":"load: noJumpDelay=false");
            pl::modmenu::ModuleBuilder b(kMenuId,"No Jump Delay");
            b.modId(ctx.id()).description("Experimental Bedrock 1.26.60 jump cooldown removal")
             .defaultEnabled(value).onToggle([this](std::string_view,bool v){setOption(v);});
            menu_registered_=b.registerModule();
            if(!menu_registered_) log("WARNING: ModMenu registration failed; use config.json");
            return true;
        } catch (...) { log("ERROR: load failed");return false; }
    }
    bool enable(pl::mod::ModContext&) {
        active_=true;stop_=false;
        if(!target_ && !worker_.joinable()) worker_=std::thread([this]{run();});
        log("mod enabled");return true;
    }
    bool disable(pl::mod::ModContext&) {
        active_=false;stop_=true;if(worker_.joinable()) worker_.join();log("mod disabled");return true;
    }
    bool unload(pl::mod::ModContext &ctx) {
        disable(ctx);
        // The hook API does not promise quiescence of threads already entering
        // the detour/trampoline. Never unload code underneath such a thread.
        if (target_) { log("ERROR: live hook cannot safely unload; restart required"); return false; }
        if(menu_registered_) {pl::modmenu::unregisterModule(kMenuId);menu_registered_=false;}
        log("mod unloaded");
        { std::lock_guard lock(log_mutex_);if(log_) {std::fclose(log_);log_=nullptr;} }
        self_=nullptr;return true;
    }
};
Mod* Mod::self_=nullptr;
} // namespace
PL_REGISTER_MOD(Mod, Mod::instance())
