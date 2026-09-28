AIC Tactics lets each installed AI choose how it recruits, attacks, raids and uses siege engines. Set these fields in that AI's AIC. Recruitment, attack and raid policies are opt-in; siege fields inherit the module settings when omitted. Existing troop lists and limits still apply.

### Recruitment

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: sortie share in each strength state. Setting one enables four-way recruitment for this AI. Defense, raid, attack and sortie shares must total 100 in every state.
- `RecruitConditions`: optional situation-specific shares. For example, when home is threatened, recruit 70% defense, 20% attack and 10% sorties. The first matching rule applies to that recruitment choice.
- `RecruitPolicy`: optional compatibility switch. `Native` keeps the original selection; `WeightedRoles` explicitly enables four-way selection.
- `DefRecruitComposition`: `PreserveSlots` respects the proportions in `DefUnit1..8`; `Native` keeps existing selection.
- `RecruitInitialDefenseMonths`: how long raid and main-army recruitment waits for the defense quota. 0 disables the wait; default 6 months.

Sortie shares enable `WeightedRoles` automatically. Situation rules, defense composition and `RecruitInitialDefenseMonths` require it explicitly. Existing recruitment intervals and quotas still apply.

### Attacks and retaliation

- `AttackTargetPolicy`: `Inherit` uses `TargetChoice`. Choose `LowestPopulation`, `FewestTroops`, `LowestCombatPower`, `Random` or `LastAggressor` to keep one chosen target per attack. To keep it across attacks, use `{ "Choice": "LastAggressor", "UntilDefeated": true }`. That choice can also contain `"Provocation": { "LossPower": 200 }`; omitted thresholds keep their defaults. `InheritPerAttack` locks the original choice for one attack.
- `AttackTargetCommitment`: older AICs can keep using this field. For new AICs, choose the target and its duration together with `AttackTargetPolicy`.
- `AttackPreparation`: `DuringAttack` prepares the next wave at home while an army attacks; `Native` keeps existing timing.
- `AttackActivation`: `AfterProvocation` waits for a qualifying attack before launching armies or raids; `Immediate` does not wait. Defensive sorties remain available.
- `ProvocationRules`: older AICs, `AfterProvocation` and recruitment threat rules can use this shared threshold field. For `LastAggressor`, new AICs can place overrides inside `AttackTargetPolicy.Provocation`.

### Raids

- `RaidTargetPolicy`: `Native` keeps existing raids; `NearestReachable` picks nearby accessible buildings; `Opportunistic` weighs focus and danger; `RandomNearby` picks a reachable building at random, then clears nearby buildings before drawing again.
- `RaidGroupCount` (1–4) splits the existing raid force; `RaidMinGroupSize` (1–256) makes undersized groups wait or combine.
- `RaidFocus` can be `Any`, `Food`, `Industry` or `HighValue`, or percentages such as `{ "Food": 60, "Industry": 20 }`; the remainder uses any building. It works with `Opportunistic` and `RandomNearby`. `RaidRiskTolerance` sets `Low`, `Medium` or `High` danger tolerance. `RaidEnemyScope` chooses the main target or any enemy.

The five raid controls after `RaidTargetPolicy` require a new raid policy. Existing `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` and `RaidRetargetDelay` still apply.

### Siege and engineers

- `SafeSiegePlacement` avoids occupied building sites (module default ON).
- `ActualSiegeResourcePayment` requires construction materials and gold, buying shortages through normal AI trade (OFF).
- `CoordinatedSiegeHarassment` sends a gathered group of engines to reachable firing positions (OFF). `SiegeHarassMinEngines` sets its preferred minimum (0–20, default 3); `HarassingSiegeEnginesMax` remains the total limit.
- `LargerSiegeForces` cycles the configured assault equipment mix while living, unassigned attack engineers and valid sites remain (OFF). `SiegeForceMax` is an optional cap (0–64); 0 lets available engineers decide. `AttMaxEngineers` still limits crews.

Safe placement is enabled automatically with advanced siege behavior. Engineer role-quota counting is ON by default; Fixed Engineers owns general crew and equipment lifecycle.

Explicit AIC siege settings override module fallbacks.
