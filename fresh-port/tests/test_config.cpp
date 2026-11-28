#include "config.hpp"
#include <cassert>
#include <filesystem>
#include <cstdlib>
#include <fstream>
int main(){
 using namespace aura;
 const char* dir=std::getenv("KILLAURA_TEST_DIR");
 const auto path=std::filesystem::path(dir ? dir : ".") / "test-config.json";
 Config c; c.killAura=true; c.friendList={"One","Two"}; c.botList={"Bot"};
 assert(saveConfig(path,c)); Config d; assert(loadConfig(path,d)); assert(d.killAura && d.friendList==c.friendList && d.botList==c.botList);
 assert(updateConfig(d,"auraType","Multi") && d.auraType==Mode::Multi);
 assert(updateConfig(d,"friendList",R"(["Friends", "Two"])")); assert(d.friendList.size()==2);
 auto old=d.friendList; assert(!updateConfig(d,"friendList","not json") && d.friendList==old);
 assert(updateConfig(d,"killRange","100") && d.killRange==8.f);
 assert(!updateConfig(d,"antiBot","perhaps"));
 {std::ofstream out(path); out << "not json";} Config before=d; assert(!loadConfig(path,d)); assert(d.friendList==before.friendList && d.killAura==before.killAura);
 std::filesystem::remove(path);
}
