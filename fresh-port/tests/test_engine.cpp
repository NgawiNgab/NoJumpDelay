#include "engine.hpp"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace aura;
    assert(distanceToBox({0,0,0}, {0,1,0}, 2, 2) == 0);
    assert(std::abs(distanceToBox({3,0,0},{0,1,0},2,2)-2.f) < 1e-5);
    assert(std::abs(distanceToBox({3,4,0},{0,1,0},2,2)-std::sqrt(13.f)) < 1e-5);
    assert(std::isinf(distanceToBox({0,0,0},{0,0,0},-1,2)));
    Config c; c.killAura=true; c.killRange=4; c.hurtCheck=true; c.antiBot=true;
    Candidate a{.id=1,.name="enemy",.origin={2,1,0},.width=1,.height=2,.player=true};
    assert(playerEligible(c,a));
    a.hurtTime=2; assert(!playerEligible(c,a)); a.hurtTime=1;
    c.friendList={"enemy"}; assert(!playerEligible(c,a)); c.friendList.clear();
    c.botList={"enemy"}; assert(!playerEligible(c,a)); c.botList.clear();
    a.name="x"; assert(!playerEligible(c,a)); a.name="enemy"; a.width=.01f; assert(!playerEligible(c,a)); a.width=1;
    Candidate b=a; b.id=2; b.origin={3,1,0};
    Candidate far=a; far.id=3; far.origin={10,1,0};
    Candidate targets[]{b, a, far};
    auto selected=selectPlayers(c,{0,0,0},targets); assert(selected.size()==1 && selected[0]==1);
    c.auraType=Mode::Multi; selected=selectPlayers(c,{0,0,0},targets); assert(selected.size()==2);
    c.auraType=Mode::Switch;
    selected=selectPlayers(c,{0,0,0},targets); assert(selected.size()==1 && selected[0]==0);
    selected=selectPlayers(c,{0,0,0},targets,2); assert(selected.size()==1 && selected[0]==1);
    selected=selectPlayers(c,{0,0,0},targets,1); assert(selected.size()==1 && selected[0]==0);
    Candidate item{.id=4,.name="",.item=true}; c.killItems=true;
    assert(itemEligible(c,item,6.0f)); assert(!itemEligible(c,item,6.01f));
    assert(!itemEligible(c,item,std::numeric_limits<float>::quiet_NaN())); c.killItems=false;
    assert(!itemEligible(c,item,1.0f));
    std::mt19937 rng(42);
    for(int i=0;i<10000;i++){int x=intervalMs(5,10,rng); assert(x>=100 && x<=200);}
    assert(intervalMs(0,0,rng)==1000); assert(intervalMs(25,25,rng)==50);
    for(int i=0;i<1000;i++){int x=intervalMs(10,5,rng);assert(x>=100 && x<=200);}
}
