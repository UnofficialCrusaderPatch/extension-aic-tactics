#ifndef AIC_TACTICS_ARMY_HPP
#define AIC_TACTICS_ARMY_HPP

namespace AicTactics {
namespace SHC141 {

enum AttackPreparation { NativePreparation, PrepareDuringAttack };
struct ReserveGroup { int id; unsigned int uid; };
struct ReserveState {
    int deployed;
    int returning;
    int recruitCursor;
    int transferSlot;
    int transferWord;
    ReserveGroup groups[22];
};
extern ReserveState reserves[9];
extern int reserveGroupOwner[1250];
extern unsigned int reserveGroupUID[1250];
bool reserveRecruitmentActive(int player);
bool reserveRoleAvailable(void* aic, int player, int role);
int __cdecl isReserveUnit(int unit);
void resetReserveCensus();
void countReserveUnit(int unit);
void completeReserveCensus();
void prepareArmyUpdate(void* aic, int player);
void finishArmyUpdate(int player, int phaseBefore);
void noteArmyLaunch(int player);
void noteArmyReturn(int player);
bool returnArmyToCampfire(void* aic, int player);
void __fastcall recruitWithReserve(void* aic, void*, int player);
int __fastcall reserveRecruitType(void* aic, void*, int player, int role);

} // namespace SHC141
} // namespace AicTactics
#endif
