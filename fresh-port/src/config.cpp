#include "config.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <system_error>

namespace aura {
using json=nlohmann::json;
namespace {
json encode(const Config& c) {
 return {{"killAura",c.killAura},{"killItems",c.killItems},{"antiBot",c.antiBot},
 {"killAuraShortcut",c.killAuraShortcut},{"killItemsShortcut",c.killItemsShortcut},
 {"rotations",c.rotations},{"swing",c.swing},{"hurtCheck",c.hurtCheck},
 {"emulateNativeAttack",c.emulateNativeAttack},{"auraType",static_cast<int>(c.auraType)},
 {"killRange",c.killRange},{"rotationsRange",c.rotationsRange},
 {"minKillAuraCPS",c.minKillAuraCPS},{"maxKillAuraCPS",c.maxKillAuraCPS},
 {"friendList",c.friendList},{"botList",c.botList}};
}
void decode(const json& j, Config& c) {
 if (!j.is_object()) throw std::invalid_argument("configuration must be an object");
 auto boolean=[&](const char* k,bool& v){ if(j.contains(k)) v=j.at(k).get<bool>(); };
 auto number=[&](const char* k,float& v){ if(j.contains(k)) { float n=j.at(k).get<float>(); if(!std::isfinite(n)) throw std::invalid_argument(k); v=std::clamp(n,1.0f,8.0f); } };
 auto integer=[&](const char* k,int& v){ if(j.contains(k)) v=std::clamp(j.at(k).get<int>(),1,20); };
 boolean("killAura",c.killAura); boolean("killItems",c.killItems); boolean("antiBot",c.antiBot);
 boolean("killAuraShortcut",c.killAuraShortcut); boolean("killItemsShortcut",c.killItemsShortcut);
 boolean("rotations",c.rotations); boolean("swing",c.swing); boolean("hurtCheck",c.hurtCheck);
 boolean("emulateNativeAttack",c.emulateNativeAttack);
 number("killRange",c.killRange); number("rotationsRange",c.rotationsRange);
 integer("minKillAuraCPS",c.minKillAuraCPS); integer("maxKillAuraCPS",c.maxKillAuraCPS);
 if (c.minKillAuraCPS>c.maxKillAuraCPS) std::swap(c.minKillAuraCPS,c.maxKillAuraCPS);
 if(j.contains("auraType")){ int n=j.at("auraType").get<int>(); if(n<0 || n>2) throw std::invalid_argument("auraType"); c.auraType=static_cast<Mode>(n); }
 if(j.contains("friendList")) c.friendList=j.at("friendList").get<std::vector<std::string>>();
 if(j.contains("botList")) c.botList=j.at("botList").get<std::vector<std::string>>();
}
}
bool loadConfig(const std::filesystem::path& path, Config& out) {
 try { std::ifstream in(path); if(!in) return false; json j=json::parse(in); Config next=out; decode(j,next); out=std::move(next); return true; } catch(...) {return false;}
}
bool saveConfig(const std::filesystem::path& path, const Config& c) {
 try {
  if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
  auto tmp=path; tmp += ".tmp";
  {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); if(!out) return false; out << encode(c).dump(2) << '\n'; out.flush(); if(!out) return false;}
  std::error_code ec; std::filesystem::rename(tmp,path,ec); return !ec;
 } catch(...) {return false;}
}
bool updateConfig(Config& c,std::string_view key,std::string_view value) {
 try {
  json j=encode(c); std::string k(key),v(value); if(!j.contains(k)) return false;
  if(k=="auraType") {
   if(v=="Single") j[k]=0; else if(v=="Multi") j[k]=1; else if(v=="Switch") j[k]=2; else return false;
  } else if(j[k].is_boolean()) {
   if(v=="true"||v=="1") j[k]=true; else if(v=="false"||v=="0") j[k]=false; else return false;
  } else if(j[k].is_array()) {
   j[k]=json::parse(v); if(!j[k].is_array()) return false;
  } else {
   j[k]=json::parse(v); if(!j[k].is_number()) return false;
  }
  Config next=c; decode(j,next); c=std::move(next); return true;
 } catch(...) { return false; }
}
} // namespace aura
