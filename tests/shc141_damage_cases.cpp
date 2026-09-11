#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "aic_tactics/shc141_damage.hpp"

using namespace AicTactics::SHC141;
static const unsigned int ImageBase = 0x400000, ImageSize = 0x2091000;
static const unsigned int UnitArray = 0x1387F38 + 0x614;
static int nativeCases, observationCases;

template<class T> static T& at(unsigned int address)
{
    return *reinterpret_cast<T*>(address);
}

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "Damage case %d/%d: %s\n", nativeCases, observationCases, message);
        std::exit(1);
    }
}

static DamageMemory fixture(int responsible, int health, int type, bool allied, bool dying)
{
    std::memset(reinterpret_cast<void*>(UnitArray), 0, 3 * 0x490);
    at<int>(0x1387F38) = 3;
    for (int id = 1; id <= 2; ++id) {
        const unsigned int address = UnitArray + id * 0x490;
        at<short>(address + 0x8C) = 2;
        at<short>(address + 0x8E) = static_cast<short>(id == 1 ? 22 : type);
        at<short>(address + 0x96) = static_cast<short>(id == 1 ? responsible : 1);
        at<int>(address + 0x98) = id * 111;
        at<short>(address + 0xC4) = 40;
        at<short>(address + 0xC6) = 70;
        at<short>(address + 0x2A0) = static_cast<short>(id == 2 && dying ? 1 : 0);
        at<unsigned char>(address + 0x32C) = 1;
        at<int>(address + 0x3C8) = health;
        at<int>(address + 0x3CC) = 500;
        at<short>(address + 0x400) = 7;
    }
    for (int player = 0; player <= 8; ++player) {
        at<int>(0x117D548 + player * 4) = allied ? 1 : player;
        at<int>(0x115F6F8 + player * 0x39F4) = 0;
    }
    at<int>(0x191DD80) = 0;
    at<int>(0xB4EE60 + (22 * 80 + type) * 4) = 50;
    at<int>(0xB4EBA0 + type * 4) = 50;
    at<int>(0xB4EBE0 + type * 4) = 50;
    at<int>(0xB4ED20 + type * 4) = 50;
    const unsigned int entity = 0x2350314 + 0xE8;
    std::memset(reinterpret_cast<void*>(entity), 0, 0xE8);
    at<short>(entity + 0x2A) = 1;
    at<short>(entity + 0x2C) = static_cast<short>(responsible);
    at<int>(entity + 0x30) = 333;
    at<short>(entity + 0xA2) = 1;
    DamageMemory memory = {reinterpret_cast<const unsigned char*>(UnitArray), 3,
        reinterpret_cast<const unsigned char*>(0x2350314), 2,
        reinterpret_cast<const int*>(0x117D548)};
    return memory;
}

static int nativeDamage(DamageSource source, int responsible, int half)
{
    typedef int (__thiscall *Two)(void*, int, int);
    typedef int (__thiscall *Three)(void*, int, int, int);
    void* units = reinterpret_cast<void*>(0x1387F38);
    if (source == UnitDamage)
        return reinterpret_cast<Two>(0x531220)(units, 1, 2);
    if (source == EntityDamage)
        return reinterpret_cast<Three>(0x531920)(units, 2, 1, half);
    return reinterpret_cast<Three>(0x532460)(units, 2, responsible, half);
}

