# Opponent ranking and commitment component

This prerequisite implements bounded decisions, not installed game hooks.
Native metric acquisition, launch/request/cleanup integration, provocation
aggregation, loader registration and save/recorder admission are still required.
It must not be packaged as a gameplay-ready targeting feature.

## Defaults and precedence

`AttackTargetPolicy=Inherit` with omitted commitment takes the original path.
Historical TargetChoice values, including Any's nearest fallback and Balanced's
distance term, are not reinterpreted. Explicitly new opponent policies default
to **PerAttack** when commitment is omitted. Authors may explicitly choose
**UntilDefeated**. This follows the user's 11 September decision and supersedes
the draft handoff's native-omission sentence only for opted-in policies.

An explicit commitment can also use Inherit ranking. This changes commitment,
not the meaning or stored value of TargetChoice. Omission must remain distinct
from an explicitly authored commitment in configuration storage, getters and
migration; serializing an effective default as an authored value would change
the behavior when returning to Inherit. The configuration provider is not
implemented in this component.

PerAttack selects at each admitted launch and retains that opponent throughout
the attack, including early rally phases. The same opponent may win successive
selections. Finish clears that commitment. UntilDefeated retains a valid target
between waves; a newer incident or weaker opponent does not replace it.
Random + UntilDefeated initializes once after valid match diplomacy/player
setup, without launching an attack. Loading must restore state rather than
initialize it or draw again. Other policies first select at admitted launch.

An active target becoming ineligible or changing lord identity returns
`TargetNeedsCleanup`, retaining the old commitment for the native cleanup
owner. It never redirects the active army. The owner must finish that cleanup
before another LaunchAttack event. A caller must not use a phase-number change
alone as proof that deployed groups/survivors have finished cleanup.

## Ranking inputs

The native adapter supplies one coherent snapshot of player IDs 1 through 8.
Self is excluded internally. The adapter must exclude allies, dead/absent or
unsupported opponents, while including eligible human and AI players. It also
supplies the verified lord UID; a reused player slot/lord cannot silently
inherit an old commitment.

LowestPopulation, FewestTroops and LowestCombatPower minimize the supplied
nonnegative metric, with lower player ID winning a tie. There is no distance
input or distance tie-break. These component names do not establish native
counting semantics: population/military exclusions and the native combat-value
cache still require their own evidence and player-facing documentation.

Random uses an ascending eligible roster and the existing bounded-draw helper.
Zero or one eligible opponent consumes no RNG. LastAggressor chooses the
smallest age among already-qualified hostile incidents, breaking equal ages by
lower player ID. With none, it uses the supplied original ranking fallback.
This is not an incident detector: per-hit attribution, thresholds, aggregation,
expiration and wake state belong to the pending native incident owner.

AttackActivation remains independent. A dormant AfterProvocation caller must
not admit an offensive launch or raid. Initializing a random commitment does
not wake the AI. No sleeper or provocation behavior is implemented here.

## Native integration boundaries

The original selector at 0x4D4680 also calls computeNervousness and handles ally
requests before ranking. The existing Legacy ai_assaultswitch hook at 0x4D477B
holds an eligible current target for native attack states >=3, after those
request branches. Native states 1/2 can still retarget. Native personalities
must retain that behavior and the installed Legacy choice.

New commitments also require the updateAIPlayerState help-request path to
respect a deployed army: that branch can issue group orders without ordinary
ranking. A simple selector-entry return is not sufficient. Retain Legacy when
composition is proved; if replacing its patch boundary requires OFF, supply
the equivalent Native branch before enforcing it. Leave Legacy source alone.

LaunchAttack is a committed simulation event, not a periodic readiness poll.
The native readiness helper uses the current opponent's strength and mutates
coordination patience. Its admission/selection order needs explicit integration
proof: speculative calls or random redraws on every waiting update are not
permitted. Unreachable keeps must use native path failure and cleanup, not
hidden nearest-opponent substitution or repeated global path scans.

## State and validation

Each player's state contains target player ID, target lord UID and an active
flag, three integers (12 bytes on the required x86 ABI, 96 for eight players).
The existing persistence/recorder owners must serialize fields explicitly with
schema version admission and integrity coverage; never serialize compiler
padding or create a second save format. Duplicate personalities use separate
state instances. Match reset/personality changes must use the owning lifecycle
and reject unsafe live changes while troops remain deployed.

Decision work is bounded by the eight-player roster plus at most eight RNG
samples, with no allocation, world/path scan or private random state. Failures
leave target state unchanged. RNG errors may already have consumed native
samples and must be fatal before commands, as documented in bounded-random-draw.

Component checks cover all 256 roster eligibility masks and all selection
tickets, metric distinctions/ties, both commitment lifetimes, repeated targets,
lord identity/diplomacy invalidation, incident precedence, duplicate states,
malformed input and Native bypass. Copying a state in a component test is not
save/load or replay acceptance. No running-game, multiplayer or measured
full-match performance acceptance is claimed.
