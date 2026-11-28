#include "combat.hpp"
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
using namespace aura;
namespace {
struct Entity {
 alignas(std::max_align_t) std::array<unsigned char,0xcb0> object{};
 alignas(std::max_align_t) std::array<unsigned char,0x90> nameComponent{};
 std::array<unsigned char,0x80> aliveComponent{};
 alignas(std::max_align_t) std::array<unsigned char,0x18> nameVector{};
 alignas(std::max_align_t) std::array<unsigned char,0x28> nameSlots{};
 alignas(std::max_align_t) std::array<unsigned char,0x40> nameEntry{};
 Vec3 position{};
 float box[8]{};
 bool player=true, hasAlive=true, hasName=true;
 std::string teamStorage, nameStorage, displayStorage;
 Entity(std::string name,Vec3 pos):position(pos) {
  setTeam(""); setName(std::move(name));
  float health=20.f; std::memcpy(aliveComponent.data()+0x7c,&health,4);
  box[6]=1; box[7]=2;
  set(0x228,&position); set(0x230,box);
  entities()[ptr()]=this;
 }
 ~Entity() {
  entities().erase(ptr());
 }
 template<class T> void set(std::size_t offset,T value) {std::memcpy(object.data()+offset,&value,sizeof(value));}
 void* ptr() {return object.data();}
 void encode(unsigned char* out,std::string& storage,std::string value) {
  storage=std::move(value); std::memset(out,0,24);
  if(storage.size()<=22) {out[0]=static_cast<unsigned char>(storage.size()*2);std::memcpy(out+1,storage.data(),storage.size());}
  else {out[0]=1;std::size_t size=storage.size();const char* ptr=storage.data();std::memcpy(out+8,&size,8);std::memcpy(out+16,&ptr,8);}
 }
 void setTeam(std::string value) {encode(object.data()+0xb28,teamStorage,std::move(value));}
 void setName(std::string value) {encode(nameComponent.data()+0x48,nameStorage,std::move(value));}
 void setDisplayName(std::string value) {
  encode(nameEntry.data()+0x10,displayStorage,std::move(value)); nameEntry[8]=4;
  void* entry=nameEntry.data(); std::memcpy(nameSlots.data()+0x20,&entry,8);
  void* begin=nameSlots.data(); void* end=nameSlots.data()+nameSlots.size();
  std::memcpy(nameVector.data(),&begin,8);std::memcpy(nameVector.data()+8,&end,8);
  set(0x140,nameVector.data());
 }
 void health(float v) {std::memcpy(aliveComponent.data()+0x7c,&v,4);}
 static std::unordered_map<void*,Entity*>& entities() {static std::unordered_map<void*,Entity*> map; return map;}
};
std::vector<void*> list, attacked;
int swings=0, begins=0, ends=0;
Vec3 lastPosition{};
std::vector<void*> getActors(void*) {return list;}
void* getComponent(void* a,const void*) {auto* e=Entity::entities().at(a);return e->hasAlive ? e->aliveComponent.data() : nullptr;}
bool isPlayer(void* a) {return Entity::entities().at(a)->player;}
void* getName(void* a) {auto* e=Entity::entities().at(a);return e->hasName ? e->nameComponent.data() : nullptr;}
void attack(void*,void* a,bool yes,const Vec3* p) {assert(yes);attacked.push_back(a);lastPosition=*p;}
void swing(void*,int i,int j) {assert(i==4 && j==0);++swings;}
void begin(void*) {++begins;}
void end(void*) {++ends;}
}
int main() {
 Entity lp("local",{0,1.62f,0}), near("near",{2,1.62f,0}), far("far",{3,1.62f,0}), item("3vr0cg5",{5,1.62f,0});
 std::array<unsigned char,0x1e0> level{};
 std::array<unsigned char,0x40> hitResult{};
 void* hit=hitResult.data();std::memcpy(level.data()+0x1d8,&hit,sizeof(hit));
 int mode=2, native=3;
 lp.set(0x1f0,level.data());lp.set(0xa08,&mode);lp.set(0xca8,&native);item.player=false;
 list={nullptr,lp.ptr(),far.ptr(),near.ptr(),item.ptr()};
 GameApi api{getActors,getComponent,isPlayer,getName,attack,swing,begin,end,reinterpret_cast<void*>(1)};
 Config c;c.killAura=true;c.killItems=true;c.rotations=true;c.killRange=4;c.rotationsRange=6;c.minKillAuraCPS=10;c.maxKillAuraCPS=10;
 Combat combat;auto now=std::chrono::steady_clock::time_point{}+std::chrono::seconds(1);
 combat.tick(lp.ptr(),c,api,now);
 assert(attacked.size()==2 && attacked[0]==item.ptr() && attacked[1]==near.ptr());
 assert(swings==2 && combat.rotation().locked && combat.rotation().player==lp.ptr());
 int hitType;Vec3 hitPoint;
 std::memcpy(&hitType,hitResult.data()+0x18,4);std::memcpy(&hitPoint,hitResult.data()+0x2c,sizeof(hitPoint));
 assert(hitType==1 && hitPoint.x>=1 && hitPoint.x<=3 && hitPoint.y>=0 && hitPoint.y<=2);
 assert(std::abs(combat.rotation().yaw+90.f)<.001f && std::abs(lastPosition.y)<.001f);
 attacked.clear();combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(50));
 assert(attacked.size()==1 && attacked[0]==item.ptr()); // item path is not CPS-gated
 c.killItems=false; attacked.clear();c.auraType=Mode::Multi;
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(100));
 assert(attacked.size()==2 && attacked[0]==far.ptr() && attacked[1]==near.ptr());
 attacked.clear();c.auraType=Mode::Single;lp.setTeam("red");near.setTeam("red");
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(200));
 assert(attacked.size()==1 && attacked[0]==far.ptr());
 attacked.clear();lp.setTeam("");near.setTeam("");c.friendList={"near"};far.health(0);
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(300));
 assert(attacked.empty() && !combat.rotation().locked);
 c.friendList.clear();far.health(20);lp.health(0);
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(400));assert(attacked.empty());
 lp.health(20);
 // Long libc++ strings use the pointer/size form rather than inline storage.
 near.setName("a-very-long-player-name-for-testing");
 c.friendList={"a-very-long-player-name-for-testing"};
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(450));
 assert(attacked.size()==1 && attacked[0]==far.ptr());
 attacked.clear();c.friendList.clear();near.setName("near");
 near.setDisplayName("display");c.friendList={"display"};
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(550));
 assert(attacked.size()==1 && attacked[0]==far.ptr());
 attacked.clear();c.friendList.clear();
 c.emulateNativeAttack=true;c.rotations=false;
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(650));
 assert(begins==1 && ends==1 && attacked.empty() && combat.rotation().locked);
 combat.reset();assert(!combat.rotation().locked);
 c.emulateNativeAttack=false;c.rotations=false;c.auraType=Mode::Switch;
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(700));assert(attacked.size()==1 && attacked[0]==far.ptr());
 attacked.clear();combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(800));assert(attacked.size()==1 && attacked[0]==near.ptr());
 attacked.clear();combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(900));assert(attacked.size()==1 && attacked[0]==far.ptr());
 attacked.clear();near.hasAlive=false;far.hasAlive=false;
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(1000));assert(attacked.empty());
 near.hasAlive=true;far.hasAlive=true;lp.hasAlive=false;
 combat.tick(lp.ptr(),c,api,now+std::chrono::milliseconds(1100));assert(attacked.empty());
 lp.hasAlive=true;combat.reset();c.killAura=false;c.killItems=true;
 item.position={0,6.f,0};hitResult.fill(0);
 combat.tick(lp.ptr(),c,api,now);assert(attacked.size()==1 && attacked[0]==item.ptr());
 std::memcpy(&hitType,hitResult.data()+0x18,4);std::memcpy(&hitPoint,hitResult.data()+0x2c,sizeof(hitPoint));
 assert(hitType==1 && hitPoint.x>=-1 && hitPoint.x<=1 && hitPoint.y>=6 && hitPoint.y<=8);
 attacked.clear();item.position.y=6.01f;combat.tick(lp.ptr(),c,api,now);assert(attacked.empty());
 c.killAura=true;c.killItems=false;c.rotations=true;c.rotationsRange=1;combat.reset();
 combat.tick(lp.ptr(),c,api,now);assert(attacked.empty() && !combat.rotation().locked);
 c.rotationsRange=10;far.hasAlive=false;near.position={0,6.f,0};combat.reset();
 combat.tick(lp.ptr(),c,api,std::chrono::steady_clock::time_point{});
 assert(attacked.size()==1 && attacked[0]==near.ptr() && combat.rotation().pitch==-90.f);
}
