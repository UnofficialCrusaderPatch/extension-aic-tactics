Control how your AI recruits troops and chooses attack targets.

Add the fields you want to your AIC configuration. Existing personalities keep their behavior without new settings.

### Attack targets

| `AttackTargetPolicy` | Choose… |
| --- | --- |
| `Inherit` (default) | Using the existing `TargetChoice`. |
| `LowestPopulation` | The opponent with the fewest civilians. |
| `FewestTroops` | The opponent with the fewest military units. |
| `LowestCombatPower` | The lowest estimated military strength, ignoring distance. |
| `Random` | Any eligible opponent with equal probability. |
| `LastAggressor` | The latest aggressor whose attack meets the retaliation criteria. |

`AttackTargetCommitment` controls how long to keep that opponent:

- `Default`: `PerAttack` for new target policies or `DuringAttack`; existing behavior otherwise.
- `PerAttack`: choose at launch and keep the target throughout that attack.
- `UntilDefeated`: keep the same opponent across attacks while it remains valid.

Random opponent per attack; the same opponent can be drawn again:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Recruitment

`RecruitPolicy` defaults to `Native`. Choose `WeightedRoles` to distribute recruitment between defense, raids, the main attack army and sorties from the castle. Troop lists, intervals and quotas still apply.

For **each** strength level (`Default`, `Weak`, `Strong`), the four integer weights must total **100**. `RecruitProbSortie…` defaults to 0. These are recruitment weights, not guaranteed army proportions.

Example: more defense when weak; prioritize defense while it is incomplete:

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
      "When": {
        "DefenseIncomplete": true
      },
      "Defense": 60, "Raid": 0, "Attack": 30, "Sortie": 10
    }
  ]
}
```

`RecruitConditions` is optional (default: `[]`, maximum eight rules). The first matching rule replaces the current strength row. Each rule needs all four weights totaling 100.

In `When`, use `Strength` to select a strength level, or these boolean conditions:

| Condition | Checks whether… |
| --- | --- |
| `HomeUnderThreat` | The home base is threatened. |
| `AttackActive` | An attack is underway. |
| `DefenseIncomplete` | The defense quota is not filled. |
| `EquipmentSurplus` | Spare equipment is available. |

`true` requires the condition; `false` requires its absence. All specified conditions must match. Omitted conditions are ignored; an empty `When` always matches.

### Changing settings

Apply related fields together. Partial updates retain omitted values; use `Default` to clear a previous `UntilDefeated`. To return both systems to existing behavior:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Previously set sortie weights and rules remain stored but inactive under `Native`; do not include them in that update. Setting sortie weight to 0 under `WeightedRoles` does not restore `Native`.

### Attack preparation and retaliation

- `AttackPreparation`: `Native` (default), or `DuringAttack` to recruit the next wave at home while the current army attacks. Existing troop limits and wave growth still apply.
- `AttackActivation`: `Immediate` (default), or `AfterProvocation` to wait for a qualifying attack before launching armies or raids. Local defensive sorties remain available.
- `ProvocationRules`: retaliation thresholds: `ThreatPower` **100**, `CombatTicks` **200**, `LossPower` **100**, `WindowTicks` **800** by default. Sustained fighting near the keep or enough military losses qualifies; damage to the lord qualifies immediately. Supply all four values together. **800 ticks = one game month**.

### Raids

- `RaidTargetPolicy`: `Native` (default), `NearestReachable` for nearby accessible buildings, or `Opportunistic` to consider distance, focus and danger.
- `RaidGroupCount`: **1–4**, default **1**. Divides the existing raid force; does not recruit extra troops.
- `RaidMinGroupSize`: **1–256**, default **4**. Smaller groups wait or combine.
- `RaidFocus`: `Any` (default), `Food`, `Industry` or `HighValue` (replacement cost). Requires `Opportunistic`.
- `RaidRiskTolerance`: `Low`, `Medium` (default) or `High`. Controls tolerance for nearby enemy troops and defenses.
- `RaidEnemyScope`: `PrimeTarget` (default) or `AnyEnemy`. Main-army targets stay unchanged.

The five raid settings require a new `RaidTargetPolicy`. Existing `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` and `RaidRetargetDelay` still apply. Return these systems to their defaults with `AttackPreparation: Native`, `AttackActivation: Immediate` and `RaidTargetPolicy: Native`.
