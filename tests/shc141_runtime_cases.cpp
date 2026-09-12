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
    at<int>(0xF98528) = 2;
    std::memset(reinterpret_cast<void*>(0xF98534+0x32C), 0, 0x32C);
    at<short>(0xF98534+0x32C+0xD0) = 2;
    at<short>(0xF98534+0x32C+0xD2) = 9;
    at<short>(0xF98534+0x32C+0xD6) = static_cast<short>(player);
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

void equipmentFixture(int type, int stock, int defense, int sortie) {
    fixture(1,4,1000,0);
    configurations[4].recruitment.conditionCount=1;
    configurations[4].recruitment.conditions[0].strength=-1;
    configurations[4].recruitment.conditions[0].requiredFacts=EquipmentSurplus;
    configurations[4].recruitment.conditions[0].weights.values[AttackRole]=100;
    for(int strength=0;strength<3;++strength) {
        configurations[4].baseRows[strength].values[SortieRole]=0;
        configurations[4].baseRows[strength].values[AttackRole]=100;
    }
    // Observe the fact without dispatching a hire into map/render services.
    at<int>(0x115E99C+0x39F4)=1;
    const unsigned int aic=0x23FC8E8+4*0x2A4;
    at<int>(aic+0x170)=defense;
    at<int>(aic+0x184)=type;
    at<int>(aic+0x154)=sortie;
    at<int>(aic+0x158)=type;
    at<int>(aic+0x288)=type;
    for(int resource=17;resource<=24;++resource) at<int>(0x115C2C8+0x39F4+resource*4)=stock;
    at<int>(0x1387F38)=2;
    at<short>(0x1387F38+0xD64-0x232)=1;
    at<short>(0x1387F38+0xD64-0x22A)=1;
    at<short>(0x1387F38+0xD64-0x234)=2;
    at<short>(0x1387F38+0xD64)=1;
}

