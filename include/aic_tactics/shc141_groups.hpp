#ifndef AIC_TACTICS_SHC141_GROUPS_HPP
#define AIC_TACTICS_SHC141_GROUPS_HPP

namespace AicTactics {
namespace SHC141 {

struct TribeAvailability {
    int nextID;
    int freeSlots;
};

// Read-only admission for the original createTribeForPlayer owner. tribes is
// its native Tribe array (including index zero), not the enclosing TribesState.
// This does not allocate, reserve an ID or prove an AIC role can use that slot.
bool queryTribeAvailability(const unsigned char* tribes, int capacity,
    int player, TribeAvailability& result);

} // namespace SHC141
} // namespace AicTactics

#endif
