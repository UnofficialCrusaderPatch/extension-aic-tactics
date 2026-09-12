#include "aic_tactics/runtime.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

unsigned int integrityDigest[2];

namespace {
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
}

// Observational, allocation-free checkpoint of the explicitly serialized words.
// This detects simulation divergence; package authentication uses SHA256 instead.
void __cdecl captureIntegrity(int legacyInterval)
{
    Digest digest = {2166136261U, 0x27D4EB2FU};
    digest.word(1); // aic-tactics-word-digest-v1
    digest.word(legacyInterval ? 1U : 0U);
    digest.word(*reinterpret_cast<const unsigned int*>(0x4D34B1));
    digest.word(static_cast<unsigned int>(legacyTargetPolicy));
    digest.block(configurations + 1, sizeof(CharacterConfiguration) * 16);
    digest.block(reinterpret_cast<const void*>(0x23FC8E8 + 676), 676 * 16);
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
    integrityDigest[0] = digest.first;
    integrityDigest[1] = digest.second;
}

} // namespace SHC141
} // namespace AicTactics
