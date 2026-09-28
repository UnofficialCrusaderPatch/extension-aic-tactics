# AIC Tactics authoring model (proposal; no new keys supported yet)

## Proposed JSON shape

Use one additional field, `AICTactics`, inside each AI's existing
`Personality`. It contains category objects: `Recruitment`, `Attacks`,
`Targeting`, `Raids`, `Siege`, `States` and `Pressure`. These categories are
for AIC authoring, separate from the module-wide controls in UCP
Customizations. Ordinary native AIC fields remain at `Personality` level.
The new names in this example are illustrative, **not supported keys**:
This is the Loader's `AICharacters` file shape; the Toolkit's character
document would put the same `AICTactics` object under its `aic` object.

```json
{
  "AICharacters": [{
    "Name": "Wolf",
    "Personality": {
      "RecruitGoldThreshold": 200,
      "AICTactics": {
        "Attacks": {"Preparation": "DuringAttack"},
        "Siege": {"SafePlacement": true, "Harassment": false},
        "States": {
          "Rules": [{
            "State": "VeryStrong",
            "All": [{"GoldAtLeast": 10000}, {"KeepEnclosed": true}]
          }],
          "Effects": {
            "Normal": {"Recruitment": {
              "Weights": {"Defense": 15, "Raid": 15, "Attack": 70, "Sortie": 0}
            }},
            "VeryStrong": {"Attacks": {
              "ForceBase": 140,
              "MixPercent": {"Engineers": 10, "Laddermen": 5}
            }}
          }
        }
      }
    }
  }]
}
```

The first version should register `AICTactics` through AIC Loader's existing
exclusive additional-field and atomic-provider APIs. Its nested record is
validated by the AIC Tactics provider and compiled once into the existing
runtime owner. JSON nesting changes authoring, not the native 676-byte AIC
array. The current 352-byte AIC Tactics runtime record has no room for these
new policies; extend its actual backend/state owner with a versioned migration
before publishing this format. The Loader already reads JSON AIC files via its
YAML parser (`resources/vanilla.json` is an installed example).

Treat each submitted `AICTactics` object as an atomic replacement. Omission
inside that object means inherit; `false` and `0` are explicit values. An
empty object removes all nested overrides. Existing flat AIC Tactics fields
remain accepted for old personalities. If old and nested representations
assign the same behavior in one effective personality, reject a conflicting
pair with a path-specific error rather than silently choosing one. The
Toolkit should write the nested representation for new work and migrate old
flat fields on edit, preserving values and defaults. Do not move native AIC
fields or require authors to repeat inherited state rows.

The current Toolkit editor recursively flattens objects and arrays into
primitive controls and its template supplies many defaults. It needs a
dedicated `AICTactics` editor for optional categories, ordered rule rows and
per-state overrides; pre-populating five states would erase inheritance.
Localize category headings, field help, validation and inherited-value labels
through the live locale registry.

## Native values remain the baseline

The native AIC record is character configuration shared by every player using
that character. Native current strength, spending flags, raid parameters and
pressure trackers are live player state. Never rewrite the shared native AIC
record on a state transition, store Very Strong/Overpowered in its three-value
strength enum, or change a field merely to make an overlay easier to read.
Two Wolves in different game states must be able to use different effective
values while retaining the same authored AIC baseline.

Resolve one property at its native owner's decision boundary, in this order:

1. A more specific, already-authored situational rule where that system has
   one (for example `RecruitConditions`). Preserve that rule's old semantics.
2. An explicit override for the selected effective state.
3. An explicit Normal override used as the author's cross-state default.
4. The existing AIC value for the appropriate native row. Very Strong and
   Overpowered use Strong for a three-row field; a scalar uses its sole value.
5. The existing module fallback only for a module-owned option absent in
   that AI's effective AIC.

Resolve by *presence*, never truthiness: `false` and `0` win when authored.
An explicit Normal override intentionally supersedes a native Weak/Strong
row for that property. The Toolkit must show this provenance clearly and
allow a state-specific override when the author wants a different Weak or
Strong value. An absent policy follows the original call/RNG path exactly.

