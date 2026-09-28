# Native AI pressure responses (investigation, not implemented AIC fields)

The original game has three related mechanisms, not two interchangeable
strength states. The current AIC Tactics module does not replace them.

| Native owner | Input | Effect |
| --- | --- | --- |
| `AICState::computeNervousness` | Friendly troop value near the AI's keep versus total enemy troop value, with a piecewise native margin | Updates `isNotNervousByEnemyTroopValue`; a transition may request ally help and play a line. It does not by itself start the severe action tracker. |
| `AICState::teamIsWeakRelativeToEnemy` called by `updateAIStrengthState` | Team/enemy troop-value ratio, keep enclosure, game age and, on one path, enemy ranged power | Starts or clears `aiNervousActionsTracker`. While active, native strength is forced Weak, recruitment/positioning changes, attacks may retreat and some buildings can be destroyed. |
| `AICState::hasNotEnoughSupplies` called by `updateAIStrengthState` | Marketplace/granary access, wood, gold, food and popularity | Starts `aiBuildingDestroyChoiceTracker` and a separate shortage demolition sequence. It must not be presented as another enemy-fear level. |

Ghidra confirms that `TroopValueState::recomputeTotalTroopValueOfTroopsNearKeep`
already scans the native unit pool at load-balancer phase 93 of 200. It counts
eligible **friendly** units when `max(abs(dx), abs(dy)) < 41` from their own
keep, excluding engineers, tunnelers and laddermen. This is a square with a
40-tile inclusive reach; the original path is not a census of nearby enemies.
Changing its radius per AI should use this owner's existing comparison and
native count. Do not add another whole-unit pass. The function's runtime hook
site and calling context still require framework AOB and normal/Extreme ABI
verification. Reference addresses from OpenSHC/Ghidra are research only.

AIC Tactics separately has a hostile-home-power census for retaliation:
`combat.cpp::atHome` currently checks a 32-tile circular area. Keep that
meaning separate from the native friendly-defense radius. If an authored
pressure condition needs nearby *enemy* power at a configurable radius,
extend that existing census owner only when needed; do not change the
retaliation radius as a side effect or start a second map/unit scan.

## Proposed per-AI interface

Keep the default `Native`. An optional pressure-response record can use the
same bounded, ordered AND/OR condition representation planned for strength
states. Give authors distinct warning/help and severe-retreat triggers,
with useful facts such as own/enemy or team/enemy troop power, keep enclosure,
time and nearby hostile power. A single friendly-defense radius per AIC is
enough for the native warning count; a separate hostile-threat radius is
needed only if such a condition is authored. Keep the supply-crisis policy
independent. Expose actions or duration only after tracing their native
consumers, so a seemingly harmless switch does not leave an attack or
building-demolition tracker half active. These are behavior concepts, not
published AIC keys yet.

Compile rules at AIC load, evaluate on the original decision schedule and
compute only referenced facts. Preserve native RNG and original owner calls
for every unconfigured AI. Native countdowns remain native saved state; any
extension hold/cooldown state belongs in the existing AIC Tactics Map
Extensions/replay section and word digest. A save used as a new map must
initialize fresh policy state. Compare per-200-tick census cost and monthly
decision cost on both normal Crusader and Extreme before claiming negligible
overhead.
