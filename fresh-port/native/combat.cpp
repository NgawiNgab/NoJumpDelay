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
 void* component=api.component(actor,api.aliveKey);
 if(!component) return false;
 const float health=read<float>(component,0x7c);
 // Equivalent to a nonzero truncating integer conversion without float-to-int UB.
 return std::isfinite(health) && std::abs(health)>=1.f;
}
bool finite(Vec3 p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
std::optional<std::string> playerName(void* actor,const GameApi& api) {
 void* vector=read<void*>(actor,0x140);
 if(vector) {
  auto begin=read<std::uintptr_t>(vector,0), end=read<std::uintptr_t>(vector,8);
  if(begin && end>=begin && end-begin>0x20) {
   void* entry=read<void*>(reinterpret_cast<void*>(begin),0x20);
   if(entry && read<unsigned char>(entry,8)==4) {
    auto result=gameString(static_cast<unsigned char*>(entry)+0x10);
    if(result) return result;
   }
  }
 }
 return name(actor,api);
}
Rotation look(void* player, Vec3 eye, const Candidate& target) {
 const float half=target.width*.5f;
 const Vec3 p{std::clamp(eye.x,target.origin.x-half,target.origin.x+half),
              std::clamp(eye.y,target.origin.y,target.origin.y+target.height),
              std::clamp(eye.z,target.origin.z-half,target.origin.z+half)};
 const float dx=p.x-eye.x, dy=p.y-eye.y, dz=p.z-eye.z;
 const float horizontal=std::hypot(dx,dz);
 constexpr float degrees=57.295776f;
 const float pitch=horizontal<=0.001f ? (dy>0.f ? -90.f : 90.f) : -std::atan2(dy,horizontal)*degrees;
 return {player,true,-std::atan2(dx,dz)*degrees,pitch};
}
Vec3 jitter(Vec3 target,float width,float height,std::mt19937& rng) {
 std::uniform_real_distribution<float> uniform(0.f,1.f);
 const float x=2.f*uniform(rng)-1.f;
 const float y=uniform(rng);
 const float z=2.f*uniform(rng)-1.f;
 return {target.x+x*width,target.y+y*height,target.z+z*width};
}
void hit(void* result, Vec3 position) {
 const int type=1;
 std::memcpy(static_cast<unsigned char*>(result)+0x18,&type,sizeof(type));
 std::memcpy(static_cast<unsigned char*>(result)+0x2c,&position,sizeof(position));
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
 std::lock_guard lock(tickMutex_); nextAttack_={}; scheduled_=false; previousSwitch_=0; setRotation({});
}
void Combat::tick(void* lp, const Config& c, const GameApi& api, std::chrono::steady_clock::time_point now) {
 std::lock_guard lock(tickMutex_);
 setRotation({});
 if(!lp || (!c.killAura && !c.killItems)) return;
 auto* pos=read<Vec3*>(lp,0x228);
 void* level=read<void*>(lp,0x1f0);
 void* mode=read<void*>(lp,0xa08);
 if(!pos||!level||!mode||!alive(lp,api)||!finite(*pos)) return;
 void* hitResult=read<void*>(level,0x1d8);
 Vec3 eye=*pos;
 Vec3 feet=eye; feet.y-=1.62f;
 const auto localTeam=c.killAura ? team(lp) : std::optional<std::string>(std::string{});
 if(!localTeam) return;
 std::vector<Candidate> candidates;
 std::vector<void*> actors;
 float nearest=std::numeric_limits<float>::infinity();
 Rotation nearestRotation{};
 const float rotationRange=(c.rotations||c.emulateNativeAttack) ? c.rotationsRange : c.killRange;
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
      std::hypot(feet.x-p->x,feet.y-p->y,feet.z-p->z)<=6.f && hitResult &&
      std::isfinite(width) && width>=0 && std::isfinite(height) && height>=0) {
    hit(hitResult,jitter(*p,width,height,random_));
    api.attack(mode,actor,true,p);
    if(c.swing) api.swing(lp,4,0);
   }
   continue;
  }
  if(!c.killAura) continue;
  const auto theirTeam=team(actor);
  const auto actorName=playerName(actor,api);
  if(!theirTeam || !actorName || (!localTeam->empty() && *localTeam==*theirTeam)) continue;
  Candidate t{.id=reinterpret_cast<std::uintptr_t>(actor),.name=*actorName,
              .origin={p->x,p->y-1.62f,p->z},.width=width,.height=height,
              .hurtTime=read<int>(actor,0x1b4),.player=true,.alive=alive(actor,api)};
  if(!playerEligible(c,t)) continue;
  const float distance=distanceToBox(eye,t.origin,width,height);
  if(!(distance<=rotationRange)) continue;
  if(distance<nearest) {
   nearest=distance; nearestRotation=look(lp,eye,t);
  }
  candidates.push_back(std::move(t)); actors.push_back(actor);
 }
 if(c.rotations||c.emulateNativeAttack) setRotation(nearestRotation);
 auto chosen=selectPlayers(c,eye,candidates,previousSwitch_);
 if(chosen.empty() || (scheduled_ && now<nextAttack_)) return;
 scheduled_=true;
 nextAttack_=now+std::chrono::milliseconds(intervalMs(c.minKillAuraCPS,c.maxKillAuraCPS,random_));
 for(auto index:chosen) {
  if(c.emulateNativeAttack) {
   void* native=read<void*>(lp,0xca8);
   if(!native) continue;
   setRotation(look(lp,eye,candidates[index]));
   api.nativeBegin(native); api.nativeEnd(native);
  } else {
   if(!hitResult) continue;
   hit(hitResult,jitter(candidates[index].origin,candidates[index].width,candidates[index].height,random_));
   api.attack(mode,actors[index],true,&candidates[index].origin);
  }
  if(c.auraType==Mode::Switch) previousSwitch_=candidates[index].id;
  if(c.swing) api.swing(lp,4,0);
 }
}
} // namespace aura