The overlay is per player, but its authored configuration is per character.
Evaluate state on the original AI decision schedule, after the native state
has been updated. By default a native emergency Weak result takes precedence;
an explicit pressure policy is the only route to changing that trigger.
At wave or raid creation, snapshot values that must remain fixed until that
force completes. Other values resolve when their existing native owner next
uses them. Do not patch a generic AIC getter while leaving direct native
reads unchanged: audit every consumer of a property and integrate at its
actual writer/decision owner. If a field has uncontrolled consumers, keep
that state override unpublished until its semantics can be made consistent.
For example, `RecruitGoldThreshold` writes a shared spending flag with
several consumers; changing the threshold scope requires an owner-by-owner
audit rather than a second flag or a silent reinterpretation.

Native saves continue to own native counters and flags. Only genuinely new
per-player decisions or in-progress force snapshots belong in AIC Tactics'
existing Map Extensions state and replay digest, with versioned migration.
On save/load restore the same effective plan; when a save is started as a new
map, discard the old plan and recompute from fresh game state. The nested
JSON is compiled at load, not parsed on AI updates.

## One sparse policy per AI character

An absent policy leaves the native three-state calculation, AIC values,
pressure responses and RNG path unchanged. One optional policy contains
ordered **state rules** and sparse **state effects**. Pressure responses use
a separate optional policy because fear and supply shortage are not strength
levels. Both use the existing AIC Loader atomic provider. The installed AIC
Tactics module must still be enabled. Different players
using the same character evaluate their own live game facts.

The UI should say **Normal**; the serialized base state may retain the AIC's
established `Default` name. `Weak`, `Normal` and `Strong` keep their native
classification until an authored rule overrides it. `Very Strong` and
`Overpowered` exist only when an authored rule can select them. An ordered
rule selects any state; first match wins, otherwise the native state wins.
Native emergency Weak takes priority by default; changing its trigger is an
explicit pressure-policy choice, not an accidental consequence of a wealth
rule. Each rule is a short **all of these** list. Two rules selecting the
same state mean **either rule**, so AND and OR need no nested expression
editor. Cap the list at eight rules with four conditions each, reject
unreachable later rules, and show priority clearly. No hidden score
multipliers or AI-name lists. An unconditional rule must be visibly marked.

Start the rule builder with gold, total troops, keep enclosed and relative
troop power. Put population, food, native score/class, role counts, active
attack and defense below quota under **More conditions**. These facts have
existing native owners. A relative power condition needs a clear comparison
scope: **all enemies**, **strongest opponent** or **current target**; a
missing opponent makes that condition false. Offer gold plus goods' sale value
only after verifying the native sale-price calculation and goods list. Only
compute facts referenced by loaded rules. For example, two Very Strong rules
could say "keep enclosed AND gold at least 10,000" and "own troop power at
least twice the strongest enemy." Either rule selects Very Strong. These
are examples, not default thresholds or supported AIC syntax.

Do not add a hold-time control before a measured weekly Extreme case shows
state flapping. This avoids extra saved state and keeps the first version
predictable: rules change the state at the next native decision.

## Effects: only author differences

Resolve each effect independently: explicit selected-state value, then an
explicit Normal override, then the existing AIC/native value. Preserve
explicit `false` and zero. Upper states fall back to the Strong AIC row for
fields that have native Weak/Default/Strong variants, and to the sole AIC
value for fields without variants. Do not copy inherited values into an AIC.

| Author intent | Minimal override | Decision boundary |
| --- | --- | --- |
| Recruitment priorities | Defense/Raid/Attack/Sortie weights; optional interval | Eligible native recruitment opportunity. Existing `RecruitConditions` wins as a situational rule; then state weights; then current AIC row. Its existing `Strength` condition retains native three-state meaning for compatibility. |
| Keep money in reserve | Existing `RecruitGoldThreshold` amount | Use the native spending decision. A separate spending-scope control requires traced recruitment, trade and siege owners; do not publish it as a simple switch. |
| Change raid size/risk | Existing `RaidUnitsBase/Random`; existing `RaidRiskTolerance` only with the opted-in raid policy | Sample when a raid group starts assembling; do not resize it mid-raid. |
| Change assault size | Existing `AttForceBase/Random`, and growth as **flat troops** or **percent of base** with one amount | Sample at wave creation, preserving native caps and team coordination. |
| Change whole-force mix | Shares of desired special attack roles; main troops take the remainder. Optional main-slot mix only if needed | Use native role counts and maxima at the existing attack recruit decision. Omission keeps the original role selector. |
| Change buying amounts | Existing `TradeAmountFood/Equipment` | When the native owner queues a purchase, not on every tick. |
| Change siege scale | Existing larger-force enable/maximum, where that independent feature is opted in | Sample at construction/assault admission; never append native composition slots. Defer a state-specific override until that feature and its state ownership are verified. |

