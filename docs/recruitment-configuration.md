# Recruitment configuration integration

The recruitment record remains schema version 1 inside the aggregate
personality envelope version 2. It implements authoring validation and the
atomic loader provider. It requires loader PRs
[18](https://github.com/UnofficialCrusaderPatch/extension-aicloader/pull/18) and
[19](https://github.com/UnofficialCrusaderPatch/extension-aicloader/pull/19),
and the calculation core in module PR #3. There is no module bootstrap or
gameplay-ready artifact yet. Native eligibility/facts, acquisition, RNG, save
and replay adapters remain required. Tests use the real loader with mocked
memory and a transactional backend; they do not establish native acceptance.

## Authored fields

| Field | Domain | Omitted default |
|---|---|---|
| `RecruitPolicy` | `Native`, `WeightedRoles` | `Native` |
| `RecruitProbSortieDefault` | Integer 0–100 | 0 |
| `RecruitProbSortieWeak` | Integer 0–100 | 0 |
| `RecruitProbSortieStrong` | Integer 0–100 | 0 |
| `RecruitConditions` | Dense ordered list of 0–8 complete rows | Empty list |

For each strength class, the base row combines the existing
`RecruitProbDef<State>`, `RecruitProbRaid<State>` and
`RecruitProbAttack<State>` with `RecruitProbSortie<State>`. All three base rows
must total 100 in WeightedRoles, including strength classes that have not yet
occurred. Existing values remain stored by aicloader; this provider does not
create a second authored copy. Native retains the original validation and
decision flow. WeightedRoles supersedes native three-role selection and
sortie-first priority; a sortie weight of zero is not equivalent to Native.

Each condition has `When`, `Defense`, `Raid`, `Attack`, and `Sortie`. The four
weights are required integer percentages totaling 100. `When` optionally
specifies `Strength` (`Default`, `Weak`, `Strong`) and booleans for
`HomeUnderThreat`, `AttackActive`, `DefenseIncomplete`, `EquipmentSurplus`.
All supplied predicates must match. False requires absence; omission ignores
that fact. The first matching row takes precedence, otherwise the current
strength's base row applies. An empty `When` matches every decision. Unknown
keys, sparse lists, missing weights, invalid types and later invalid rows
are rejected. Native definitions of threat and surplus are still investigation
gates; parsing those names does not advertise them as playable features.

```json
{
  "RecruitPolicy": "WeightedRoles",
  "RecruitProbDefDefault": 30,
  "RecruitProbRaidDefault": 20,
  "RecruitProbAttackDefault": 40,
  "RecruitProbSortieDefault": 10,
  "RecruitProbDefWeak": 60,
  "RecruitProbRaidWeak": 10,
  "RecruitProbAttackWeak": 20,
  "RecruitProbSortieWeak": 10,
  "RecruitProbDefStrong": 20,
  "RecruitProbRaidStrong": 20,
  "RecruitProbAttackStrong": 50,
  "RecruitProbSortieStrong": 10,
  "RecruitConditions": [
    {
      "When": {"HomeUnderThreat": true, "AttackActive": false},
      "Defense": 65, "Raid": 0, "Attack": 10, "Sortie": 25
    }
  ]
}
```

This is an authoring example, not tested balance or an installable AI pack.
Existing recruitment intervals, troop lists and quotas are not replaced by
these configuration fields. Eligible-role filtering belongs to the native
decision adapter and precedes the draw.

## Application, getters and rollback

The provider exclusively claims these five fields and the two target fields
under `aic-tactics`, sharing one transaction and backend candidate.
Field collisions abort registration and remove only fields registered by that
attempt. Configuration is stored separately for each AI character 1–16. It
contains no per-player group, commitment, RNG or incident state.

Use one `overwriteAIC` call for interdependent changes. The loader stages native
values first; validation reads those prospective values together with the new
additional values. A partial update retains omitted fields. Condition lists are
replaced as a whole. Active WeightedRoles also participates when only original
weights change, so a single-field setter cannot leave an invalid total. Invalid
updates fail before either owner commits. Failure after a backend commit restores
both owners through the loader's transaction; failed rollback is fatal.

Getters return stored authored values, with detached condition tables. Native
getters still return the original AIC storage, not effective conditional weights.
Fully Native updates without new fields decline this provider and retain the
loader's existing best-effort path, including other legacy additional handlers.

Selecting `RecruitPolicy=Native` alone disables this policy while retaining
previously authored sortie weights and conditions as dormant values. Explicitly
setting any of those subordinate fields in an update whose resulting policy is
Native is rejected, even for zero or an empty list. To resume WeightedRoles, the
whole current candidate is revalidated against current native values. A whole
`resetAIC` returns native fields to the loader's captured defaults and clears all
recruitment and target values for that character. This does not edit source files or
other characters. Atomic reset still requires other registered additional-field
owners to support the loader transaction contract.

## Native backend boundary

`config.provider.register(loader, backend)` requires
`backend.prepare(aiCharacter, detachedAuthored, compiled)` returning pure prepared
`commit()` and `rollback()` operations. `compiled` has schemaVersion 2, with
version-1 `recruitment` and `targeting` records. In `compiled.recruitment`, mode 0/1
matches Native/WeightedRoles; base rows use Default/Weak/Strong order and weights
use defense/raid/attack/sortie order. Conditions carry strength -1/0/1/2 and the
calculation core's required/forbidden fact masks. Native has no compiled base
rows, so validation does not reinterpret unusual supported original values.
The target record and reset/default rules are documented in
[target configuration](target-configuration.md).

The eventual backend must reject unsupported native capabilities and prohibited
live changes during preparation, before writes; prepare must not mutate native
state. It owns memory, admission and reversible updates. No backend is supplied
by this PR, and the test double's live-update rejection is not a production
synchronization mechanism. Persistence/admission must integrate with existing
map-extensions and recorder owners before release. No Legacy option is changed
or declared compatible by this provider.
