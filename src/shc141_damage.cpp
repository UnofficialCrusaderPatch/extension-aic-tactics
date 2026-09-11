#include <cstring>
#include "aic_tactics/shc141_damage.hpp"

namespace AicTactics {
namespace SHC141 {

template<class T> static T read(const unsigned char* record, int offset)
{
    T value;
    std::memcpy(&value, record + offset, sizeof(value));
    return value;
}

static const unsigned char* unit(const DamageMemory& memory, int id)
{
    // Fixed SHC 1.41 array capacity; relocated/expanded native owners require a
    // separately admitted binding, not guessed access beyond the original pool.
    if (!memory.units || memory.unitCapacity < 1 || memory.unitCapacity > 2500 ||
        id < 1 || id >= memory.unitCapacity) return 0;
    return memory.units + id * 0x490;
}

static bool hostile(const DamageMemory& memory, int source, int victim)
{
    return memory.teams && source >= 1 && source <= 8 && victim >= 1 && victim <= 8 &&
        source != victim && memory.teams[source] != memory.teams[victim];
}

bool beginDamage(const DamageMemory& memory, DamageSource source,
    int first, int second, DamageProbe& probe)
{
    std::memset(&probe, 0, sizeof(probe));
    if (source < UnitDamage || source > FireDamage) return false;
    const int victimID = source == UnitDamage ? second : first;
    const unsigned char* victim = unit(memory, victimID);
    if (!victim || read<short>(victim, 0x2A0) != 0 || read<int>(victim, 0x3C8) <= 0)
        return false;
    int sourceOwner = second;
    if (source == UnitDamage) {
        const unsigned char* attacker = unit(memory, first);
        if (!attacker) return false;
        sourceOwner = read<short>(attacker, 0x96);
    } else if (source == EntityDamage) {
        if (!memory.entities || memory.entityCapacity < 1 || memory.entityCapacity > 3000 ||
            second < 1 || second >= memory.entityCapacity) return false;
        sourceOwner = read<short>(memory.entities + second * 0xE8, 0x2C);
    }
    const int victimOwner = read<short>(victim, 0x96);
    if (!hostile(memory, sourceOwner, victimOwner)) return false;
    probe.active = true;
    probe.victimID = victimID;
    probe.victimUID = read<int>(victim, 0x98);
    probe.victimOwner = victimOwner;
    probe.sourceOwner = sourceOwner;
    probe.unitType = read<short>(victim, 0x8E);
    probe.health = read<int>(victim, 0x3C8);
    probe.x = read<short>(victim, 0xC4);
    probe.y = read<short>(victim, 0xC6);
    return true;
}

bool finishDamage(const DamageMemory& memory, DamageProbe& probe,
    DamageObservation& result)
{
    std::memset(&result, 0, sizeof(result));
    const bool active = probe.active;
    probe.active = false;
    if (!active) return false;
    const unsigned char* victim = unit(memory, probe.victimID);
    if (!victim || probe.health <= 0 || read<int>(victim, 0x98) != probe.victimUID ||
        read<short>(victim, 0x96) != probe.victimOwner ||
        !hostile(memory, probe.sourceOwner, probe.victimOwner)) return false;
    int remaining = read<int>(victim, 0x3C8);
    if (remaining < 0) remaining = 0;
    if (remaining >= probe.health) return false;
    result.victimID = probe.victimID;
    result.victimUID = probe.victimUID;
    result.victimOwner = probe.victimOwner;
    result.sourceOwner = probe.sourceOwner;
    result.unitType = probe.unitType;
    result.healthLost = probe.health - remaining;
    result.killed = remaining == 0;
    result.x = probe.x;
    result.y = probe.y;
    return true;
}

} // namespace SHC141
} // namespace AicTactics
