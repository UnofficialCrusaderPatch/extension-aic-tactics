#include "aic_tactics/composition.hpp"

namespace AicTactics {
int defenseTypeQuota(const int* roster, int maximum, int unitType)
{
    if (!roster || maximum <= 0 || unitType <= 0) return 0;
    int length = 0;
    while (length < 8 && roster[length] != 0) ++length;
    if (!length) return 0;
    const int share = maximum / length;
    const int remainder = maximum % length;
    int quota = 0;
    for (int slot = 0; slot < length; ++slot)
        if (roster[slot] == unitType) quota += share + (slot < remainder ? 1 : 0);
    return quota;
}
}
