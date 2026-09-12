# EquipmentSurplus

`When.EquipmentSurplus: true` matches when the AI can recruit at least one
European troop type from its AIC rosters using a complete equipment set left
over after its home-defense needs. `false` matches the absence of that surplus.
This is a condition, not a new stock limit or an instruction to buy equipment.

Home needs include the remaining `DefTotal` quota, including its existing native
multipliers, and both sortie minima, including the native ranged adjustment.
Defenders and sorties already recruited in the current opportunity reduce these
deficits. Each resource is accounted for separately: spare pikes without spare
metal armor do not make a Pikeman kit. Knights also need a spare horse.

With `PreserveSlots`, missing shares are counted by troop type. With `Native`
composition, any allowed defender type might fill the remaining places; the
calculation conservatively allows the full deficit for every resource that
could be needed. Thus mixed Native rosters can require more stock before this
condition becomes true. No equipment is actually earmarked or removed.

Candidate types come from `DefUnit1..8`, `RaidUnit1..8`, the four main attack
entries, sortie types and the native attack subrole type selector. Lists stop at
their first empty entry, as in recruitment. Each European type is checked once,
in native type order. Gold-only troops and equipment for unauthored types cannot
establish surplus. The normal acquisition owner checks gold, available peasants,
equipment, horses and restrictions; the module validates the recruitment
building's owner/type/lifetime. Actual role selection still checks quotas,
group admission, grace and the current attack phase.

The predicate only reserves home-defense needs. It does not promise enough
equipment to complete an entire raid force or the next main army. Use it to
shift recruitment toward an offensive role when a usable kit is available
beyond defense needs. Probabilities and normal recruitment cadence still apply.

## Native integration and bounds

The resource recipe is read from the original European acquisition table at
`0xB55260`, seven rows of four integers. Offsets 0/4/8 are resource IDs; offset
12 identifies the horse requirement. The check uses the same table as
`0x52E960`, not a copied weapon-to-unit mapping. Intermediate demand arithmetic
is 64-bit. Invalid recipes or a stale required composition census yield false.

No new state, hook, RNG stream or save format is introduced. The predicate is
derived from the current simulation snapshot and is recalculated after a hire.
It runs only when the personality includes an EquipmentSurplus condition.
There are at most seven native check-only acquisition calls per opportunity
attempt; their diagnostics and horse-cache writes are restored. Existing
native queries may scan for a peasant or refresh horses. There are no new world
scans, allocations, resource purchases or additional recruitment opportunities.
The cost still requires full-match measurement before performance acceptance.

## Tests and remaining acceptance

The actual MSVC2005 runtime executes original acquisition instructions against
controlled native state. Cases cover all six unmounted European types, exact
home-quota boundaries, missing armor, gold and peasants, foreign barracks,
unrelated equipment, very large quotas, available/reserved horses, and
PreserveSlots with valid/stale counts. Tests check facts, first-match selection,
RNG, native diagnostics and absence of purchases. Loader tests cover true and
false condition masks. This is component evidence, not running-game, MP,
saved-continuation, replay or measured-performance acceptance.
