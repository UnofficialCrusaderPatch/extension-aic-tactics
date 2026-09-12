# Recruitment integration test

[Download the GamerGrill-tested sortie build at 33d91be](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/actions/runs/34661062781/artifacts/10287381748)
(CI artifact expires 2 October 2026). Newer CI artifacts include subsequent changes;
the verification below identifies the tested source and DLL separately.

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
native purchase request per attempt (at most four at the native opportunity), in Defense/Raid/Attack/Sortie order,
using the existing purchase amount and nervous-recruitment rule. Purchase requests
do not recruit, spend gold directly, advance the roster cursor or consume RNG.

Recruitment buildings are checked against the native building pool, live state,
owner and required type on every probe. Cached IDs are not accepted after removal
or reuse as a different building. This follows the native building lookup's
validity predicate; no additional persistent building cache is introduced.

`AttMaxDefault` retains its native subrole-selection meaning: it is not a hard
army-size cap. Once no special attack subrole has an open quota, native selection
falls back to the main roster even above this value. An unavailable special
subrole with an open quota does not enable that fallback. Wave size/growth and
their Legacy limits remain separate; reserve preparation must preserve them.

Supported condition facts in this development build are AttackActive (native
attack phase nonzero), DefenseIncomplete (current native defense count below its
quota) and HomeUnderThreat (native nervous-action tracker positive). The latter
still needs the package's qualifying-threat semantics before release.

Native-to-Native loader updates retain the loader's existing lifecycle. Changes
that enable or disable a new policy require a fresh process; unsynchronized live
policy changes are rejected. Legacy source is unchanged. Requiring recruitinterval
OFF preserves the declared baseline with that option OFF, but replacing an ON
baseline for old personalities remains an outstanding compatibility requirement.

## GamerGrill evidence, 12 September

The exact downloaded 33d91be artifact ran in the eight-AI Green Haven spectator
fixture, with Aggressive AI Behaviour and Vanilla Interpretation Castles. The two
Wolves used Defense/Raid/Attack/Sortie = 0/0/0/100; the other six slots were Native.
ReadProcessMemory verified the effective rows and four/eight successful Pikeman
hires. Sampled player 1 unit 131/UID 77239 belonged to group 1241/UID 78129 (size 4);
player 5 unit 716/UID 64932 belonged to group 1229/UID 63238 (size 8). Both had
native role 7, the correct owner and matching group membership bits.

Inner ZIP SHA256: `cc7f45a0cae3e7b3072c3ec3c3d9b7de16e0af76ef323a6d36c7fcfe66704ef0`.
DLL SHA256: `012689c509700edc3eb00ee27bf1f6750fedf78a92a76cd575fd4303df6dec0f`.
Fresh-process loading restored native sortie counts four/eight. Black terrain
patches persisted after load; this is not full save/load acceptance.

The MSVC2005 build at 6ad77da additionally passed a real conditional recruitment
test: DefenseIncomplete selects 100% Defense; otherwise the base row selects
100% Attack. One Wolf hired three defenders, reached 32 defenders, then recruited
attack Spearmen. Sampled attack units had native role 20, correct owners, UIDs and
membership bits. This does not verify the later open-subrole fallback correction,
which has original-instruction regression coverage only.
DLL SHA256: `04e2fe6f731f92f8b3f0939896497f4f2d802789f3938079acacccea08a4c873`.

Native team IDs and keep-entry coordinates confirmed neighbouring allied pairs:
players 1/2 Wolf/Caliph at (130,265)/(196,329); 3/4 Saladin/Pig at
(321,203)/(257,278); 5/6 Wolf/Richard at (77,195)/(141,130); 7/8 Saladin/Caliph
at (203,69)/(268,129). Distances are about 92, 99, 91 and 88 tiles, respectively.
These coordinates pin this saved fixture, not newly randomized games.

The Native slots bypassed policy observations; that alone is not a proof of native
decision/RNG equivalence. Raid recruitment, new target behaviour, reserves,
split raids, multiplayer, replay restoration and full-match performance have not
passed in-game acceptance.
