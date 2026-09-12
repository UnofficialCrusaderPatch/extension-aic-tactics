// Fixed native-layout fixtures; no map, renderer or desktop is involved.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <windows.h>
#include "aic_tactics/runtime.hpp"
using namespace AicTactics;
using namespace AicTactics::SHC141;
namespace {
const unsigned int Units=0x138854C, Tribes=0x1667F78, Stride=0x39F4;
int checks;
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) { std::fprintf(stderr,"Combat check %d: %s\n",checks,message); std::exit(1); }
}
void unit(int id, int player, int type) {
    const unsigned int address=Units+id*0x490;
    at<short>(address+0x8C)=2; at<short>(address+0x8E)=static_cast<short>(type);
    at<short>(address+0x96)=static_cast<short>(player); at<int>(address+0x98)=id+100;
    at<int>(address+0x3C8)=100; at<short>(address+0xC4)=40; at<short>(address+0xC6)=70;
}
void fixture() {
    std::memset(configurations,0,sizeof(configurations));
    std::memset(incidents,0,sizeof(incidents)); std::memset(targetStates,0,sizeof(targetStates));
    std::memset(targetLifecycle,0,sizeof(targetLifecycle)); std::memset(reserves,0,sizeof(reserves));
    std::memset(raidStates,0,sizeof(raidStates));
    std::memset(reinterpret_cast<void*>(Units),0,2500*0x490);
    std::memset(reinterpret_cast<void*>(Tribes),0,0x28+1250*0x334);
    std::memset(reinterpret_cast<void*>(0x115BDF8),0,9*Stride);
    std::memset(reinterpret_cast<void*>(0x23FC8E8+676),0,16*676);
    at<unsigned int>(0x1FE7DA8)=1000;
    for (int player=1;player<=8;++player) {
        at<int>(0x117D548+player*4)=player;
        at<int>(0x115E0F8+player*Stride)=player+1;
        at<int>(0x115DF78+player*Stride)=100-player;
        at<int>(0x115F6E0+player*Stride)=100+player;
        unit(player,player,55);
    }
}
void census() {
    resetCombatCensus();
    for (int id=1;id<32;++id) countCombatUnit(id);
    completeCombatCensus();
}
}
void runCombatCases() {
    fixture(); unit(10,2,22); unit(11,3,5); unit(12,3,53);
    at<short>(Units+11*0x490+0x2A0)=1;
    configurations[1].combat.targeting.policy=LowestPopulation;
    configurations[1].preparation=PrepareDuringAttack;
    census();
    check(combatCensus[2].troops==1 && combatCensus[3].troops==0,"military census included dying/utility units");
    TargetContext context;
    check(readTargetContext(1,context) && !context.players[1].eligible && context.players[8].eligible,"enemy roster differs");
    const unsigned int rng=at<unsigned int>(0x1A279C0);
    check(commitOpponent(1)==1 && targetStates[1].player==8,"population policy did not choose native metric");
    check(reserves[1].deployed==1 && reserves[1].returning==0,"launch did not start private preparation lifecycle");
    at<int>(0x115DF78+2*Stride)=0;
    check(commitOpponent(1)==1 && targetStates[1].player==8,"active army changed target");
    check(at<unsigned int>(0x1A279C0)==rng,"nonrandom policy advanced RNG");
    at<int>(0x117D548+8*4)=1;
    check(commitOpponent(1)==0 && targetStates[1].player==8,"alliance change skipped cleanup");
    targetLifecycle[1]|=2;
    check(commitOpponent(1)==1 && targetStates[1].player==2,"completed cleanup did not release target");
    at<int>(Units+2*0x490+0x98)=999;
    check(commitOpponent(1)==0,"recycled lord UID remained eligible");
    at<unsigned int>(0x1FE7DA8)=1002;
    check(!readTargetContext(1,context),"stale census admitted target selection");

    fixture(); configurations[1].combat.activation=AfterProvocation;
    check(!offensiveActionsAllowed(1) && offensiveActionsAllowed(2),"dormancy leaked between personalities");
    incidents[1].awake=1;
    check(offensiveActionsAllowed(1),"qualified wake did not allow offense");
    configurations[1].combat.activation=ImmediateAttack;
    configurations[1].preparation=PrepareDuringAttack;
    const int group=1249, id=10;
    const unsigned int tribe=Tribes+group*0x334;
    unit(id,1,22); at<short>(Units+id*0x490+0x42A)=20;
    at<short>(Units+id*0x490+0x2A4)=1;
    at<short>(Units+id*0x490+0x2D8)=group; at<unsigned int>(Units+id*0x490+0x2E4)=77;
    at<int>(tribe+0x2C)=1; at<unsigned int>(tribe+0x34)=77;
    at<short>(tribe+0x40)=2; at<short>(tribe+0x5C)=1;
    at<unsigned short>(tribe+0x60)=1U<<id;
    reserves[1].groups[14].id=group; reserves[1].groups[14].uid=77;
    at<int>(0x115E9A0+1*Stride+10*4)=1; at<int>(0x115EEE8+Stride)=1;
    census();
    check(isReserveUnit(id)==1,"private group membership lost");
    check(at<int>(0x115EEE8+Stride)==0 && at<int>(0x115E9A0+Stride+40)==0,"reserve counted in active wave");
    check(reserveRoleAvailable(reinterpret_cast<void*>(0x23FC8E8),1,7),"reserve blocked sortie role");
    reserves[1].returning=1;
    check(!reserveRoleAvailable(reinterpret_cast<void*>(0x23FC8E8),1,20),"active wave bought before reserve transfer");
    at<short>(tribe+0x40)=3;
    check(!isReserveUnit(id),"retiring group remained a live reserve");
    at<short>(tribe+0x40)=2;
    const unsigned int aic=0x23FC8E8+676;
    at<int>(aic+0x1F4)=20; at<int>(aic+0x288)=22;
    at<int>(aic+0x298)=20; at<int>(aic+0x29C)=1;
    at<int>(0x1FE7DD4)=90;
    prepareArmyUpdate(reinterpret_cast<void*>(0x23FC8E8),1);
    const int active=at<short>(0x115EF04+Stride+192*2);
    check(active==1241,"reserve transfer did not allocate through the native player partition");
    check(at<short>(Units+id*0x490+0x2D8)==active && at<unsigned int>(Units+id*0x490+0x2E4)==90,
        "native transfer did not update the unit's group identity");
    check(at<short>(tribe+0x5C)==0 && at<short>(tribe+0x40)==3
        && at<short>(Tribes+active*0x334+0x5C)==1,"native add/remove did not retire the empty reserve");
    check(at<int>(0x115EEE8+Stride)==1 && at<int>(0x115E9A0+Stride+40)==1,
        "reserve handover did not update the native readiness counters");

    fixture(); configurations[1].raids.policy=NearestReachableRaid;
    configurations[1].raids.groups=4; configurations[1].raids.minimumSize=4;
    configurations[1].raids.risk=MediumRaidRisk;
    raidStates[1].groups[0].tribe.id=group; raidStates[1].groups[0].tribe.uid=77;
    at<int>(tribe+0x2C)=1; at<unsigned int>(tribe+0x34)=77; at<short>(tribe+0x40)=2;
    unit(id,1,22); at<short>(Units+id*0x490+0x2D8)=group;
    at<unsigned int>(Units+id*0x490+0x2E4)=77;
    resetRaidUnitCensus(); countRaidUnit(id,100);
    check(raidGroupCensus[1][0].combatants==1 && raidGroupCensus[1][0].power==100,"raid group census differs");
    check(raidUnitPower[1][4*32+2]==100,"raid exposure grid differs");
    at<unsigned int>(Units+id*0x490+0x2E4)=78; countRaidUnit(id,100);
    check(raidGroupCensus[1][0].combatants==1,"recycled raid UID counted");
    captureIntegrity(0); const unsigned int first=integrityDigest[0], second=integrityDigest[1];
    captureIntegrity(0);
    check(first==integrityDigest[0] && second==integrityDigest[1],"observation changed digest");
    observeIntegrityBoundary(0); captureBoundaryIntegrity();
    check(first==integrityDigest[0] && second==integrityDigest[1],"boundary snapshot digest differs from live state");
    raidStates[1].retargetPending=15; captureIntegrity(0);
    check(first!=integrityDigest[0] || second!=integrityDigest[1],"pending raid decision missing from digest");
    captureBoundaryIntegrity();
    check(first==integrityDigest[0] && second==integrityDigest[1],"ending observation changed with the live world");
    for (int player=1;player<=8;++player) {
        fixture(); configurations[player].preparation=PrepareDuringAttack;
        const int source=1250-player;
        const unsigned int sourceAddress=Tribes+source*0x334;
        at<int>(sourceAddress+0x2C)=player; at<unsigned int>(sourceAddress+0x34)=77;
        at<short>(sourceAddress+0x40)=2; at<short>(sourceAddress+0x5C)=17;
        reserves[player].groups[14].id=source; reserves[player].groups[14].uid=77;
        reserves[player].returning=1;
        for (int member=10;member<27;++member) {
            unit(member,player,22); const unsigned int address=Units+member*0x490;
            at<short>(address+0x42A)=20; at<short>(address+0x2D8)=static_cast<short>(source);
            at<unsigned int>(address+0x2E4)=77;
            at<unsigned short>(sourceAddress+0x60+(member/16)*2)|=static_cast<unsigned short>(1U<<(member%16));
        }
        resetReserveCensus();
        const unsigned int personality=0x23FC8E8+player*676;
        at<int>(personality+0x1F4)=20; at<int>(personality+0x288)=22;
        at<int>(personality+0x298)=20; at<int>(personality+0x29C)=1;
        at<int>(0x1FE7DD4)=90;
        prepareArmyUpdate(reinterpret_cast<void*>(0x23FC8E8),player);
        const int destination=at<short>(0x115EF04+player*Stride+192*2);
        check(destination==source-8 && at<short>(Tribes+destination*0x334+0x5C)==16,
            "bounded handover moved more or fewer than sixteen units");
        check(at<short>(sourceAddress+0x5C)==1 && at<int>(0x115EEE8+player*Stride)==16,
            "bounded handover lost its remaining reserve");
        const unsigned int destinationUID=at<unsigned int>(Tribes+destination*0x334+0x34);
        prepareArmyUpdate(reinterpret_cast<void*>(0x23FC8E8),player);
        check(at<short>(sourceAddress+0x40)==3 && at<short>(Tribes+destination*0x334+0x5C)==17
            && at<int>(0x115EEE8+player*Stride)==17,"saved cursor failed to resume the remaining unit");
        check(at<unsigned int>(Tribes+destination*0x334+0x34)==destinationUID,
            "continued handover replaced its active army group");
    }
    std::printf("%d combat/reserve/raid native-layout checks passed; no running-game acceptance\n",checks);
}

