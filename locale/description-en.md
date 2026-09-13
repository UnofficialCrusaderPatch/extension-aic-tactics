Configure your AI’s recruitment, next-wave preparation, attack targets and raids through its AIC. Omitted settings keep existing behavior.

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

- `Default`: `PerAttack` for new target policies or `DuringAttack`; existing behavior otherwise.
- `PerAttack`: keep the chosen target throughout one attack.
- `UntilDefeated`: keep it across attacks while it remains a valid opponent.

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
