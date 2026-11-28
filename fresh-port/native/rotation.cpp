#include "rotation.hpp"
#include <cstdint>
#include <cstring>
#include <limits>

namespace aura {
namespace {
template<class T> T read(const void* p,std::size_t off) {
 T out; std::memcpy(&out,static_cast<const unsigned char*>(p)+off,sizeof(out));return out;
}
template<class T> void write(void* p,std::size_t off,T v) {
 std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));
}
void writeRegistryYaw(void* actor,float yaw) {
 // Recovered from the AArch64 body of Sigma's rotation detour; the registry
 // layout and lifetime on a real device are not independently validated.
 void* registry=read<void*>(actor,0x10);
 if(!registry) return;
 const auto begin=read<std::uintptr_t>(registry,0x38), end=read<std::uintptr_t>(registry,0x40);
 if(!begin || end<begin || end-begin<8) return;
 const std::uintptr_t bytes=end-begin;
 const std::uintptr_t bucket=(bytes+0x7fffffff8ULL)&0x5d5f39088ULL;
 if(bucket>bytes-8) return;
 auto index=read<std::uint64_t>(reinterpret_cast<void*>(begin),bucket);
 const auto nodes=read<std::uintptr_t>(registry,0x50), nodesEnd=read<std::uintptr_t>(registry,0x58);
 if(!nodes || nodesEnd<nodes) return;
 const std::uint32_t entity=read<std::uint32_t>(actor,0x18);
 const std::size_t page=(entity>>11)&0x7f;
 for(std::size_t guard=0;index!=std::numeric_limits<std::uint64_t>::max() && guard<4096;++guard) {
  if(index >= (nodesEnd-nodes)/0x20) return;
  auto* node=reinterpret_cast<void*>(nodes+index*0x20);
  const auto next=read<std::uint64_t>(node,0);
  if(read<std::uint32_t>(node,8)==0xbabe7211u) {
   void* storage=read<void*>(node,0x10);
   if(storage) {
    const auto sparse=read<std::uintptr_t>(storage,8), sparseEnd=read<std::uintptr_t>(storage,0x10);
    if(sparse && sparseEnd>=sparse && page<(sparseEnd-sparse)/8) {
     auto* sparsePage=read<void*>(reinterpret_cast<void*>(sparse),page*8);
     if(sparsePage) {
      const auto packed=read<std::uint32_t>(sparsePage,(entity&0x7ffu)*4);
      if(((static_cast<std::uint64_t>(entity&0xfffc0000u)^packed)<0x3ffffu)) {
       void* pageTable=read<void*>(storage,0x50);
       if(pageTable) {
        void* values=read<void*>(pageTable,(packed>>4)&0x3ff8u);
        if(values) write(values,(packed&0x7fu)*8,yaw);
       }
      }
     }
    }
   }
  }
  index=next;
 }
}
}
void injectRotation(void* actor,float pitch,float yaw) {
 if(!actor) return;
 auto* direct=read<void*>(actor,0x238);
 if(direct) {write(direct,0,pitch);write(direct,4,yaw);}
 writeRegistryYaw(actor,yaw);
}
}