void runIntegrityBenchmark()
{
    LARGE_INTEGER frequency,begin,end;
    check(QueryPerformanceFrequency(&frequency)!=0,"performance counter unavailable");
    const int Samples=1001, Batch=32;
    double elapsed[3][Samples];
    volatile unsigned int baseline=0;
    // Nonzero deterministic bytes prevent a zero-state-only benchmark. No game
    // routine consumes this synthetic observation fixture.
    std::memset(raidUnitPower,0x35,sizeof(raidUnitPower));
    std::memset(raidStaticDefenses,0x72,sizeof(raidStaticDefenses));
    for (int warm=0;warm<100;++warm) observeIntegrityBoundary(0);
    for (int sample=0;sample<Samples;++sample) {
        for (int step=0;step<3;++step) {
            const int kind=(sample+step)%3;
            QueryPerformanceCounter(&begin);
            for (int repeat=0;repeat<Batch;++repeat) {
                if (kind==0) ++baseline;
                else if (kind==1) observeIntegrityBoundary(0);
                else captureIntegrity(0);
            }
            QueryPerformanceCounter(&end);
            elapsed[kind][sample]=static_cast<double>(end.QuadPart-begin.QuadPart)
                * 1000000.0 / static_cast<double>(frequency.QuadPart) / Batch;
        }
    }
    const char* names[]={"empty_loop","boundary_copy","live_digest"};
    std::printf("{\"scope\":\"Cache-warm native component timing; excludes Lua, recorder, simulation and full-match overhead\",\"samples\":%d,\"batch\":%d,\"microseconds\":{",Samples,Batch);
    for (int kind=0;kind<3;++kind) {
        std::sort(elapsed[kind],elapsed[kind]+Samples);
        std::printf("%s\"%s\":{\"median\":%.6f,\"p95\":%.6f,\"p99\":%.6f,\"max\":%.6f}",
            kind ? "," : "",names[kind],elapsed[kind][500],elapsed[kind][950],elapsed[kind][990],elapsed[kind][1000]);
    }
    std::printf("}}\n");
}
