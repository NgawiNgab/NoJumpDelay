#include "nojump_core.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <cstdlib>

struct Arena {
    std::vector<unsigned char> data = std::vector<unsigned char>(0x12000);
    template<class T> bool read(std::uintptr_t p, T &v) const {
        if (p > data.size() || sizeof(T) > data.size()-p) return false;
        std::memcpy(&v, data.data()+p, sizeof(T)); return true;
    }
    bool can_write(std::uintptr_t p, std::size_t n) const { return p <= data.size() && n <= data.size()-p; }
    template<class T> void put(std::uintptr_t p, T v) { if(!can_write(p,sizeof(T))) std::abort(); std::memcpy(data.data()+p,&v,sizeof(T)); }
};
#define CHECK(x) do { if(!(x)) {std::cerr << "failed: " #x << " line " << __LINE__ << '\n';return 1;} } while(0)
int main() {
    static_assert(!nojump::default_enabled);
    static_assert(nojump::jump_stride == 20 && nojump::jump_hash == 0xc52c8f78);
    // Synthetic EnTT-like structures; this does not verify the real game ABI.
    Arena a; constexpr std::uintptr_t ctx=0x100,reg=0x200,buckets=0x300,entries=0x400,storage=0x600,sparse=0x700,page=0x1000,dense=0x900,block=0x5000;
    constexpr std::uint32_t entity=5, packed=5;
    a.put(ctx+8,reg); a.put(ctx+0x10,entity); a.put(reg+0x38,buckets);a.put(reg+0x40,buckets+8);
    a.put(reg+0x50,entries); a.put(reg+0x58,entries+0x20);a.put(buckets, std::uint64_t(0));
    a.put(entries,UINT64_MAX);a.put(entries+8,nojump::jump_hash);a.put(entries+0x10,storage);
    a.put(storage+8,sparse);a.put(storage+0x10,sparse+8);a.put(sparse,page);a.put(page+entity*4,packed);
    a.put(storage+0x50,dense);a.put(dense,block);
    CHECK(nojump::component(a,ctx,nojump::jump_hash,20)==block+5*20);
    CHECK(!nojump::component(a,0,nojump::jump_hash,20));
    CHECK(!nojump::component(a,ctx,0x1234,20));
    a.put(page+entity*4,0xffffffffu);CHECK(!nojump::component(a,ctx,nojump::jump_hash,20));
    a.put(page+entity*4,packed); a.put(sparse,std::uintptr_t(0));CHECK(!nojump::component(a,ctx,nojump::jump_hash,20));
    a.put(sparse,page); a.put(buckets,UINT64_MAX);CHECK(!nojump::component(a,ctx,nojump::jump_hash,20));
    a.put(buckets,std::uint64_t(0)); a.put(entries+8,0x1234u); a.put(entries,std::uint64_t(0));CHECK(!nojump::component(a,ctx,nojump::jump_hash,20));
    std::uintptr_t out=0; CHECK(!nojump::add(UINTPTR_MAX,1,out));
    return 0;
}
