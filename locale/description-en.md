Adds recruitment and attack-target settings to your AI's AIC. Without new settings, existing behavior is preserved.

### Recruitment

- `RecruitPolicy`: `Native` (default) keeps existing recruitment. `WeightedRoles` distributes it between defense, raids, the main army and sorties.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: sortie weights for normal, weak and strong AI states. Integers **0–100**, default **0**. Together with the existing defense, raid and attack weights, each strength row must total **100**. Requires `WeightedRoles`; troop lists, intervals and quotas still apply.
- `RecruitConditions`: up to **8** ordered rules, empty by default. The first matching rule overrides the strength row. Each rule contains `When` and four weights totaling 100: `Defense`, `Raid`, `Attack`, `Sortie`. Requires `WeightedRoles`.
- `DefRecruitComposition`: `Native` (default) keeps existing behavior. `PreserveSlots` keeps each `DefUnit1..8` entry’s share; repeated entries increase that type’s share. Missing equipment leaves those places open. Requires `WeightedRoles`.
- `RecruitInitialDefenseMonths`: **0–30** months, default **6**. During this period, defer raid and main-army recruitment while defense is below quota. Sorties remain allowed; zero-weight roles stay disabled. **0** disables the period. Requires `WeightedRoles`.

`When` can check `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (base threatened), `AttackActive` (attack underway), `DefenseIncomplete` (defense below quota) and `EquipmentSurplus` (spare equipment). All specified checks must match; `true` requires the condition, `false` its absence. An empty `When` always matches.

### Attack targets

`AttackTargetPolicy`:

- `Inherit` (default): Existing `TargetChoice`.
- `LowestPopulation`: Fewest civilians.
- `FewestTroops`: Fewest military units.
- `LowestCombatPower`: Lowest estimated military strength, ignoring distance.
- `Random`: An eligible opponent at random, with equal chances.
- `LastAggressor`: Latest attacker meeting the retaliation criteria.

`AttackTargetCommitment`:

- `Default`: `PerAttack` for new target policies; existing behavior with `Inherit`.
- `PerAttack`: keep the chosen target throughout one attack.
- `UntilDefeated`: keep it across attacks while it remains a valid opponent.

Partial updates keep omitted values. Return to existing behavior with `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit` and `AttackTargetCommitment: Default`. Previously configured sortie weights and rules remain inactive; omit them from the switch to `Native`.
