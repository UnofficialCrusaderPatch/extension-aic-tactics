#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "aic_tactics/shc141_groups.hpp"

using namespace AicTactics::SHC141;
static unsigned char tribeState[0x28 + 1250 * 0x334];
static unsigned char before[sizeof(tribeState)];
static int cases;

template<class T> static T& at(unsigned char* pointer)
{
    return *reinterpret_cast<T*>(pointer);
}

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "Tribe case %d: %s\n", cases, message);
        std::exit(1);
    }
}

static void check(int player, int mode)
{
    ++cases;
    std::memset(tribeState, 0, sizeof(tribeState));
    int expectedID = 0, free = 0;
    for (int id = 1250 - player; id > 0; id -= 8) {
        // Full own partition with other players' partitions empty is deliberate.
        const bool available = mode == 0 || (mode == 2 && id == 1250 - player - 24) ||
            (mode == 3 && id <= 8);
        at<short>(tribeState + 0x28 + id * 0x334 + 0x18) = available ? 0 : 2;
        if (available) {
            if (!expectedID) expectedID = id;
            ++free;
        }
    }
    std::memcpy(before, tribeState, sizeof(before));
    int* nextUID = reinterpret_cast<int*>(0x1FE7DD4);
    *nextUID = 321;
    TribeAvailability result;
    require(queryTribeAvailability(tribeState + 0x28, 1250, player, result), "valid query rejected");
    require(result.nextID == expectedID && result.freeSlots == free, "wrong partition admission");
    require(*nextUID == 321 && std::memcmp(before, tribeState, sizeof(before)) == 0, "query mutated state");
    typedef int (__thiscall *Create)(void*, int);
    const int created = reinterpret_cast<Create>(0x5227E0)(tribeState, player);
    require(created == result.nextID, "query differs from original allocator");
    if (created) {
        unsigned char* record = tribeState + 0x28 + created * 0x334;
        require(at<int>(record + 4) == player && at<int>(record + 0xC) == 321, "wrong native owner/UID");
        require(at<short>(record + 0x18) == 2 && *nextUID == 322, "wrong native allocation state");
        require(queryTribeAvailability(tribeState + 0x28, 1250, player, result), "post-allocation query failed");
        require(result.freeSlots == free - 1, "allocated slot remained free");
    } else {
        require(*nextUID == 321 && std::memcmp(before, tribeState, sizeof(before)) == 0,
            "failed native allocation changed state");
    }
}

void runGroupCases()
{
    for (int player = 1; player <= 8; ++player)
        for (int mode = 0; mode < 4; ++mode) check(player, mode);
    TribeAvailability result;
    require(!queryTribeAvailability(0, 1250, 1, result), "null array admitted");
    require(!queryTribeAvailability(tribeState + 0x28, 1250, 0, result), "player zero admitted");
    require(!queryTribeAvailability(tribeState + 0x28, 1250, 9, result), "player nine admitted");
    require(!queryTribeAvailability(tribeState + 0x28, 1251, 1, result), "unknown pool layout admitted");
    require(result.nextID == 0 && result.freeSlots == 0, "invalid query left stale result");
    std::printf("%d original-instruction tribe allocation comparisons and 4 input checks passed\n", cases);
}
