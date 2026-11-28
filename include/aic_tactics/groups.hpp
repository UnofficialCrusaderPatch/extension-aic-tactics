#ifndef AIC_TACTICS_GROUPS_HPP
#define AIC_TACTICS_GROUPS_HPP

namespace AicTactics {
namespace SHC141 {

// Detached attack groups: troops an AI duty moves out of the native role
// slots into a tribe of their own (a lord hunt, a split assault, an extra
// patrol, an ally relief force, an extra siege-defence group). The native AI
// never orders a tribe outside its slot table, so the owning duty does, and
// the record below is saved so a reload continues the same duty with the
// same troops.
enum GroupDuty {
    NoDuty = 0,
    PatrolDuty = 1,
    HuntDuty = 2,
    SplitDuty = 3,
    ReliefDuty = 4,
    SiegeDefenceDuty = 5,
    GroupDutyCount = 6
};

struct DetachedGroup {
    int tribe;
    unsigned int uid;
    int duty;
    int target;            // duty-defined: player, unit or building id
    unsigned int targetUID;
    int x;                 // duty-defined goal tile
    int y;
    unsigned int startTick;
    int launchSize;        // members when the duty began, for loss checks
    int returning;         // members are being handed back to native slots
    int cursor;            // member word to continue from
    unsigned int leaving;  // relief: tick its troops began to leave the ally's walls; 0 none
};

const int DetachedGroupsPerPlayer = 8;
extern DetachedGroup detachedGroups[9][DetachedGroupsPerPlayer];

// Returns the record index, or -1 when no record or native tribe is free.
int openDetachedGroup(int player, int duty);
bool detachedGroupValid(int player, int index);
int detachedGroupSize(int player, int index);
// The group's native leader when it is a living member; 0 otherwise.
int detachedGroupLeader(int player, int index);
// Moves one living unit of the player from its current tribe into the group.
bool moveUnitToDetachedGroup(int player, int index, int unit);
// Hands up to `budget` members back to their native role slots through the
// game's own attack-group selection; clears the record once the group is
// empty. Returns the members still left.
int returnDetachedGroup(void* aic, int player, int index, int budget);
// Drops records whose tribe no longer matches its saved identity.
void validateDetachedGroups();

} // namespace SHC141
} // namespace AicTactics
#endif
