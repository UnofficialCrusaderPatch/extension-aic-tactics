// Executes the production adapter against original acquisition instructions.
// Deliberately unavailable recruits keep this fixture out of map/render services.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "aic_tactics/runtime.hpp"

using namespace AicTactics;
using namespace AicTactics::SHC141;
namespace {
int cases;
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
void check(bool value, const char* message) {
    ++cases;
    if (!value) { std::fprintf(stderr, "Runtime case %d: %s\n", cases, message); std::exit(1); }
}
void fixture(int player, int character, int gold, int missing) {
    std::memset(configurations, 0, sizeof(configurations));
    std::memset(observations, 0, sizeof(observations));
    std::memset(reinterpret_cast<void*>(0x115BDF8), 0, 9*0x39F4);
    std::memset(reinterpret_cast<void*>(0x1667F78), 0, 0x28+1250*0x334);
    std::memset(reinterpret_cast<void*>(0x1387F38), 0, 0x10000);
    at<int>(0x191DD80) = 0;
    at<int>(0x1FE7D78) = 0;
    at<int>(0x1FE7DA8) = 10000;
    at<int>(0x1387F38) = 1; // No idle unit record, even when UI count is nonzero.
    unsigned int playerBase = player * 0x39F4;
    at<int>(playerBase+0x115E0F8) = character+1;
    at<int>(playerBase+0x115E964) = 1;
    at<int>(playerBase+0x115F76C) = 1;
    at<int>(playerBase+0x115BF54) = 1;
    for (int resource=1; resource<=24; ++resource)
        at<int>(playerBase+0x115C2C8+4*resource) = resource==missing ? 0 : 10;
    at<int>(playerBase+0x115C304) = gold;
    unsigned int record = 0x23FC8E8 + character*0x2A4;
    std::memset(reinterpret_cast<void*>(record), 0, 0x2A4);
    at<int>(record+0x9C) = 4;
    at<int>(record+0x154) = 8;
    at<int>(record+0x158) = 22;
    at<int>(record+0x170) = 100;
    at<int>(record+0x184) = 22;
    configurations[character].recruitment.mode = WeightedRoles;
    for (int strength=0; strength<3; ++strength)
        configurations[character].baseRows[strength].values[SortieRole] = 100;
}
void opportunity(int player) {
    const unsigned int rng = at<unsigned int>(0x1A279C0);
    check(recruitOpportunity(reinterpret_cast<void*>(0x23FC8E8),player,1)==1,"opt-in was not handled");
    check(at<unsigned int>(0x1A279C0)==rng,"unavailable recruit consumed RNG");
    for(int role=0;role<4;++role) check(observations[player].hires[role]==0,"unavailable recruit hired");
}
}
void runRuntimeCases() {
    for(int player=1;player<=8;++player) {
        for(int character=1;character<=16;++character) {
            fixture(player,character,1000,17);
            opportunity(player);
            check(observations[player].character==character,"loader/native character mapping differs");
            check(observations[player].probeReasons[SortieRole]==2,"missing bow not reported");
            check(at<int>(player*0x39F4+0x115E868+17*4)==4,"native purchase queue missing");
            check(observations[player].probeTypes[DefenseRole]==0,"zero-weight defense probed");
            configurations[character].recruitment.mode=NativeRecruitment;
            const unsigned int sequence=observations[player].sequence;
            check(recruitOpportunity(reinterpret_cast<void*>(0x23FC8E8),player,1)==0,"Native path not preserved");
            check(observations[player].sequence==sequence,"Native personality evaluated policy");
        }
    }
    fixture(1,4,0,17); opportunity(1);
    check(observations[1].probeReasons[SortieRole]==1,"gold restriction lost");
    check(observations[1].purchaseResource==0,"gold failure ordered equipment");
    fixture(1,4,1000,0); opportunity(1);
    check(observations[1].probeReasons[SortieRole]==3,"actual peasant restriction lost");
    fixture(1,4,1000,17);
    at<int>(0x115BF54+0x39F4)=0; opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-1,"missing building admitted");
    fixture(1,4,1000,17);
    at<int>(0x115F73C+0x39F4)=8; opportunity(1);
    check(observations[1].purchaseResource==0,"filled quota ordered equipment");
    fixture(1,4,1000,17);
    RecruitmentPolicy& policy=configurations[4].recruitment;
    policy.conditionCount=2;
    for(int i=0;i<2;++i) {
        policy.conditions[i].strength=-1;
        policy.conditions[i].requiredFacts=DefenseIncomplete;
        policy.conditions[i].weights.values[i==0?DefenseRole:SortieRole]=100;
    }
    opportunity(1);
    check(observations[1].condition==0,"first matching row not selected");
    check(observations[1].probeTypes[DefenseRole]==22 && observations[1].probeTypes[SortieRole]==0,
        "conditional zero weight still probed");
    fixture(1,4,1000,0);
    for(int group=1249;group>0;group-=8) at<short>(0x1667F78+0x40+group*0x334)=2;
    opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-3,"full own partition admitted new group");
    const unsigned int slot=0x115EF04+0x39F4+167*2;
    at<short>(slot)=1249;
    at<unsigned int>(0x115F094+0x39F4+167*4)=123;
    at<unsigned int>(0x1667F78+0x34+1249*0x334)=123;
    at<int>(0x1667F78+0x2C+1249*0x334)=1;
    opportunity(1);
    check(observations[1].probeReasons[SortieRole]==3,"existing group could not accept with full partition");
    at<unsigned int>(0x1667F78+0x34+1249*0x334)=124;
    opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-3,"stale group UID admitted");
    at<unsigned int>(0x1667F78+0x34+1249*0x334)=123;
    at<int>(0x1667F78+0x2C+1249*0x334)=2;
    opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-3,"foreign group owner admitted");
    std::printf("%d production runtime/original-instruction checks passed; no new running-game acceptance implied\n",cases);
}
