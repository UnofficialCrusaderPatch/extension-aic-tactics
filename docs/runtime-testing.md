# Recruitment integration test

Extract this ZIP into an isolated SHC 1.41 test installation with the UCP development runtime, then enable AIC Tactics and the included AIC Loader 1.1.3; require Legacy `ai_defense` ON and `ai_recruitinterval` OFF.
Merge `sortie-test-aic-fragment.json` into the `aic` object of a copied AI personality, keeping its existing sortie types/minima and recruitment intervals, then start a fresh game process and a spectator skirmish.
Check that this AI recruits only eligible sorties up to its existing minima, while unmodified personalities retain Native recruitment; a Wolf with Pikemen needs a barracks, gold, pikes and metal armor.

This is a development recruitment build. It includes actual native acquisition,
assignment, conditional role selection and equipment-purchase requests. The
private loader version identifies the transactional API from loader PR #19;
it is not an upstream release. Public release runtimes may reject these unsigned
modules. No game executable or development framework is bundled.

EquipmentSurplus is rejected. PreserveSlots, custom initial grace, next-wave
reserves, opponent policies/retaliation and split raids are not implemented by
this runtime yet. Do not use this build for multiplayer or historical recordings:
configuration fingerprints and the shared save/replay state contract remain open.
Reloaded unit membership has been sampled, but complete saved continuation has
not passed. A test reload showed black terrain patches; attribution is unresolved.

The compiled configuration stores authored character IDs 1..16; native player
characters use 2..17. Each player has separate observation counters. These counters
do not drive policy and are not save state. The launch log publishes their addresses
for read-only diagnostics. Native game state and the native RNG drive decisions.

Omitted RecruitPolicy uses Native. New role rows must total 100 and require
WeightedRoles. The first matching condition overrides the base strength row;
zero-weight roles are not probed or recruited. Missing equipment can create one
native purchase request per opportunity, in Defense/Raid/Attack/Sortie order,
using the existing purchase amount and nervous-recruitment rule. Purchase requests
do not recruit, spend gold directly, advance the roster cursor or consume RNG.

Supported condition facts in this development build are AttackActive (native
attack phase nonzero), DefenseIncomplete (current native defense count below its
quota) and HomeUnderThreat (native nervous-action tracker positive). The latter
still needs the package's qualifying-threat semantics before release.

Native-to-Native loader updates retain the loader's existing lifecycle. Changes
that enable or disable a new policy require a fresh process; unsynchronized live
policy changes are rejected. Legacy source is unchanged. Requiring recruitinterval
OFF preserves the declared baseline with that option OFF, but replacing an ON
baseline for old personalities remains an outstanding compatibility requirement.
