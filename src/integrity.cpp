#include "aic_tactics/runtime.hpp"
#include "aic_tactics/patrols.hpp"
#include "aic_tactics/groups.hpp"
#include "aic_tactics/lord_hunt.hpp"
#include "aic_tactics/allies.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

unsigned int integrityDigest[2];

namespace {
// Observation only: never used by the simulation or serialized as game state.
// A fixed memcpy snapshot preserves the final observed replay boundary without
// running the word digest on every tick. The sink's exact size is checked below.
unsigned int boundaryWords[40960];
unsigned int boundaryCount;
struct Boundary {
    void word(unsigned int value) { boundaryWords[boundaryCount++] = value; }
    void block(const void* data, unsigned int bytes) {
        word(bytes);
        std::memcpy(boundaryWords + boundaryCount, data, bytes);
        boundaryCount += bytes / 4;
    }
};
struct Digest {
    unsigned int first;
    unsigned int second;
    void word(unsigned int value) {
        first = (first ^ value) * 16777619U;
        second = ((second << 7) | (second >> 25)) ^ value;
        second = second * 0x9E3779B1U + 0x85EBCA77U;
    }
    void block(const void* data, unsigned int bytes) {
        word(bytes);
        const unsigned char* values = static_cast<const unsigned char*>(data);
        for (unsigned int index = 0; index < bytes; index += 4) {
            unsigned int value;
            std::memcpy(&value, values + index, sizeof(value));
            word(value);
        }
    }
};
template<class Sink> void visitIntegrity(Sink& digest, int legacyInterval,
    unsigned int defenseTicks, int targetPolicy, const void* aicRecords)
{
    digest.word(1); // aic-tactics-word-digest-v1
    digest.word(legacyInterval ? 1U : 0U);
    digest.word(defenseTicks);
    digest.word(static_cast<unsigned int>(targetPolicy));
    digest.word(static_cast<unsigned int>(safePlacementFallback));
    digest.word(static_cast<unsigned int>(engineerRoleFallback));
    digest.word(static_cast<unsigned int>(siegePaymentFallback));
    digest.block(siegePaymentPolicy + 1, sizeof(int) * 16);
    digest.word(static_cast<unsigned int>(siegeHarassFallback));
    digest.word(static_cast<unsigned int>(siegeHarassMinimumFallback));
    digest.block(siegeHarassPolicy + 1, sizeof(int) * 16);
    digest.block(siegeHarassMinimum + 1, sizeof(int) * 16);
    digest.word(static_cast<unsigned int>(largeSiegeFallback));
    digest.word(static_cast<unsigned int>(siegeForceMaximumFallback));
    digest.block(largeSiegePolicy + 1, sizeof(int) * 16);
    digest.block(siegeForceMaximum + 1, sizeof(int) * 16);
    digest.block(patrolOrders + 1, sizeof(int) * 16);
    digest.block(patrolFormation + 1, sizeof(int) * 16);
    digest.block(patrolSpacing + 1, sizeof(int) * 16);
    digest.word(static_cast<unsigned int>(patrolOrdersFallback));
    digest.word(static_cast<unsigned int>(patrolFormationFallback));
    digest.word(static_cast<unsigned int>(patrolSpacingFallback));
    digest.block(lordHuntRetarget + 1, sizeof(int) * 16);
    digest.block(lordHuntSiege + 1, sizeof(int) * 16);
    digest.block(lordHuntSwitch + 1, sizeof(int) * 16);
    digest.block(lordHuntShare + 1, sizeof(int) * 16);
    digest.block(lordHuntSplit + 1, sizeof(int) * 16);
    digest.word(static_cast<unsigned int>(lordHuntRetargetFallback));
    digest.word(static_cast<unsigned int>(lordHuntSiegeFallback));
    digest.word(static_cast<unsigned int>(lordHuntSwitchFallback));
    digest.word(static_cast<unsigned int>(lordHuntShareFallback));
    digest.word(static_cast<unsigned int>(lordHuntSplitFallback));
    digest.block(configurations + 1, sizeof(CharacterConfiguration) * 16);
    digest.block(aicRecords, 676 * 16);
    digest.word(defenseCensusTick);
    digest.word(static_cast<unsigned int>(defenseCensusValid));
    digest.block(defenseTypeCounts, sizeof(defenseTypeCounts));
    digest.word(combatCensusTick);
    digest.word(static_cast<unsigned int>(combatCensusValid));
    digest.block(combatCensus, sizeof(combatCensus));
    digest.block(targetStates, sizeof(targetStates));
    digest.block(targetLifecycle, sizeof(targetLifecycle));
    digest.block(incidents, sizeof(incidents));
    digest.block(reserves, sizeof(reserves));
    digest.block(raidStates, sizeof(raidStates));
    digest.block(raidGroupCensus, sizeof(raidGroupCensus));
    digest.block(raidUnitPower, sizeof(raidUnitPower));
    digest.block(raidStaticDefenses, sizeof(raidStaticDefenses));
    digest.block(siegeHarassPlans, sizeof(siegeHarassPlans));
    digest.block(detachedGroups, sizeof(detachedGroups));
    digest.block(lordHunts, sizeof(lordHunts));
    digest.block(lordHuntCensus, sizeof(lordHuntCensus));
    digest.block(lordHuntUnits, sizeof(lordHuntUnits));
    digest.word(raidBuildingCensusTick);
    digest.word(static_cast<unsigned int>(raidBuildingCensusValid));
    digest.block(patrolExtraGroups + 1, sizeof(int) * 16);
    digest.block(patrolMinSize + 1, sizeof(int) * 16);
    digest.block(patrolRadius + 1, sizeof(int) * 16);
    digest.block(siegeScreen + 1, sizeof(int) * 16);
    digest.block(siegeScreenForward + 1, sizeof(int) * 16);
    digest.block(siegeScreenExtraGroups + 1, sizeof(int) * 16);
    digest.word(static_cast<unsigned int>(patrolExtraGroupsFallback));
    digest.word(static_cast<unsigned int>(patrolMinSizeFallback));
    digest.word(static_cast<unsigned int>(patrolRadiusFallback));
    digest.word(static_cast<unsigned int>(siegeScreenFallback));
    digest.word(static_cast<unsigned int>(siegeScreenForwardFallback));
    digest.word(static_cast<unsigned int>(siegeScreenExtraGroupsFallback));
    digest.block(patrolRoutes, sizeof(patrolRoutes));
    digest.block(allySettings + 1, sizeof(allySettings[0]) * 16);
    digest.block(allyFallback, sizeof(allyFallback));
    digest.block(allyStates, sizeof(allyStates));
}
typedef char BoundaryCapacity[(sizeof(CharacterConfiguration) * 16 + 676 * 16
    + sizeof(defenseTypeCounts) + sizeof(combatCensus) + sizeof(targetStates)
    + sizeof(targetLifecycle) + sizeof(incidents) + sizeof(reserves)
    + sizeof(raidStates) + sizeof(raidGroupCensus) + sizeof(raidUnitPower)
    + sizeof(raidStaticDefenses) + sizeof(siegeHarassPlans) + sizeof(detachedGroups)
    + sizeof(lordHunts) + sizeof(lordHuntCensus) + sizeof(lordHuntUnits)
    + sizeof(int) * 98 + sizeof(patrolOrders) * 3 + sizeof(lordHuntRetarget) * 5 + 128
    + sizeof(patrolOrders) * 6 + sizeof(int) * 12 + sizeof(patrolRoutes)
    + sizeof(allySettings) + sizeof(allyFallback) + sizeof(allyStates) + 12
    <= sizeof(boundaryWords)) ? 1 : -1];
}

// Observational, allocation-free checkpoint of the explicitly serialized words.
// This detects simulation divergence; package authentication uses SHA256 instead.
void __cdecl captureIntegrity(int legacyInterval, unsigned int defenseTicks, int targetPolicy, const void* aicRecords)
{
    Digest digest = {2166136261U, 0x27D4EB2FU};
    visitIntegrity(digest, legacyInterval, defenseTicks, targetPolicy, aicRecords);
    integrityDigest[0] = digest.first;
    integrityDigest[1] = digest.second;
}

void __cdecl observeIntegrityBoundary(int legacyInterval, unsigned int defenseTicks, int targetPolicy, const void* aicRecords)
{
    boundaryCount = 0;
    Boundary snapshot;
    visitIntegrity(snapshot, legacyInterval, defenseTicks, targetPolicy, aicRecords);
}

void __cdecl captureBoundaryIntegrity()
{
    Digest digest = {2166136261U, 0x27D4EB2FU};
    for (unsigned int index = 0; index < boundaryCount; ++index) digest.word(boundaryWords[index]);
    integrityDigest[0] = digest.first;
    integrityDigest[1] = digest.second;
}

} // namespace SHC141
} // namespace AicTactics
