# Strength-state extension investigation (proposal, not implemented AIC fields)

Tracked in [AIC Tactics #29](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/issues/29).
The separate native warning, severe pressure and supply-crisis paths are
recorded in [native-pressure-investigation.md](native-pressure-investigation.md)
and [#28](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/issues/28).

This note records the native behavior and an implementation boundary for adding
per-AI Very Strong and Overpowered states. Names and configuration shapes below
are proposals, not supported keys. Do not place them in an installable AIC yet.

## Native behavior found

The named OpenSHC/Ghidra project already maps the relevant owners:

| Responsibility | Named original-game function or structure |
| --- | --- |
| Strength score and three-state decision | `AICState::updateAIStrengthState`, `PlayerData::aiStrengthState` and `aiStrengthFeeling` |
| Monthly/Extreme-weekly scheduling | `AICState::updateAIBehaviour` |
| Recruitment cadence and acquisition | `AICState::aiRecruitUnits` |
| Attack subrole choice and native maxima | `AICState::randomlySelectAttackUnitTypeToRecruit` |
| Attack/raid requirement writers | `AICState::setCurrentAttackStrength`, `setCurrentAttackRaidParameter`, `getCurrentDesiredAttackRaidUnitCount` |
| Market and food-demand owners | `AICState::buyRequiredGoods`, `setFoodBuyPlan` |

These labels and reference addresses guide call-graph and ABI verification;
the shipped extension still needs its existing AOB/native-binding owner.

The OpenSHC reference draft of `AICState::updateAIStrengthState` computes a
0–10 score from four native counters: gold (0/1/2 at 200 and 2000), population
(0/1/2 at 15 and 40), food (0/1/2 at 20 and 200), and attack + raid + defense
troop count (0/2/4 at 8 and 40). Scores below 3 select Weak, 3–7 Default,
and 8–10 Strong. Native pause, nervousness and supply handling can force Weak.
Normal Crusader evaluates this on the monthly AI decision; Extreme also has
a weekly path. A read-only headless decompilation of the named Stronghold
Crusader executable confirms these branches. The OpenSHC draft currently uses
16/21 for the first population/food thresholds and needs a separate faithful
correction before it is cited as a verified reconstruction.

`AIStrengthType` has exactly Default=0, Weak=1, Strong=2. Native recruitment
probabilities and recruitment intervals index fixed fields in the 676-byte
`AICSpecification` using that value. Therefore an extension must not store 3
or 4 in `aiStrengthFeeling` or append entries to the native specification.
Maintain a separate effective state for opted-in AIs and keep native state for
original consumers.
The present AIC Tactics implementation only reads `aiStrengthFeeling` for its
opted-in weighted recruitment; it does not replace the original score writer.
The OpenSHC draft's off-by-one thresholds are a reconstruction discrepancy,
not an observed AIC Tactics runtime regression.

Existing AIC fields already include `RecruitGoldThreshold`,
`TradeAmountFood`, `TradeAmountEquipment`, `RaidUnitsBase/Random`,
`AttForceBase/Random`, the three recruitment probability rows, and three
recruitment intervals. AIC Loader owns those fields; AIC Tactics must reference
their effective values rather than add duplicate base copies.

The native monthly update compares gold with `RecruitGoldThreshold` and writes
`canStartSpending`. The raiding role bypasses that comparison. This shared flag
gates ordinary recruitment, several engineer/siege paths, and much goods
buying; emergency food/popularity and nervous-state buying have exceptions.
Thus a configurable "threshold scope" needs decisions at the actual spending
owners. Simply changing the shared flag would change unrelated purchases.

At wave creation, `setCurrentAttackStrength` adds a random component and
5/7 troops per wave depending on gold, capped at 200; `AttForceBase` is added
later to the readiness requirement. Legacy `ai_addattack` globally replaces
the growth arithmetic with a flat or base-relative choice. Per-AI/state growth
must own that writer once, preserve the existing cap/other launch checks and
resolve the global Legacy conflict for untouched AIs.

The raid requirement is `RaidUnitsBase + currentAttackRaidParameter`, with
Extreme's trail multiplier. The parameter is rolled from `RaidUnitsRandom`
with additional gold/opponent-gold branches. A state-specific base or random
value should be sampled when the raid requirement is created, not changed
after a group has begun assembling.

`setFoodBuyPlan` queues `TradeAmountFood` when configured stock minima are
missed. Equipment demand is also queued by the recruitment path using
`TradeAmountEquipment`; the full set of equipment-trade callers still needs
confirmation. The existing market/trade owner must execute purchases.

## Smallest author-facing model

Prefer **one optional AIC record** containing an ordered rule list and named
state overrides, rather than a switch plus separate `Weak*`, `Strong*`,
`VeryStrong*` and `Overpowered*` fields for every behavior. A rule says when
to select a state; that state's override lists only behavior that differs.
The Default/Normal state needs no trigger. Weak/Strong without custom rules
retain native classification; Very Strong/Overpowered require a matching rule
and otherwise do not exist. Presence of this record is the opt-in; an empty
record resets to native behavior.
This is a proposed interface, not an implemented or published AIC key.

Keep every new behavior opt-in. Untouched AICs retain native three-state
classification, native Weak/Default/Strong values and Legacy-compatible
growth. In the new mode, call the base state Default in AIC syntax and Normal
in help text. Resolve each property independently: a value explicitly set
for the selected state wins; otherwise an explicitly set Default value wins;
otherwise use the existing AIC value where one exists. This lets a new
Default override cover every state without copying it five times while old
Weak/Strong AIC values keep working when no new override claims that property.
For newly added states with no override, define a documented legacy fallback
per property, normally Strong for fields that already have a Strong variant
and the sole existing value for fields without variants. Explicit zero and
false remain distinct from omission. Without a matching Very Strong or
Overpowered rule, that state is unreachable.

Use one bounded, ordered state-rule list per personality. The first matching
rule selects any of the five effective states; when none matches, use the
native Weak/Default/Strong state. A rule may have a small list of alternatives
(OR); within each alternative, all listed predicates must match (AND). Do
not allow recursive Boolean trees or implicit numeric coercion. Precompile
the authored order, comparator and needed-fact mask at AIC load. Empty
conditions must be rejected or made explicitly unconditional. This provides
author-controlled priority without a second scoring formula or a forest of
`WeakGoldMin`-style keys. The original emergency Weak state remains in native
game state; an authored rule can select a different *extension* state if the
author deliberately makes it match.

Useful predicates with identified original-game owners are: native score or
class, gold, population, food, attack/raid/defense troop count, native
`totalTroopValue`, team/enemy troop value, keep enclosure, home threat,
defense quota and attack phase. `PlayerData` holds troop value and both team
and enemy totals; compute a relative percentage from those underlying totals
at the decision boundary rather than relying on
`relativeStrengthOfTeamComparedToEnemy`, which another native function updates
only when called. A per-opponent comparison needs an explicit opponent scope;
the current target or strongest hostile player can be found among at most
eight player slots. Define the no-enemy case explicitly so it cannot silently
count as 100% power.

Ghidra confirms `GameStateStructures::checkKeepEnclosed` reads existing
path-connection-layer and zone-size data, then checks at most eight player
slots. It does not perform a fresh path search or map scan. Call it once per
AI state decision only when an authored rule needs it, after cheap numeric
conditions have passed. The existing `currentResources[25]` supports a total
gold-plus-wares predicate with a bounded 25-resource sum. The definition of
ware value must use a verified native *sale-price* owner, saturating integer
arithmetic and a documented inclusion list; it must not call market functions
for every recruitment attempt. Until that price/stock semantic is verified,
this remains a candidate predicate, not a published AIC key.

One per-state override record can cover recruitment weights and interval,
gold reserve, raid base/random size, attack base/random size, food/equipment
purchase amount, attack composition and wave growth. Existing AIC Tactics
`RaidRiskTolerance` and `SiegeForceMax` are also sensible optional state
overrides: a weak AI can avoid defended buildings, while an overpowered AI
can use more siege equipment. Changing `AttackTargetPolicy` mid-attack is
deliberately excluded because committed armies must retain their target.
Do not add separate
`Weak*`/`Strong*`/`VeryStrong*` fields for each value. Reuse the existing
main-attack unit slots and native role quotas for composition; a ratio policy
would weight eligible slots, not invent new unit types or native slots.
Growth can be Flat or PercentOfBase, with a value per state and one clearly
defined integer rounding rule. Snapshot the chosen state and wave parameters
at wave creation so changes in gold cannot resize an assembling army.

The author confirmed that attack ratios cover the **whole force**, including
engineers and other special roles, and that each state can choose a different
mix. The current AIC Tactics recruiter already probes ten native attack
subroles plus the main-troop role. Preserve their native unit types, group
assignments and hard quotas. Make the ratio optional **independently of**
`RecruitPolicy`: a modder should not have to replace the defense/raid/attack
recruitment rows just to choose an attack mix. An omitted mix in the selected
state inherits an authored Default mix, or otherwise uses vanilla selection.
An omitted state block must not opt a personality into new behavior.

For the author-facing mix, list only desired special-role shares, as whole
percentages; main troops receive the remainder. For example, an engineer
share of 10 and ladderman share of 5 means 85% main troops before native
limits and availability are applied. Within main troops, retain the existing
`AttUnitMain1..4` cycle unless a separate main-slot mix is explicitly
requested. Unlisted special roles get zero *only in an authored mix*, and an
explicit zero remains zero. Shares cannot exceed 100 together; require a
non-engineer path toward the existing attack-readiness target. Native role
maxima remain hard caps, so the final force may differ from requested shares.
If a positive-share role is capped, blocked or unaffordable, select among the
remaining positive-share eligible roles; never recruit a zero-share role as
an invisible fallback. Reject a mix that names no recruitable attack role.

At each existing attack recruitment opportunity, compare the current native
role counts plus recruits already made in that update with the selected
state's shares. Choose the eligible role with the greatest integer share
deficit. Eleven bounded comparisons are sufficient; ties have a fixed order
or use the owning synchronized RNG. Do not scan units, add an attack census,
search the map or evaluate triggers per frame. Derive the effective state at
the native strength-decision cadence and keep a small per-player snapshot
only if changing state mid-wave would otherwise change an assembling force.
The current `recruitOpportunity` hook runs only for `WeightedRoles`. An
independent attack-mix opt-in also needs the existing native attack-role
selector's owner path; it cannot be implemented by enabling WeightedRoles
behind the author's back. Inspect that selector's hook context and the
existing recruitment probe before adding a second entry point. For an
unconfigured personality, retain the original selector and its RNG calls.

Native readiness counts attack troops **excluding engineers** against
`AttForceBase + currentWaveRandomAttackingStrength`. For the opted-in ratio
policy, retain this established combat target and derive additional engineer
capacity from the desired whole-force share. Engineer share must be below
100%; the final mix is approximate when native role caps, equipment, gold or
group slots prevent recruiting a requested share. Do not redefine
`AttForceBase` as a new total including engineers or silently bypass readiness.
Check every reserve, team-coordination and attack-launch consumer of the
combat target before implementing this calculation.

## Author-facing schema and help

Keep one nested policy record in the AIC Loader's atomic provider: an ordered
rule list for state selection and one sparse override record per state.
`Default` is the base state; `Weak` and `Strong` can be reached through native
classification without custom rules, while `VeryStrong` and `Overpowered`
require a matching rule. Each state contains only its differing settings,
including its own optional attack mix. Resolve each setting independently:
selected state, then an
authored Default override, then the existing AIC/native setting. This
preserves explicit `false` and zero and prevents a state that only changes
attack mix from silently changing trade or recruitment. No new native enum
values or `AttUnitMain` slots are needed.

Ship a machine-readable schema alongside the module, generated from its
field metadata at packaging time. It should carry type, range, default or
inheritance behavior, a concise title/help key and one small example per
setting. Runtime checks still own cross-field rules such as share totals;
the build must compare schema constraints with those checks. The AI Toolkit
should consume the packaged schema rather than maintain a second set of
ranges and defaults. Localize titles/help through the existing nine-language
registry; keep an English description in the schema for non-Toolkit clients.
In the UI, hide state overrides until a
state is added; show inherited/native values as such rather than copying
defaults into five forms. Group settings by *when the state applies*,
*recruitment*, *attack/raid size*, *army mix* and *spending*. A concise help
example should show a Normal mix and one stronger-state override; do not
list every native AIC field or repeat the same explanation in every state.
Document that percentages are targets for attack-role recruitment, not a
guaranteed final force, and that existing AIC role maxima still apply.

The current module has hand-written Lua validation and Markdown
descriptions, but no shared schema export. The Toolkit owner is
`Krarilotus/AI-Toolkit`. At its 28 September 2026 main checkout, the
Character editor uses `config/template*.json`, `fieldPools.json`,
`sections*.json`, and translated `locales/*/{fields,help}.yaml`.
`character-editor.js` flattens nested objects into generic leaf controls;
it also warns when loaded keys are absent from the static template. Arrays
and optional per-state records therefore need a small dedicated control
and an extension-metadata adapter in that owner. Filling the static template
with all five states would add defaults to every AIC and destroy sparse
inheritance. Keep unknown AIC fields intact on save while adding a dedicated
editor only for supported module metadata. Inspect the Toolkit's contribution
guidance and current PRs before that dependent integration; do not publish
proposed keys or hand-maintain a second set of constraints.

The gold-reserve scope needs a separate owner audit before specifying public
values. Candidate categories are ordinary recruitment, siege/engineers,
equipment purchases and food/other purchases. Preserve native exceptions by
default, and keep actual prices, market access and debit with the existing
owners. Do not call `RecruitGoldThreshold` a recruitment-only control.

## Integration and acceptance gates

Reuse the AIC Loader atomic provider and existing AIC Tactics per-character
configuration and per-player runtime state. First verify the OpenSHC drafts
against the named game binary and trace every target writer/caller. Resolve
new native sites through UCP's current AOB scanner with normal/Extreme ABI
fixtures. Coordinate the `ai_addattack` Legacy requirement and preserve an
equivalent fallback for personalities that do not opt in. Keep any necessary
selected-state/wave snapshot in the existing save/replay state owner; a saved
game renamed to a map starts with a fresh plan.

Acceptance needs two AIs with opposite policies, missing/zero/false values,
all five reachable states, absent upper states, native default equality,
mid-assembly transitions, gold-reserve exceptions, raid/attack quotas,
market shortages, save/load and replay state restore. Measure the added work
on the existing AI schedule. Multiplayer gameplay is player-owned.

Source leads: `OpenSHC-aic-tactics-reference/src/OpenSHC/AI/AICState/`
`updateAIStrengthState.cpp`, `updateAIBehaviour.cpp`, `aiRecruitUnits.cpp`,
`setCurrentAttackStrength.cpp`, `setCurrentAttackRaidParameter.cpp`,
`getCurrentDesiredAttackRaidUnitCount.cpp`, `setFoodBuyPlan.cpp`, and
`buyRequiredGoods.cpp`; `extension-ucp2-legacy/port/ai_addattack.lua`;
`aicloader-atomic-updates/personality.lua`; and the current AIC Tactics
recruitment/army/raid adapters. OpenSHC addresses in these sources are
reconstruction evidence, never extension runtime bindings.
The read-only Ghidra probe is `Roadmap/Investigations/GhidraStrengthStateProbe.java`;
its output is in an isolated temporary project, not in the module package.
