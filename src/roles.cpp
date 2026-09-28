#include "aic_tactics/runtime.hpp"

namespace AicTactics {
namespace SHC141 {

int engineerRoleFallback = 1;

namespace {
template<class T> T read(unsigned int address, unsigned int offset)
{
    return *reinterpret_cast<const T*>(address + offset);
}

bool ordinaryRole(int role)
{
    return role == 1 || role == 2 || role == 4 || role == 6 || role == 7
        || (role >= 11 && role <= 20);
}
}

// The native role dispatch is reused after this predicate; it owns the role
// counters and all normal-unit cases. Role 10 retains its original engineer path.
int __cdecl countableEngineerRole(int player, int unit)
{
    if (!engineerRoleFallback) return 0;
    if (player < 1 || player > 8 || unit <= 0
        || unit >= static_cast<int>(nativeBindings.unitCapacity)) return 0;
    const int character = read<int>(nativeBindings.players + player * 0x39F4, 0x2300);
    if (character < 2 || character > 17) return 0;
    const unsigned int address = nativeBindings.unitRecords + unit * 0x490;
    const int role = read<short>(address, 0x42A);
    if (read<short>(address, 0x8C) != 2 || read<short>(address, 0x8E) != 30
        || read<short>(address, 0x96) != player || read<short>(address, 0x2A0) != 0
        || read<int>(address, 0x3C8) <= 0 || !ordinaryRole(role)
        || read<short>(address, 0x42E) != 0 || read<unsigned char>(address, 0x32F) == 2) return 0;

    const int group = read<short>(address, 0x2D8);
    if (group <= 0 || group >= 1250) return 0;
    const unsigned int tribe = nativeBindings.tribes + group * nativeBindings.tribeStride;
    if (read<int>(tribe, 0x2C) != player
        || read<unsigned int>(tribe, 0x34) != read<unsigned int>(address, 0x2E4)
        || read<short>(tribe, 0x40) != 2 || read<short>(tribe, 0x50) == 0x410) return 0;
    const unsigned int member = tribe + 0x60 + (unit / 16) * 2;
    return (read<unsigned short>(member, 0) & (1U << (unit % 16))) != 0 ? 1 : 0;
}

} // namespace SHC141
} // namespace AicTactics
