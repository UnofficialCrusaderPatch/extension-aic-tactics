# Native damage observation adapter

This adapter observes one existing damage-owner call; it does not implement
damage, install hooks, emit game commands or determine whether an incident is
large enough to cause retaliation. The source is verified against SHC 1.41
SHA-256 `3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a`.
Other executable layouts, relocated/expanded pools and installed detours need
separate admission before this binding may be used.

## Provenance and filtering

`beginDamage` captures victim identity, owner, original type, position and
positive health immediately before the native invocation. The source owner
comes from the actual call's inputs:

| Native owner | Address | Inputs used |
|---|---|---|
| processUnitAttackOtherUnit | 0x531220 | First argument is attacker unit ID, second is victim ID; read the attacker's owner before the call |
| processEntityDamageToUnit | 0x531920 | First argument is victim ID, second is entity ID; read that entity's owner before the call |
| processFireDamageToUnit | 0x532460 | First argument is victim ID, second is the responsible player supplied by the native fire owner |

The native fire routine's third argument controls reduced damage, not ownership.
Owner 0 is unknown. Never infer a player from nearby units or a cached enemy:
native fire with responsible player 0 can leave the victim's old
lastEncounteredEnemyPlayerID unchanged. The adapter never reads that field.

Only distinct player IDs 1..8 on hostile teams are admitted. Already-dying or
nonpositive-health victims are ignored. The caller must supply valid native
pool bounds and live IDs at the original call boundary; this helper is not an
arbitrary memory inspection API. It bounds original SHC arrays at 2,500 units
and 3,000 entities, including their index-zero entries.

`finishDamage` consumes the probe once. It requires the same victim UID and
owner, continued hostility and a positive health decrease. Healing, unchanged
health, recycled unit IDs and ownership changes produce no observation. Health
loss is capped at the victim's positive pre-call health; death is the transition
from positive health to zero or less. The pre-call unit type is retained for
the existing native military-value owner, including the separate lord emergency
rule. A damage observation alone does not qualify an incident or wake an AI.

An ignored/invalid observation must never cancel, retry or replace native damage.
The production binding must bypass observation for unaffected Native players,
use stack-local probes on the simulation thread, and ensure each native call
has exactly one surrounding observation. No probe persists across a tick,
save/load, callback or asynchronous task. Persistent incident aggregates belong
to the existing admitted save/replay owner.

## Verification and limits

MSVC2005 SP1 x86 `/W4 /WX /O2` builds and runs the existing private reference
test host. It executes 37 original-instruction damage cases with synthetic
unit/player/entity state and explicit damage-table values, without service
stubs. Before each observed call it restores the exact native initial image;
afterward the complete original image and return value must equal the
unobserved native call. This covers incidental native queues as well as
victim health, RNG and other globals. A further 13 observation-only cases
check stale identity, ownership/diplomacy changes, healing and invalid bounds.
The 98 recruitment query cases still pass in the shared host.

The test host now places its complete code after the reserved original image.
This is necessary because damage calls the original percentage helper at
0x4092C0; the earlier recruitment-only layout occupied that address. Its test-only
linker flags merge host code after the `.aorigin` reservation and explicitly
permit execution. LNK4254 is suppressed for this intentional code/data merge.
Guards require the reservation at RVA 0x1000 and host main beyond the image.
Only this test executable's own reserved memory is overwritten, never an
unrelated process/allocation. The licensed reference bytes remain private and
ignored. Compiler/source/test hashes and results are in local evidence JSON.

These results do not establish running-game damage-hook composition, every
projectile/fire creator's owner propagation, building damage, home-area threat
metrics, sustained-combat/loss windows, sleeper behavior, multiplayer,
save/load/replay/restore or measured match overhead. Native binding and the
bounded incident owner remain required under issue #6. No balance defaults,
Legacy compatibility or gameplay-ready artifact are claimed here.
