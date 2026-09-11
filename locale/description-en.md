AIC Tactics is being developed to give AI personality authors optional control over recruitment, attacks and raids.

Planned features include conditional recruitment and sortie probabilities, preparing the next army during an attack, opponent selection and retaliation, and raids split into a limited number of groups. Existing personalities are intended to keep their behavior unless their author explicitly enables a new policy.

New opponent policies are designed to keep the same target throughout an attack by default (`PerAttack`). Authors can choose `UntilDefeated` to retain that opponent across attacks. Unmodified personalities keep their existing behavior, including Legacy target stability.

**Under development: no gameplay-ready release is available yet.**

## Setup guide (development preview)

These are fields in an AI personality's AIC configuration, not new GUI controls. The examples explain the configuration under development; they are not complete AI packs or ready-to-play settings. Keep parameter names and values exactly as written, even with another GUI language.

### Keep existing behavior

For an untouched personality, omit the new fields. Defaults are `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit` and `AttackTargetCommitment: Default`. Recruitment and targeting can be enabled separately.

### Choose an opponent

`AttackTargetPolicy` has these intended meanings:

| Value | Meaning |
| --- | --- |
| `Inherit` | Use the personality's original `TargetChoice` (default). |
| `LowestPopulation` | Prefer the opponent with the fewest civilians. |
| `FewestTroops` | Prefer the opponent with the fewest military units. |
| `LowestCombatPower` | Prefer the lowest estimated military strength, without distance weighting. |
| `Random` | Give each eligible opponent an equal chance; repeats are possible. |
| `LastAggressor` | Respond to the latest qualifying hostile incident, not every hit. |

`AttackTargetCommitment: Default` means `PerAttack` for new policies: choose at attack launch and keep that target throughout the attack. With `Inherit`, it keeps existing Native/Legacy behavior. Choose `UntilDefeated` explicitly to keep a valid opponent across attacks. A target becoming invalid requires attack cleanup before selecting a replacement; the deployed army must not switch opponents mid-attack.

Example: random opponent for each attack, with no mid-attack switching:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Distribute recruitment

`RecruitPolicy: WeightedRoles` shares recruitment choices between defense, raids, the main attack army and sorties (troops that leave the castle to fight nearby enemies). Existing intervals, troop lists and quotas still apply. Weights affect eligible recruitment choices, not a guaranteed final army ratio.

Set all three strength rows: `Default`, `Weak` and `Strong`. Each row's four integer weights must total 100. The existing `RecruitProbDef…`, `RecruitProbRaid…` and `RecruitProbAttack…` fields are reused; `RecruitProbSortie…` adds sorties and defaults to 0. Values are never automatically rescaled. Example, not a balance recommendation:

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
  "RecruitProbSortieStrong": 10
}
```

### Conditional recruitment (advanced)

`RecruitConditions` defaults to an empty list. It accepts up to eight ordered rules. Each needs `When` and all four weights: `Defense`, `Raid`, `Attack`, `Sortie`, totaling 100. The first matching rule replaces the strength row. Otherwise that row applies.

In `When`, `Strength` selects `Default`, `Weak` or `Strong`. The yes/no checks are `HomeUnderThreat` (home threatened), `AttackActive` (attack underway), `DefenseIncomplete` (defense not filled) and `EquipmentSurplus` (spare equipment). `true` requires the condition, `false` requires its absence, and omitted checks are ignored. All supplied checks must match; an empty `When` always matches.

### Change or undo settings

Apply related fields together. Partial updates keep omitted values, including an earlier `UntilDefeated`; set commitment to `Default` to restore automatic behavior. To disable these policies, apply:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Switching recruitment to `Native` alone retains previously entered sortie weights and rules as inactive values. Do not set sortie fields or rules while using `Native`. A sortie weight of 0 in `WeightedRoles` does not restore Native recruitment.

**Still pending:** gameplay integration, exact native threat/surplus and opponent-counting definitions, retaliation thresholds, next-wave preparation, split raids and Legacy option enforcement. There is no release-ready Legacy switch recipe yet. These descriptions do not establish multiplayer, save or replay compatibility.
