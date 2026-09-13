#include "aic_tactics/runtime.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

unsigned int integrityDigest[2];

namespace {
// Observation only: never used by the simulation or serialized as game state.
// A fixed memcpy snapshot preserves the final observed replay boundary without
// running the word digest on every tick. The sink's exact size is checked below.
unsigned int boundaryWords[32768];
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
template<class Sink> void visitIntegrity(Sink& digest, int legacyInterval)
{
    digest.word(1); // aic-tactics-word-digest-v1
    digest.word(legacyInterval ? 1U : 0U);
    digest.word(*reinterpret_cast<const unsigned int*>(nativeBindings.initialDefenseTicks));
    digest.word(static_cast<unsigned int>(legacyTargetPolicy));
    digest.block(configurations + 1, sizeof(CharacterConfiguration) * 16);
    digest.block(reinterpret_cast<const void*>(nativeBindings.aicRecords), 676 * 16);
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
    digest.word(raidBuildingCensusTick);
    digest.word(static_cast<unsigned int>(raidBuildingCensusValid));
}
typedef char BoundaryCapacity[(sizeof(CharacterConfiguration) * 16 + 676 * 16
    + sizeof(defenseTypeCounts) + sizeof(combatCensus) + sizeof(targetStates)
    + sizeof(targetLifecycle) + sizeof(incidents) + sizeof(reserves)
    + sizeof(raidStates) + sizeof(raidGroupCensus) + sizeof(raidUnitPower)
    + sizeof(raidStaticDefenses) + 128 <= sizeof(boundaryWords)) ? 1 : -1];
}

// Observational, allocation-free checkpoint of the explicitly serialized words.
// This detects simulation divergence; package authentication uses SHA256 instead.
void __cdecl captureIntegrity(int legacyInterval)
{
    Digest digest = {2166136261U, 0x27D4EB2FU};
    visitIntegrity(digest, legacyInterval);
    integrityDigest[0] = digest.first;
    integrityDigest[1] = digest.second;
}

void __cdecl observeIntegrityBoundary(int legacyInterval)
{
    boundaryCount = 0;
    Boundary snapshot;
    visitIntegrity(snapshot, legacyInterval);
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
