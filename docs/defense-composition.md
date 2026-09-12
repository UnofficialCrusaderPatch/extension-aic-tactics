# Keep the intended defender mix

Set `RecruitPolicy` to `WeightedRoles` and `DefRecruitComposition` to
`PreserveSlots`. The existing `DefUnit1..8` roster determines each troop type's
share of `DefTotal`; repeated entries give that type more seats. Recruitment
stops at the first `None`, as it does for the native roster.

For example, Archer, Archer, Spearman with `DefTotal=10` reserves seven seats for
Archers and three for Spearmen. Seats are divided equally between roster entries;
any remainder goes to the earlier entries. A missing spear or recruitment building
leaves a Spearman seat vacant. It does not become another Archer seat, and it does
not prevent another eligible recruitment role from being chosen.

`DefRecruitComposition` defaults to `Native`, retaining the current roster
selection. `PreserveSlots` requires WeightedRoles. Quotas use the existing defense
total, including the native nervous/Extreme multipliers. Existing wall and patrol
assignment remains in charge; `DefWalls` still separates those destinations.
Soldiers already present above their type's share are retained. They are neither
removed nor reassigned to force a new mix.

Partial updates retain the chosen composition. Resetting the character restores
Native composition. Original roster, quota and interval fields retain their values
and getters. No AI pack is rewritten. Switching policies requires a clean process.

## Native integration

Related to [recruitment issue #2](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/issues/2)
and dependent on recruitment runtime PR #12.

Legacy `ai_defense` remains required ON and its source is unchanged. The extension
chains its reset/count hooks at 0x579879/0x579A7C, preserving the existing wall
counter and native defender count. Those native branches census wall defenders
(role 1) and outer patrols (role 4). A fixed 9-by-80 array adds per-type counts only
for opted-in personalities. There is no second world scan or hot-path allocation.

The recruitment adapter uses the same census age and total as the native quota.
An absent, stale or inconsistent census makes Defense temporarily ineligible;
other roles remain available. Recruits within one opportunity use separate local
counts, including new wall defenders, so multiple hires do not overfill a share
or the remaining wall quota. Derived native counts are never rewritten.

## Saved continuation

Map Extensions 1.0.0 owns serialization. AIC Tactics registers one section and
validates its complete payload before restoring anything. The payload contains
the 16 compiled policies, 16 original AIC records, the interval-migration setting,
and the census tick/validity/type counts. Comparing exact configuration bytes
prevents an old raid adjustment or a composition census from silently being used
with a different authored policy. Observation counters are excluded.

The current format is `AICTACT` plus byte 3, followed by little-endian 32-bit
interval setting, native initial timer, 4,608 compiled-policy bytes, 10,816 native-AIC
bytes, census tick, validity and 720 counts. Total: 18,328 bytes before compression.
The native configuration ABI is now 288 bytes per character; Lua and DLL reject
earlier ABIs. This is an unreleased format change, not a migration
promise for earlier development saves.

Old saves remain loadable with Native recruitment. A new match initializes empty
census state. An old save without policy state is rejected when WeightedRoles is
active; start a new match with that policy. Save/load with the same configuration
restores the census instead of skipping a decision while rebuilding it.

Full module/content admission remains a release gate: Map Extensions 1.0.0 does
not reject a save merely because its registered provider is now absent. The
recorder's export/restore/integrity integration is also still tracked separately
in recorder issue #47. The supplied capture/validate/restore functions use the
same payload as ordinary saves; their presence does not imply recorder support.

## Verification

- MSVC2005 SP1: 20,023 quota cases and 1,719 runtime/original-instruction checks,
  plus the existing 98 recruitment and 32+4 group-admission cases.
- 480 actual FASM wrapper comparisons against the unchanged Legacy Lua port:
  native effects, registers, flags, caller stack and callback arguments match.
  Added C++ calls are ABI spies in this particular test.
- 40 Lua tests cover loader/backend transactions and state validation/restore.
- The adjusted interval bridge passes 11,520 FASM/x86 checks with the new ABI.

No new in-game, multiplayer, saved-continuation or performance acceptance is
claimed for this change. Earlier PR #12 game evidence identifies its older builds.
