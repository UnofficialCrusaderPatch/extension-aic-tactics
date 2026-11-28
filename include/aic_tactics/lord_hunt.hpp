#ifndef AIC_TACTICS_LORD_HUNT_HPP
#define AIC_TACTICS_LORD_HUNT_HPP

namespace AicTactics {
namespace SHC141 {

// Lord hunt settings, indexed by installed AIC character 1..16. Each value is
// 0 (use the module setting) or the character's choice + 1:
// lordHuntRetarget 0 off, 1 on; lordHuntSiege 0 hiding castle, 1 home castle;
// lordHuntSwitch, lordHuntShare and lordHuntSplit are percentages 0..100.
extern int lordHuntRetarget[17];
extern int lordHuntSiege[17];
extern int lordHuntSwitch[17];
extern int lordHuntShare[17];
extern int lordHuntSplit[17];
extern int lordHuntRetargetFallback;
extern int lordHuntSiegeFallback;
extern int lordHuntSwitchFallback;
extern int lordHuntShareFallback;
extern int lordHuntSplitFallback;

enum LordHuntPhase {
    LordHuntHidingDecided = 1,  // the hiding check has acted for this attack
    LordHuntSwitchedBack = 2,   // the attack left the hiding castle again
    LordHuntOver = 4            // both hunt waves are spent
};

enum LordHuntSplitOrder { NoSplitOrder, SplitAttacksBuilding, SplitAttacksWall, SplitMoves };

// One AI attack on a human player. Group fields hold a detached group record
// index + 1 (0 = none); `returning` has one bit per detached record this
// feature is handing back to the native role slots.
struct LordHuntState {
    int origin;                 // human player the attack is against
    int originLordUID;
    int host;                   // ally whose castle got the siege; 0 = origin's own
    int splitTo;                // castle the split troops attack; 0 = no split
    int phase;
    int peak;                   // largest attack army of this attack
    int huntGroup;
    int splitGroup;             // melee split troops
    int shooterGroup;           // ranged split troops
    int huntWave;               // 1 the first share, 2 the rest of that army
    int huntSource;             // LordHuntSource of the current hunt
    unsigned int huntOrderTick;
    unsigned int huntClosedTick; // 0 while the way to the lord is open
    int splitOrder;
    int splitSize;
    int returning;
    unsigned int splitSearchTick; // last target search while the split troops walk
};

enum LordHuntSource { MainArmySource = 0, SplitTroopSource = 1 };

// Filled by the combat census for AIs in attack states 1..6. Candidates are
// unit IDs in census (unit ID) order, tagged with their LordHuntSource.
const int LordHuntCandidateCapacity = 384;
struct LordHuntCensus {
    int attackTroops;
    int candidateCount;
    int candidates[LordHuntCandidateCapacity];
};

extern LordHuntState lordHunts[9];
extern LordHuntCensus lordHuntCensus[9];
extern int lordHuntUnits[9];     // living units other than the lord, per player

// What the target owner writes after the lord hunt update: the castle to
// target (0 leaves it) and the AI attack state to request (0 leaves it,
// 1 plans the attack again, 8 asks for native cleanup).
struct SiegeRetarget {
    int castle;
    int phase;
};

void resetLordHuntCensus();
// Called by the combat census for every living unit of players 1..8.
void countLordHuntUnit(int unit, int player, int type);
// Runs after the native attack state update of an AI player.
SiegeRetarget updateLordHunt(void* aic, int player);
// The castle an attack on `opponent` sieges: the hiding castle while a
// redirect for that opponent is active, otherwise the opponent's own.
int lordHuntSiegeCastle(int player, int opponent);

} // namespace SHC141
} // namespace AicTactics
#endif
