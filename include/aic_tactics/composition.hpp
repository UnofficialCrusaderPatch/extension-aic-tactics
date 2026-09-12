#ifndef AIC_TACTICS_COMPOSITION_HPP
#define AIC_TACTICS_COMPOSITION_HPP

namespace AicTactics {
enum DefenseComposition { NativeComposition, PreserveSlots };

// Native rosters end at their first zero. Earlier slots receive remainder seats;
// repeated types combine their seats. Negative quotas admit no recruits.
int defenseTypeQuota(const int* roster, int maximum, int unitType);
}
#endif
