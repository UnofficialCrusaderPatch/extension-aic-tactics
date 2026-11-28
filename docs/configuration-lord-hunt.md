# Lord hunt configuration

The native AI plans every attack against one player's keep. A human lord standing in an ally's castle is not at that keep, so the AI wrecks his empty home and never gets near him. With lord hunt on, an AI attacking a human player does two things:

1. **Siege the castle where the lord hides.** The lord is outside his own walls and inside an allied player's walls (the game's walled-in area of that player's keep), and no troop of the attacking main army can walk there. The AI then turns its attack to that castle and runs its full native siege there: it plans again, with siege camp, engineers, ladders and moat filling.
   - **His own castle still has people and buildings:** the army splits. By default the full siege goes to the castle he hides in, and half of the army attacks his own castle on its own. The castle choice and the share are settings.
   - **His own castle is empty:** the whole army goes to the castle he hides in.
   - **During an attack** the AI only changes castles while most of its army remains (80% of the largest army of this attack, or of `AttForceBase` when that is larger).
   - **He leaves the castle again:** the split troops rejoin the army and the attack turns back to his own castle (a new plan, under the same 80% rule; an army below it ends the attack through the native retreat).
   - **The attack ends** (retreat, target defeated, or the AI's own selection changes the opponent): the target is put back and every split or hunting troop returns to the AI's own groups.
2. **Hunt the lord.** As soon as the attackers have an open way to him (the game's own area test, with and without ladders and siege towers; the same test the AI uses to call a castle breached), a share of the army attacking that castle is moved into a group of its own and ordered to attack him. Main army troops go first (then siege defence, the other attack roles and patrols), closest to the lord first, equal distances by unit ID. Engineers, moat diggers, laddermen, tunnellers, siege engines and their crews stay on their jobs.
   - **All hunters dead:** the rest of that army goes, once.
   - **The way closes for 200 ticks** (a wall repaired), **the lord dies** or **the attack ends:** the hunters return to the AI's own groups.

The split troops have no siege plan of their own: melee troops attack the nearest building of that castle they can walk to, then the next; when none is reachable they attack the nearest wall they can stand next to, else they walk to the keep. Archers, crossbowmen, slingers and horse archers walk with them and shoot the troops they meet.

Only attacks on human players change (`PlayerData` AI type 0, a fact every peer shares). AI attacks on AI players are left alone.

| AIC field | Module setting | Values | Default | Effect |
| --- | --- | --- | --- | --- |
| `AICTactics_LordHuntRetarget` | `lordHuntRetarget` | `true`, `false` | `false` | Siege the castle where the lord hides, as above. `false` keeps the attack on his own castle (the hunt still works there). |
| `AICTactics_LordHuntSiege` | `lordHuntSiege` | `HidingCastle`, `HomeCastle` | `HidingCastle` | Which castle gets the full siege when the army splits. The other castle gets the split troops. |
| `AICTactics_LordHuntSplitShare` | `lordHuntSplitShare` | 0–100 | 50 | Percent of the main army sent to the castle without the full siege. 0 sends no split troops; with `HidingCastle` the whole army then goes there. |
| `AICTactics_LordHuntSwitchPercent` | `lordHuntSwitchPercent` | 0–100 | 80 | During an attack (attack states 3–6), change castles only while this percent of the army remains. Before the march the change is always allowed. |
| `AICTactics_LordHuntShare` | `lordHuntShare` | 0–100 | 0 | Percent of the army at the lord's castle that goes for him, rounded up. 0 turns the hunt off. |

An absent field uses the module setting from Customizations; an explicit value changes it for that character only.

## Examples

Turn the lord hunt on for one AI:

```json
"AICTactics_LordHuntRetarget": true,
"AICTactics_LordHuntShare": 50
```

Keep the full siege at the human's home and send a third of the army after the hiding lord's host:

```json
"AICTactics_LordHuntSiege": "HomeCastle",
"AICTactics_LordHuntSplitShare": 33
```

## Limits

- At most eight area pairs are tested per AI update; a troop whose area is beyond that count counts as unable to reach the lord for that update.
- The hunt follows the reach of the hunting group's leader. Re-orders go out at most every 80 ticks, when the leader no longer attacks the lord.
- Census candidates are limited to 384 troops per AI, in unit ID order.
- While the split troops only walk to the keep, they look for a building or wall at most every 80 ticks.

## Saves, replays and multiplayer

- **Saves.** The attack record (opponent and lord, castle, split, hunt wave and timers) and the census it decides from are saved in AIC Tactics' state, and the hunting and split troops are saved as detached groups (`include/aic_tactics/groups.hpp`). A reload continues the same attack with the same troops; nothing is guessed from the map and no troops are swept. Every saved group is checked against its native tribe before use. Changing these settings does not invalidate a save; the next decision follows the new values. With `AICTactics_LordHuntRetarget` off the attack turns back to the lord's own castle and the split troops rejoin; with `AICTactics_LordHuntShare` 0 the hunters rejoin.
- **Replays and multiplayer.** Decisions use only synchronized game state at the AI's native update, in fixed order, with integer math and no random numbers. The settings are part of AIC Tactics' multiplayer admission check; the saved records are part of its replay digest.
- **Without AIC Tactics.** While these features are on, AIC Tactics marks its saved state as required, so Map Extensions refuses the save without AIC Tactics.

## Compatibility

- **UCP2-Legacy `ai_attacktarget` and `ai_assaultswitch`** change bytes inside the game's monthly target selection (the choice of opponent, and keeping the current target during an assault). AIC Tactics replaces only the scheduler's call to that selection, runs it unchanged, and applies the hiding castle afterwards, and only while the selection keeps the same opponent. Both options can stay on.
- **AIC Tactics target policies** (`AttackTargetPolicy` and its commitment) choose the opponent; lord hunt only chooses which of that opponent's shelters is sieged. The committed opponent and its lord stay the policy's target, so commitment, cleanup and `LastAggressor` work unchanged.
- **Monsterfish's Lord Hunt module** is part of AIC Tactics now; installing both is not supported.
- **Diagnostics.** With the module setting `diagnostics` on (default off), each lord hunt decision is written to the UCP log as an `[aic-tactics]` line: the hiding castle and split, every hunt wave, hunters that cannot reach the lord, and the end of the attack.

Lord Hunt by Monsterfish, under GPL-3.0; see [CREDITS.md](../CREDITS.md).
