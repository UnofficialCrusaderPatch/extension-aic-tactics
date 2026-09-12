# AIC Tactics integration testing

Extract the integration ZIP into an isolated SHC 1.41 installation with the UCP development runtime, then enable the included AIC Loader 1.1.3, Map Extensions 1.1.0, Protocol 1.1.0, Chat 1.0.0 and AIC Tactics; apply the Legacy settings in `AIC-TACTICS-COMPATIBILITY.md`.
Merge one example fragment into the `aic` object of a copied personality and start a fresh process on the eight-player Green Haven spectator fixture, keeping neighbouring allied pairs and Vanilla Interpretation Castles.
Observe recruitment, target stability, a separate next-wave reserve and distinct raid targets; use the dependent recorder build for saved continuation and replay checks, and keep native memory/UID evidence for every claimed result.

The current source integrates all four subsystems, required saved state and
checkpoint digests. Its MSVC2005 build and component/native-layout checks pass;
it has not yet been accepted in the game. The
historical evidence below applies only to its named earlier revisions, not this
integration. See `combat-integration.md` for bounds and implementation details.

The test ZIP includes the unchanged, checksum-pinned store `luamemzip.dll` with
the proposed Map Extensions state API. Map Extensions 1.1.0 and the private loader
version are development prerequisites, not upstream release claims. No game or
framework binary is bundled. The recorder change is a separate dependent PR;
existing recorder versions must not be treated as compatible with this state API.

CI runs compilation and policy checks on pushes and PRs. Packaging currently
requires workflow dispatch with the published repository and full commit SHA of
the Map Extensions and Protocol prerequisites; it does not substitute older APIs.
The downloaded package must be tested separately before attaching acceptance
evidence to it.

For multiplayer, all peers need the same files, extension order and settings.
The host's first Start action checks every human peer; press Start again after
the replies arrive. A mismatch appears in the existing local chat display.
AIC settings freeze at this exchange; changing them requires a fresh process.
Single-player and replay do not run this lobby exchange.

The compiled configuration stores authored character IDs 1..16; native player
characters use 2..17. The launch log publishes configuration and observation
addresses for read-only inspection. Observations never drive decisions. The
reference sampler must use the current 344-byte ABI and validate unit/group UIDs.

Cover Native/default versus baseline, one opted-in AI, mixed repeated characters,
quota/equipment failures, target death/alliance change, no paths, reserve losses,
two handovers, raid regrouping, save/load, replay restore and measured performance.
Two physical MP peers and pre-match content/configuration admission are still
release gates. A local native fixture or parser test does not satisfy them.

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

The local MSVC2005 build at 858752d also exercised the interval replacement in
the real game. With `legacyRecruitInterval` ON, 24 snapshots showed the two
WeightedRoles Wolves' counters cycling through 0..3 for their authored interval
4; the six Native slots' counters stayed at 0. Stored AIC intervals were unchanged.
The Wolves hired four/seven sortie Pikemen with verified owners and membership.
This is a cadence check, not complete Native command/RNG equivalence.
DLL SHA256: `da2adec0630de96d70993e5977cae152718cc768121cac0b05d7b0e7dd656baa`.

The same build ran a 100% Raid test with Spearmen and `RaidUnitsBase=8`,
`RaidUnitsRandom=0`. Restart Mission preserved the neighbouring teams and
reinitialized native raid adjustments to -3/0. Both Wolves reached their effective
quotas of five/eight. At tick 4307, player 5's unit 509/UID 28870 had native role 2
and belonged to group 1229/UID 56582, size 8, with matching membership. Later
casualties and replacement recruitment were observed; at tick 5469 player 1's
latest recruit UID 102791 belonged to group 1241/UID 114333, size 3. A reused
player 5 unit ID correctly failed the diagnostic UID check.

Use a fresh match or Restart Mission when testing changed raid size parameters.
The native quota is the base plus a saved per-player adjustment; loading an old
save preserves that adjustment. Native initialization can reduce the quota when
the selected target has less than 500 gold, even with `RaidUnitsRandom=0`.
The extension retains this rule. An earlier test loaded old adjustments -2/-21,
producing quotas 6/-13; that is not an eight-unit acceptance fixture.

The fresh raid run was saved as `aicraid` and reloaded with the same configuration.
No black terrain patches appeared in the inspected view. This does not explain
the older sortie save's corruption or establish full saved-state equivalence.

Native team IDs and keep-entry coordinates confirmed neighbouring allied pairs:
players 1/2 Wolf/Caliph at (130,265)/(196,329); 3/4 Saladin/Pig at
(321,203)/(257,278); 5/6 Wolf/Richard at (77,195)/(141,130); 7/8 Saladin/Caliph
at (203,69)/(268,129). Distances are about 92, 99, 91 and 88 tiles, respectively.
These coordinates pin this saved fixture, not newly randomized games.

The Native slots bypassed policy observations; that alone is not a proof of native
decision/RNG equivalence. New target behaviour, reserves,
split raids, multiplayer, replay restoration and full-match performance have not
passed in-game acceptance.
