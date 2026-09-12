#include "aic_tactics/composition.hpp"
#include <cassert>
#include <climits>
#include <cstdio>

int main()
{
    using AicTactics::defenseTypeQuota;
    int roster[8] = {22, 22, 24, 0, 25, 0, 0, 0};
    assert(defenseTypeQuota(roster, 10, 22) == 7);
    assert(defenseTypeQuota(roster, 10, 24) == 3);
    assert(defenseTypeQuota(roster, 10, 25) == 0);
    assert(defenseTypeQuota(roster, 2, 22) == 2);
    assert(defenseTypeQuota(roster, 2, 24) == 0);
    assert(defenseTypeQuota(roster, -1, 22) == 0);
    assert(defenseTypeQuota(0, 10, 22) == 0);
    int cases = 7;
    for (int length = 1; length <= 8; ++length) {
        for (int slot = 0; slot < 8; ++slot) roster[slot] = slot < length ? 22 + slot % 3 : 0;
        for (int maximum = 0; maximum <= 2500; ++maximum) {
            int total = 0;
            for (int type = 22; type <= 24; ++type) total += defenseTypeQuota(roster, maximum, type);
            assert(total == maximum);
            ++cases;
        }
        for (int slot = 0; slot < length; ++slot) roster[slot] = 22;
        assert(defenseTypeQuota(roster, INT_MAX, 22) == INT_MAX);
        ++cases;
    }
    std::printf("%d composition quota checks passed\n", cases);
    return 0;
}