void checkEquipment(bool expected, const char* message) {
    const int reason=at<int>(0x1387F38+0x60C);
    const int resource=at<int>(0x1387F38+0x610);
    opportunity(1);
    check(((observations[1].facts & EquipmentSurplus)!=0)==expected,message);
    check(observations[1].condition==(expected?0:-1),"equipment condition did not select the first matching row");
    check(at<int>(0x1387F38+0x60C)==reason && at<int>(0x1387F38+0x610)==resource,
        "equipment fact changed native acquisition diagnostics");
    check(observations[1].purchaseResource==0,"equipment fact ordered purchases");
}
}
void runRuntimeCases() {
    for(int type=22;type<=27;++type) {
        equipmentFixture(type,10,6,4);checkEquipment(false,"reserved home kit counted as surplus");
        equipmentFixture(type,11,6,4);checkEquipment(true,"complete surplus kit missing");
        at<int>(0x115C304+0x39F4)=0;checkEquipment(false,"unaffordable kit counted usable");
        at<int>(0x115C304+0x39F4)=1000;
        at<int>(0x1387F38)=1;checkEquipment(false,"kit without peasant counted usable");
        at<int>(0x1387F38)=2;
        at<short>(0xF98534+0x32C+0xD6)=2;checkEquipment(false,"foreign barracks counted usable");
    }
    equipmentFixture(25,11,6,4);
    at<int>(0x115C2C8+0x39F4+24*4)=10;
    checkEquipment(false,"pikes without spare armor counted surplus");
    equipmentFixture(22,11,0x7FFFFFFF,0x7FFFFFFF);
    checkEquipment(false,"huge home quota overflowed equipment reserve");
    equipmentFixture(70,100,0,0);
    checkEquipment(false,"unrelated equipment or gold-only roster counted surplus");
    equipmentFixture(28,11,0,0);
    checkEquipment(false,"knight kit without a horse counted usable");
    at<int>(0xF98528)=3;
    const unsigned int stable=0xF98534+2*0x32C+0xD0;
    at<short>(stable)=1;at<short>(stable+2)=0x23;at<short>(stable+6)=1;
    at<signed char>(stable+0x1C7)=1;at<signed char>(stable+0x1D7)=0;
    at<short>(0x115E04A+0x39F4)=99;
    checkEquipment(true,"free horse and full kit did not qualify");
    check(at<short>(0x115E04A+0x39F4)==99,"surplus probe changed the horse cache");
    at<int>(0x23FC8E8+4*0x2A4+0x170)=1;
    checkEquipment(false,"home defender's last horse counted surplus");
    equipmentFixture(22,1,10,0);
    const unsigned int equipmentAic=0x23FC8E8+4*0x2A4;
    at<int>(equipmentAic+0x188)=22;at<int>(equipmentAic+0x18C)=24;
    at<int>(0x115EEE0+0x39F4)=7;
    configurations[4].defenseComposition=PreserveSlots;
    resetDefenseCensus();
    for(int count=0;count<7;++count) countDefenseUnit(1,22);
    checkEquipment(true,"filled archer shares unnecessarily reserved surplus bows");
    invalidateDefenseCensus();
    checkEquipment(false,"stale composition census admitted surplus equipment");
    configurations[4].defenseComposition=NativeComposition;
    checkEquipment(false,"Native composition failed to reserve possible archer vacancies");
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
    // Grace gates only offensive recruitment while the native defense quota is open.
    for(int tickIndex=0;tickIndex<4;++tickIndex) {
        const unsigned int ticks[4]={0,4799,4800,0x80000000U};
        fixture(1,4,1000,17);
        configurations[4].initialDefenseTicks=4800;
        for(int strength=0;strength<3;++strength) {
            configurations[4].baseRows[strength].values[SortieRole]=0;
            configurations[4].baseRows[strength].values[AttackRole]=100;
        }
        at<int>(0x23FC8E8+4*0x2A4+0x288)=22;
        at<unsigned int>(0x1FE7DA8)=ticks[tickIndex];
        opportunity(1);
        check(observations[1].probeTypes[AttackRole]==(tickIndex<2?0:22),"grace end boundary or unsigned clock differs");
        check(observations[1].probeTypes[DefenseRole]==0,"grace overrode zero defense weight");
        check(observations[1].purchaseResource==(tickIndex<2?0:17),"grace ordered gated attack equipment");
        at<int>(0x115EEE0+0x39F4)=100;
        opportunity(1);
        check(observations[1].probeTypes[AttackRole]==22,"full defense did not release grace");
    }
    fixture(1,4,1000,17);
    configurations[4].initialDefenseTicks=24000;
    at<int>(0x1FE7DA8)=0;
    opportunity(1);
    check(observations[1].probeTypes[SortieRole]==22,"grace blocked sortie defense");
    fixture(1,4,0,17); opportunity(1);
    check(observations[1].probeReasons[SortieRole]==1,"gold restriction lost");
    check(observations[1].purchaseResource==0,"gold failure ordered equipment");
    fixture(1,4,1000,0); opportunity(1);
    check(observations[1].probeReasons[SortieRole]==3,"actual peasant restriction lost");
    fixture(1,4,1000,17);
    at<int>(0x115BF54+0x39F4)=0; opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-1,"missing building admitted");
    const unsigned int building=0xF98534+0x32C;
    for(int state=0;state<=3;state+=3) {
        fixture(1,4,1000,17);
        at<short>(building+0xD0)=static_cast<short>(state);
        opportunity(1);
        check(observations[1].probeReasons[SortieRole]==-1,"removed building admitted");
        check(observations[1].purchaseResource==0,"removed building ordered equipment");
    }
    fixture(1,4,1000,17);
    at<short>(building+0xD6)=2; opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-1,"foreign recruitment building admitted");
    at<short>(building+0xD6)=1;
    at<short>(building+0xD2)=8; opportunity(1);
    check(observations[1].probeReasons[SortieRole]==-1,"reused wrong-type building admitted");
    at<short>(building+0xD2)=9; opportunity(1);
    check(observations[1].probeReasons[SortieRole]==2,"restored recruitment building did not recover");
    const int invalidBuildings[3]={-1,2,2000};
    for(int index=0;index<3;++index) {
        fixture(1,4,1000,17);
        at<int>(0x115BF54+0x39F4)=invalidBuildings[index]; opportunity(1);
        check(observations[1].probeReasons[SortieRole]==-1,"out-of-range recruitment building admitted");
    }
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
    fixture(1,4,1000,17);
    for(int strength=0;strength<3;++strength) {
        configurations[4].baseRows[strength].values[SortieRole]=0;
        configurations[4].baseRows[strength].values[AttackRole]=100;
    }
    const unsigned int aic=0x23FC8E8+4*0x2A4;
    at<int>(aic+0x288)=22;
    at<int>(aic+0x23C)=1;
    at<int>(0x115F71C+0x39F4)=1;
    opportunity(1);
    check(observations[1].probeTypes[AttackRole]==30,
        "unavailable open subrole incorrectly enabled main-roster fallback above its quota");
    check(observations[1].purchaseResource==0,"unavailable subrole ordered fallback equipment");
    at<int>(aic+0x23C)=0;
    opportunity(1);
    check(observations[1].probeTypes[AttackRole]==22,
        "all subroles full lost original main-roster fallback");
    check(observations[1].purchaseResource==17,"native fallback failed to request its equipment");

    fixture(1,4,1000,19);
    configurations[4].defenseComposition=PreserveSlots;
    for(int strength=0;strength<3;++strength) {
        configurations[4].baseRows[strength].values[SortieRole]=0;
        configurations[4].baseRows[strength].values[DefenseRole]=100;
    }
    at<int>(aic+0x170)=10; at<int>(aic+0x180)=10;
    at<int>(aic+0x184)=22; at<int>(aic+0x188)=22; at<int>(aic+0x18C)=24;
    at<int>(0x115EEE0+0x39F4)=7;
    resetDefenseCensus();
    for(int count=0;count<7;++count) countDefenseUnit(1,22);
    countDefenseUnit(0,22); countDefenseUnit(9,22); countDefenseUnit(1,80);
    check(defenseTypeCounts[1][22]==7,"defense census ownership/bounds mismatch");
    opportunity(1);
    check(observations[1].probeTypes[DefenseRole]==24,"full repeated archer shares stole spear seats");
    check(observations[1].purchaseResource==19,"vacant spear share failed to request spears");
    countDefenseUnit(1,24); countDefenseUnit(1,24); countDefenseUnit(1,24);
    at<int>(0x115EEE0+0x39F4)=10;
    opportunity(1);
    check(observations[1].probeTypes[DefenseRole]==0,"filled composition recruited above total quota");
    at<int>(0x115EEE0+0x39F4)=7;
    resetDefenseCensus();
    for(int count=0;count<7;++count) countDefenseUnit(1,22);
    at<int>(0x1FE7DA8)+=2;
    opportunity(1);
    check(observations[1].probeTypes[DefenseRole]==0,"stale census admitted composition recruit");
    resetDefenseCensus(); // Native total and composition census intentionally disagree.
    opportunity(1);
    check(observations[1].probeTypes[DefenseRole]==0,"incomplete census admitted composition recruit");
    for(int count=0;count<7;++count) countDefenseUnit(1,22);
    invalidateDefenseCensus(); opportunity(1);
    check(observations[1].probeTypes[DefenseRole]==0,"invalidated load cache admitted composition recruit");
    configurations[4].baseRows[0].values[DefenseRole]=50;
    configurations[4].baseRows[0].values[SortieRole]=50;
    at<int>(0x115C2C8+0x39F4+17*4)=0;
    opportunity(1);
    check(observations[1].probeTypes[SortieRole]==22,"vacant defense shares stalled another role");
    std::printf("%d production runtime/original-instruction checks passed; no new running-game acceptance implied\n",cases);
}
