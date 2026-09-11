# Target policy configuration

These fields join recruitment in the same exclusive `aic-tactics` loader
provider. This is configuration support, not a playable targeting release.
The target decision component is tracked in module PR #7 and the native
targeting/incident feature remains open in issue #6.

| Field | Accepted values | Omitted default |
|---|---|---|
| AttackTargetPolicy | Inherit, LowestPopulation, FewestTroops, LowestCombatPower, Random, LastAggressor | Inherit |
| AttackTargetCommitment | Default, PerAttack, UntilDefeated | Default |

`Default` restores automatic commitment. With a new opponent policy it resolves
to **PerAttack**; with Inherit it preserves **Native** commitment. It is the
explicit setter/reset value for omission, not a third target-lock lifetime.
This follows the user's decision that newly opted-in policies must keep an
active army's target. UntilDefeated must be chosen explicitly.

PerAttack chooses at an admitted launch and keeps the target through that
attack, including early rally. UntilDefeated retains a valid opponent between
waves. A new ranking winner or incident cannot divert a deployed army. Invalid
targets require native cleanup before a new attack. UntilDefeated's valid
commitment takes precedence over newer qualifying aggressors.

Inherit retains the original TargetChoice meaning and stored value. An explicit
PerAttack/UntilDefeated with Inherit changes only commitment; Inherit+Default
restores the original behavior, including the installed Legacy stability fix.
No original enum value is renamed or reused: Any is not the new Random policy,
and Balanced is not distance-free LowestCombatPower.

Examples:

```json
{"AttackTargetPolicy": "Random"}
```

This uses PerAttack and may legitimately select the same opponent twice.

```json
{"AttackTargetPolicy": "Random", "AttackTargetCommitment": "UntilDefeated"}
```

This initializes one opponent after valid match setup and retains it across
waves until invalid. Save/load must restore the commitment without a new draw.

```json
{"AttackTargetPolicy": "Inherit", "AttackTargetCommitment": "Default"}
```

This returns targeting to Native without editing original TargetChoice or
changing recruitment configuration. Examples are not installable AIC packs
until the production module and its declared dependencies are available.

## Partial updates, getters and reset

Updates retain omitted fields, so changing Random to LowestPopulation after
explicitly choosing UntilDefeated keeps UntilDefeated. Set commitment to Default
to restore the automatic lifetime. Getters return the authored/default token,
not its effective expansion; Default therefore round-trips without accidentally
making PerAttack explicit when returning to Inherit.

All seven recruitment/target fields are registered under one owner. A mixed
update validates both components before native or additional values commit;
backend or later-provider failure rolls them back together. An active target
policy participates in native-field updates so the backend can enforce match
admission. Target-only policies do not validate unused Native recruitment rows.
Fully Native configuration declines ordinary updates with no new fields.

Whole reset restores captured native defaults and clears both configurations
for that character only. It still requires every other registered additional
field owner to participate reversibly. Configuration remains per character;
commitments and incident/reserve/raid state must be separately owned per player.
Unsafe live resets/swaps with deployed troops must be rejected by the backend
before the transaction writes, not silently destroy or reassign those troops.

## Backend schema and compatibility

The aggregate compiled envelope is **schemaVersion 2**:
`{schemaVersion=2, recruitment=<version-1 record>, targeting=<version-1 record>}`.
No version-1 production backend has been released; the earlier draft provider
envelope is superseded, not a supported save migration format. A backend must
explicitly admit version 2 and the requested features, rejecting unsupported
capabilities before either owner writes.

Target policy numbers are Inherit=0, LowestPopulation=1, FewestTroops=2,
LowestCombatPower=3, Random=4, LastAggressor=5. Commitment numbers are
Default=0, PerAttack=1, UntilDefeated=2, matching the C++ decision component.
The backend receives a detached authored candidate and pure prepared reversible
operations. The provider owns no native hook, RNG, per-player runtime state,
Legacy switch, live synchronization mechanism or save format.

Provocation activation/rules are not registered yet: reliable native incident
provenance and measured units remain prerequisites. Metric names do not imply
verified native counting exclusions. No user-facing capability or package may
claim execution support merely because configuration validation passes.