An authored attack mix with 10% engineers and 5% laddermen means 85% main
troops before availability and native role maxima. The existing four main
unit slots still choose the main types. Ratios guide recruitment rather
than guarantee the final force. `AttForceBase` remains the native combat
readiness target; engineer capacity is derived separately. Reject mixes
with no positive combat-capable role. Keep attack-target commitment fixed
for an active attack even if the AI's effective state changes.

Existing opponent selection, retaliation, split-raid and next-wave controls
remain independent AIC settings. State rules do not create duplicate target
policies or retarget an active army. Show `RaidRiskTolerance` and group size
only when the opted-in raid policy uses them. Keep siege placement, role
counting, harassment, larger forces and construction payment as independent
per-AI controls with their documented module fallbacks; state effects can
change a verified quantitative limit, not silently enable a disabled feature.
Freeze wave and raid size/mix choices at their existing creation boundaries
so a monthly state change cannot reshape a force already in progress.

## Pressure responses stay distinct

Native `computeNervousness` checks **friendly** troop power near the keep and
may request help. Severe team weakness enters `aiNervousActionsTracker`,
which can force Weak, retreat and destroy buildings. Supply collapse uses a
different demolition tracker. Give authors separate warning/help and severe
pressure conditions using the same bounded rule builder; leave both native
by default. If no pressure rule is authored, native severe pressure can still
force Weak. Supply shortage gets its own independent choice. Expose response
switches only at the verified native owners, separating attack retreat, help
request and pressure demolition from supply demolition. An action switch
must not leave a native tracker half active.

The native friendly-defense count uses a square radius of 40 tiles and is
already refreshed at phase 93 of each 200-tick load-balancer cycle. A per-AI
defender radius belongs in that existing count. The hostile-home census uses
a separate circular 32-tile radius for retaliation; a configurable pressure
threat radius must not silently change retaliation. Offer hostile proximity
only if the existing census can supply it at bounded cost for opted-in AIs.
Do not add another unit or map scan. See
[native-pressure-investigation.md](native-pressure-investigation.md).

## Authoring and runtime cost

The Toolkit should offer two small builders: **When does this state apply?**
and **What changes in this state?** Put pressure under its own heading. Show
inherited native values as labels, and add an override only when the author
edits it. A preview with example
gold, troop power and keep status can show which ordered rule wins; this is
editor-only. Generate the module's machine-readable schema and concise help
from one metadata source, with titles/help in all nine UCP languages. A
simple example should teach one Normal mix and one stronger-state rule.
The editor must show whether a value comes from that state, Normal, the
existing AIC, or the module fallback; absence, explicit OFF and valid zero
must stay distinct through profile merging and serialization. Do not add
five full state pages full of copied defaults.

Compile bounded rules once at AIC load. Evaluate at the original monthly AI
strength decision (also weekly in Extreme), short-circuit cheap conditions,
and cache an expensive referenced fact at most once per AI decision. Attack
mix selection needs at most eleven role comparisons per eligible recruitment
choice. The native near-keep unit pass remains one pass; any per-unit radius
lookup must be measured. No per-frame rule engine, map search, new RNG or
second census. The native path must keep its original RNG consumption.

Persist only necessary wave snapshots through the current Map Extensions
section and integrity digest; native counters and trackers remain native
game state. Initialize fresh snapshots for a saved game started as a new map.
Benchmark 200-tick census cost and AI decision cost against native/missing-
policy baselines on normal Crusader and Extreme, then compare replay hashes.
These are acceptance requirements, not completed measurements.
