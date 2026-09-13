#ifndef AIC_TACTICS_SHC141_DAMAGE_HPP
#define AIC_TACTICS_SHC141_DAMAGE_HPP

namespace AicTactics {
namespace SHC141 {

enum DamageSource { UnitDamage, EntityDamage, FireDamage };

struct DamageMemory {
    const unsigned char* units; // Native Unit array, including index 0.
    int unitCapacity;
    const unsigned char* entities; // Native Entity array, including index 0.
    int entityCapacity;
    const int* teams; // Player IDs 0..8, from the existing diplomacy owner.
};

struct DamageProbe {
    bool active;
    int victimID;
    int victimUID;
    int victimOwner;
    int sourceOwner;
    int unitType;
    int health;
    int x;
    int y;
};

struct DamageObservation {
    int victimID;
    int victimUID;
    int victimOwner;
    int sourceOwner;
    int unitType;
    int healthLost;
    bool killed;
    int x;
    int y;
};

// Call around one original damage-owner invocation on the simulation thread.
// UnitDamage: first=attacker ID, second=victim ID.
// EntityDamage: first=victim ID, second=entity ID.
// FireDamage: first=victim ID, second=responsible player argument (0=unknown).
// Invalid/irrelevant observations never suppress the original damage call.
bool beginDamage(const DamageMemory& memory, DamageSource source,
    int first, int second, DamageProbe& probe);
bool finishDamage(const DamageMemory& memory, DamageProbe& probe,
    DamageObservation& result);

} // namespace SHC141
} // namespace AicTactics

#endif
