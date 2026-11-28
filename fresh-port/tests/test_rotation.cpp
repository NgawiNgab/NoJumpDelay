#include "rotation.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

template<class T> void put(void* p,std::size_t offset,T v) {std::memcpy(static_cast<unsigned char*>(p)+offset,&v,sizeof(v));}
int main() {
 std::array<unsigned char,0x240> actor{};
 std::array<unsigned char,0x60> registry{};
 std::array<unsigned char,0x20> node{};
 std::array<unsigned char,0x58> storage{};
 std::array<std::uint64_t,1> buckets{0};
 std::array<std::uint32_t,2048> sparsePage{};
 std::array<void*,1> sparsePages{sparsePage.data()};
 std::array<float,256> values{};
 std::array<void*,1> packedPages{values.data()};
 float direct[2]{};
 put(actor.data(),0x10,registry.data());put(actor.data(),0x18,std::uint32_t{2});put(actor.data(),0x238,direct);
 put(registry.data(),0x38,buckets.data());put(registry.data(),0x40,buckets.data()+1);
 put(registry.data(),0x50,node.data());put(registry.data(),0x58,node.data()+node.size());
 put(node.data(),0,std::numeric_limits<std::uint64_t>::max());put(node.data(),8,std::uint32_t{0xbabe7211});put(node.data(),0x10,storage.data());
 put(storage.data(),8,sparsePages.data());put(storage.data(),0x10,sparsePages.data()+1);put(storage.data(),0x50,packedPages.data());
 sparsePage[2]=2;
 aura::injectRotation(actor.data(),12.f,34.f);
 assert(direct[0]==12.f && direct[1]==34.f && values[4]==34.f);
 sparsePage[2]=0xffffffffu;
 aura::injectRotation(actor.data(),-1.f,-2.f);
 assert(direct[0]==-1.f && direct[1]==-2.f && values[4]==34.f);
 put(actor.data(),0x10,static_cast<void*>(nullptr));
 aura::injectRotation(actor.data(),1.f,2.f);assert(direct[0]==1.f && direct[1]==2.f);
 aura::injectRotation(nullptr,1.f,2.f);
}
