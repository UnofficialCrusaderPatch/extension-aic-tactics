#include <cstring>
#include "aic_tactics/shc141_groups.hpp"

namespace AicTactics {
namespace SHC141 {

bool queryTribeAvailability(const unsigned char* tribes, int capacity, int stride,
    int player, TribeAvailability& result)
{
    result.nextID = 0;
    result.freeSlots = 0;
    if (!tribes || capacity != 1250 || (stride != 0x334 && stride != 0x688) || player < 1 || player > 8)
        return false;
    for (int id = 1250 - player; id > 0; id -= 8) {
        short state;
        std::memcpy(&state, tribes + id * stride + 0x18, sizeof(state));
        if (state == 0) {
            if (!result.nextID) result.nextID = id;
            ++result.freeSlots;
        }
    }
    return true;
}

} // namespace SHC141
} // namespace AicTactics