static void check(unsigned char* baseline, unsigned char* before, DamageSource source, int responsible,
    int health = 500, int type = 22, bool allied = false, bool dying = false, int half = 0)
{
    ++nativeCases;
    const DamageMemory memory = fixture(responsible, health, type, allied, dying);
    std::memcpy(before, reinterpret_cast<void*>(ImageBase), ImageSize);
    const int expectedReturn = nativeDamage(source, responsible, half);
    const int remaining = at<int>(UnitArray + 2 * 0x490 + 0x3C8);
    std::memcpy(baseline, reinterpret_cast<void*>(ImageBase), ImageSize);
    // The test executable reserves this entire range and places its own code
    // after it. Restore every original global, including incidental native
    // effect queues, before comparing the observed invocation with baseline.
    std::memcpy(reinterpret_cast<void*>(ImageBase + 0x1000), before + 0x1000, ImageSize - 0x1000);
    const int first = source == UnitDamage ? 1 : 2;
    const int second = source == UnitDamage ? 2 : source == EntityDamage ? 1 : responsible;
    DamageProbe probe;
    const bool eligible = responsible >= 2 && responsible <= 8 && !allied && !dying && health > 0;
    require(beginDamage(memory, source, first, second, probe) == eligible, "wrong provenance admission");
    require(nativeDamage(source, responsible, half) == expectedReturn, "changed original return value");
    DamageObservation result;
    const bool observed = finishDamage(memory, probe, result);
    require(observed == (eligible && remaining < health), "wrong damage observation");
    if (observed) {
        require(result.sourceOwner == responsible && result.victimOwner == 1, "wrong responsible owner");
        require(result.victimID == 2 && result.victimUID == 222 && result.unitType == type, "wrong victim identity");
        require(result.healthLost == health - remaining && result.killed == (remaining == 0), "wrong health/death delta");
        require(result.x == 40 && result.y == 70, "wrong location");
    }
    require(!finishDamage(memory, probe, result), "one native call emitted twice");
    if (std::memcmp(baseline, reinterpret_cast<void*>(ImageBase), ImageSize) != 0) {
        int shown = 0;
        for (unsigned int offset = 0; offset < ImageSize && shown < 12; ++offset)
            if (baseline[offset] != at<unsigned char>(ImageBase + offset)) {
                std::fprintf(stderr, "Native difference at %X: %u -> %u\n", ImageBase + offset,
                    baseline[offset], at<unsigned char>(ImageBase + offset));
                ++shown;
            }
    }
    require(std::memcmp(baseline, reinterpret_cast<void*>(ImageBase), ImageSize) == 0,
        "observation changed native memory, RNG or damage outcome");
}

static void identityChecks()
{
    const unsigned int victim = UnitArray + 2 * 0x490;
    for (int change = 0; change < 6; ++change) {
        ++observationCases;
        DamageMemory memory = fixture(2, 500, 22, false, false);
        DamageProbe probe;
        require(beginDamage(memory, FireDamage, 2, 2, probe), "fixture admission failed");
        at<int>(victim + 0x3C8) = 400;
        if (change == 0) at<int>(victim + 0x98) = 444;
        if (change == 1) at<short>(victim + 0x96) = 3;
        if (change == 2) at<int>(0x117D548 + 2 * 4) = 1;
        if (change == 3) at<int>(victim + 0x3C8) = 500;
        if (change == 4) at<int>(victim + 0x3C8) = 600;
        if (change == 5) memory.unitCapacity = 2;
        DamageObservation result;
        require(!finishDamage(memory, probe, result), "stale identity, ally or healing counted as damage");
        require(!probe.active, "finished probe remained active");
    }
    for (int change = 0; change < 7; ++change) {
        ++observationCases;
        DamageMemory memory = fixture(2, 500, 22, false, false);
        if (change == 0) memory.units = 0;
        if (change == 1) memory.unitCapacity = 2501;
        if (change == 2) memory.unitCapacity = 2;
        if (change == 3) memory.teams = 0;
        if (change == 4) memory.entities = 0;
        if (change == 5) memory.entityCapacity = 1;
        if (change == 6) memory.entityCapacity = 3001;
        DamageProbe probe;
        require(!beginDamage(memory, EntityDamage, 2, 1, probe), "unsupported memory admitted");
    }
}

void runDamageCases()
{
    unsigned char* baseline = static_cast<unsigned char*>(std::malloc(ImageSize));
    unsigned char* before = static_cast<unsigned char*>(std::malloc(ImageSize));
    require(baseline != 0 && before != 0, "cannot allocate comparison");
    for (int source = UnitDamage; source <= FireDamage; ++source) {
        for (int owner = 0; owner <= 8; ++owner)
            check(baseline, before, static_cast<DamageSource>(source), owner);
        check(baseline, before, static_cast<DamageSource>(source), 2, 500, 22, true);
    }
    check(baseline, before, FireDamage, 2, 500, 22, false, true);
    check(baseline, before, FireDamage, 2, 500, 22, false, false, 1);
    check(baseline, before, FireDamage, 2, 30);
    check(baseline, before, FireDamage, 2, 20, 55);
    check(baseline, before, FireDamage, 0, 20, 55);
    check(baseline, before, FireDamage, 2, 500, 53);
    check(baseline, before, FireDamage, 2, 500, 76);
    identityChecks();
    std::free(baseline);
    std::free(before);
    std::printf("%d original-instruction damage cases and %d observation validation cases passed\n",
        nativeCases, observationCases);
}
