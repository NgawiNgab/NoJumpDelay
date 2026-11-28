#include "combat.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string>

namespace aura {
namespace {
template<typename T> T read(const void* p, std::size_t offset) {
 T value; std::memcpy(&value, static_cast<const unsigned char*>(p)+offset, sizeof(value)); return value;
}
// Target libc++ layout observed in the supplied binary: low bit of first
// byte selects long form. Bound lengths before allocating or copying.
std::optional<std::string> gameString(const void* object) {
 const auto* bytes=static_cast<const unsigned char*>(object);
 std::size_t size=bytes[0]>>1;
 const char* data=reinterpret_cast<const char*>(bytes+1);
 if(bytes[0]&1) {size=read<std::size_t>(object,8); data=read<const char*>(object,16);}
 else if(size>22) return std::nullopt;
 if(size>4096 || (!data && size)) return std::nullopt;
 return size ? std::string(data,size) : std::string{};
}
std::optional<std::string> name(void* actor, const GameApi& api) {
 void* component=api.nameComponent(actor);
 return component ? gameString(static_cast<const unsigned char*>(component)+0x48) : std::nullopt;
}
std::optional<std::string> team(void* actor) {
 return gameString(static_cast<const unsigned char*>(actor)+0xb28);
}
std::uint32_t hash(const std::string& s) {
 std::uint32_t h=0x811c9dc5u;
 for(unsigned char c:s) {h^=c; h*=0x01000193u;}
 return h;
}
bool alive(void* actor, const GameApi& api) {
 void* c=api.component(actor,api.aliveKey);
 return !c || read<float>(c,0x7c)!=0.f;
}
bool finite(Vec3 p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
Rotation look(void* player, Vec3 eye, const Candidate& target) {
 const float half=target.width*.5f;
 const Vec3 p{std::clamp(eye.x,target.origin.x-half,target.origin.x+half),
              std::clamp(eye.y,target.origin.y-target.height,target.origin.y),
              std::clamp(eye.z,target.origin.z-half,target.origin.z+half)};
 const float dx=p.x-eye.x, dy=p.y-eye.y, dz=p.z-eye.z;
 constexpr float degrees=57.29577951308232f;
 return {player,true,std::atan2(dz,dx)*degrees-90.f,-std::atan2(dy,std::hypot(dx,dz))*degrees};
}
}
GameApi GameApi::fromBase(std::uintptr_t b) {
 return {reinterpret_cast<decltype(actors)>(b+0x10b16414),
         reinterpret_cast<decltype(component)>(b+0x0fdabbfc),
         reinterpret_cast<decltype(isPlayer)>(b+0x0fd8e120),
         reinterpret_cast<decltype(nameComponent)>(b+0x0fd881d4),
         reinterpret_cast<decltype(attack)>(b+0x1009cba4),
         reinterpret_cast<decltype(swing)>(b+0x0af60d78),
         reinterpret_cast<decltype(nativeBegin)>(b+0x09b6bbd4),
         reinterpret_cast<decltype(nativeEnd)>(b+0x09b94c98),
         reinterpret_cast<const void*>(b+0x141d8f48)};
}
void Combat::setRotation(Rotation value) {std::lock_guard lock(rotationMutex_); rotation_=value;}
Rotation Combat::rotation() {std::lock_guard lock(rotationMutex_); return rotation_;}
void Combat::reset() {
 std::lock_guard lock(tickMutex_); nextAttack_={}; setRotation({});
}
void Combat::tick(void* lp, const Config& c, const GameApi& api, std::chrono::steady_clock::time_point now) {
 std::lock_guard lock(tickMutex_);
 setRotation({});
 if(!lp || (!c.killAura && !c.killItems)) return;
 auto* pos=read<Vec3*>(lp,0x228);
 void* level=read<void*>(lp,0x1f0);
 void* mode=read<void*>(lp,0xa08);
 if(!pos||!level||!mode||!alive(lp,api)||!finite(*pos)) return;
 Vec3 eye=*pos; eye.y-=1.62f;
 const auto localTeam=c.killAura ? team(lp) : std::optional<std::string>(std::string{});
 if(!localTeam) return;
 std::vector<Candidate> candidates;
 std::vector<void*> actors;
 float nearest=std::numeric_limits<float>::infinity();
 Rotation nearestRotation{};
 const float rotationRange=c.rotations ? std::max(c.killRange,c.rotationsRange) : c.killRange;
 // C++ vector return uses AArch64 x8 indirect-result ABI. Assumes the game and
 // module share the Android libc++ allocator and vector ABI.
 for(void* actor:api.actors(level)) {
  if(!actor||actor==lp) continue;
  auto* p=read<Vec3*>(actor,0x228);
  auto* aabb=read<float*>(actor,0x230);
  if(!p||!aabb||!finite(*p)) continue;
  float width=aabb[6],height=aabb[7];
  if(!api.isPlayer(actor)) {
   const auto itemName=c.killItems ? name(actor,api) : std::nullopt;
   if(itemName && hash(*itemName)==0xf228491fu &&
      std::hypot(pos->x-p->x,pos->y-p->y,pos->z-p->z)<=6.f) {
    api.attack(mode,actor,true,p);
    if(c.swing) api.swing(lp,4,0);
   }
   continue;
  }
  if(!c.killAura) continue;
  const auto theirTeam=team(actor);
  const auto playerName=name(actor,api);
  if(!theirTeam || !playerName || (!localTeam->empty() && *localTeam==*theirTeam)) continue;
  Candidate t{.id=reinterpret_cast<std::uintptr_t>(actor),.name=*playerName,
              .origin={p->x,p->y-1.62f,p->z},.width=width,.height=height,
              .hurtTime=read<int>(actor,0x1b4),.player=true,.alive=alive(actor,api)};
  if(!playerEligible(c,t)) continue;
  const float distance=distanceToBox(eye,t.origin,width,height);
  if(distance<=rotationRange && distance<nearest) {
   nearest=distance; nearestRotation=look(lp,eye,t);
  }
  candidates.push_back(std::move(t)); actors.push_back(actor);
 }
 if(c.rotations) setRotation(nearestRotation);
 auto chosen=selectPlayers(c,eye,candidates);
 if(chosen.empty() || now<nextAttack_) return;
 nextAttack_=now+std::chrono::milliseconds(intervalMs(c.minKillAuraCPS,c.maxKillAuraCPS,random_));
 for(auto index:chosen) {
  if(c.emulateNativeAttack) {
   void* native=read<void*>(lp,0xca8);
   if(!native) continue;
   setRotation(look(lp,eye,candidates[index]));
   api.nativeBegin(native); api.nativeEnd(native);
  } else api.attack(mode,actors[index],true,&candidates[index].origin);
  if(c.swing) api.swing(lp,4,0);
 }
}
} // namespace aura
