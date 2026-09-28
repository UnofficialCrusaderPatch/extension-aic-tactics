AIC Tactics lets each installed AI choose how it recruits, attacks, raids and uses siege engines. Set these fields in that AI's AIC. Recruitment, attack and raid policies are opt-in; siege and engineer fields inherit the module settings when omitted. Existing troop lists and limits still apply.

### Recruitment

- `RecruitPolicy`: `Native` keeps existing recruitment. `WeightedRoles` divides recruits between defense, raids, the main army and sorties.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: sortie share in each native strength state. Each row's defense, raid, attack and sortie shares must total 100.
- `RecruitConditions`: up to 8 ordered rules that replace those shares for one recruitment choice. `When` can check `Strength`, `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` and `EquipmentSurplus`; the first matching rule wins.
- `DefRecruitComposition`: `PreserveSlots` respects the proportions in `DefUnit1..8`; `Native` keeps existing selection.
- `RecruitInitialDefenseMonths`: how long raid and main-army recruitment waits for the defense quota. 0 disables the wait; default 6 months.

Sortie shares, situation rules, defense composition and the initial defense period require `WeightedRoles`. Existing recruitment intervals and quotas still apply.

### Attacks and retaliation

- `AttackTargetPolicy`: `Inherit` uses `TargetChoice`. The other choices favor fewer civilians (`LowestPopulation`), fewer troops (`FewestTroops`), weaker military power (`LowestCombatPower`), a random opponent (`Random`) or the latest qualifying attacker (`LastAggressor`).
- `AttackTargetCommitment`: `PerAttack` keeps one target during an attack; `UntilDefeated` keeps it across attacks. `Default` uses `PerAttack` with a new target policy or next-wave preparation, and existing behavior otherwise.
- `AttackPreparation`: `DuringAttack` prepares the next wave at home while an army attacks; `Native` keeps existing timing.
- `AttackActivation`: `AfterProvocation` waits for a qualifying attack before launching armies or raids; `Immediate` does not wait. Defensive sorties remain available.
- `ProvocationRules`: `ThreatPower` and `CombatTicks` set the threat near the keep; `LossPower` and `WindowTicks` set the losses and time window. Lord damage qualifies immediately. Supply all four values together.

### Raids

- `RaidTargetPolicy`: `Native` keeps existing raids; `NearestReachable` chooses accessible nearby buildings; `Opportunistic` also weighs focus and danger.
- `RaidGroupCount` (1–4) splits the existing raid force; `RaidMinGroupSize` (1–256) makes undersized groups wait or combine.
- `RaidFocus` chooses `Any`, `Food`, `Industry` or `HighValue` buildings; it requires `Opportunistic`. `RaidRiskTolerance` sets `Low`, `Medium` or `High` danger tolerance. `RaidEnemyScope` chooses the main target or any enemy.

The five raid controls after `RaidTargetPolicy` require a new raid policy. Existing `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` and `RaidRetargetDelay` still apply.

### Siege and engineers

- `SafeSiegePlacement` avoids occupied building sites (module default ON).
- `ActualSiegeResourcePayment` requires construction materials and gold, buying shortages through normal AI trade (OFF).
- `CoordinatedSiegeHarassment` sends a gathered group of engines to reachable firing positions (OFF). `SiegeHarassMinEngines` sets its preferred minimum (0–20, default 3); `HarassingSiegeEnginesMax` remains the total limit.
- `LargerSiegeForces` repeats the configured assault equipment mix (OFF). `SiegeForceMax` caps active and pending engines (0–20, default 10); 0 keeps one native batch. `AttMaxEngineers` still limits crews.
- `CorrectEngineerRoleCounting` counts assigned engineers toward their troop quotas; siege and oil duties remain separate (ON).

For siege and engineer options, an explicit AIC value—including `false` or 0—overrides the module setting. Omit it to inherit that setting.
