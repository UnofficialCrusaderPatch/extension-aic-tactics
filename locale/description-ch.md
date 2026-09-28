AIC Tactics 可分别调整每个已安装 AI 的招兵、进攻、袭扰和攻城器械。在该 AI 的 AIC 中填写下列字段。新的招兵、进攻和袭扰策略可按需启用；攻城和工程兵字段省略时采用模块设置。原有兵种列表和数量上限仍然生效。

### 招兵

- `RecruitPolicy`：`Native` 沿用原有招兵方式；`WeightedRoles` 将新兵分配给防御、袭扰、主力军和出城迎敌。
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`：原有三档实力状态下的出击比例。每档的防御、袭扰、进攻和出击比例合计必须为 100。
- `RecruitConditions`：最多 8 条按顺序检查的规则；第一条符合的规则会替换本次招兵比例。`When` 可检查 `Strength`、`HomeUnderThreat`、`AttackActive`、`DefenseIncomplete` 和 `EquipmentSurplus`。
- `DefRecruitComposition`：`PreserveSlots` 保留 `DefUnit1..8` 中各兵种的比例；`Native` 沿用原有选择。
- `RecruitInitialDefenseMonths`：防守人数未达配额时，袭扰和主力军等待的时间。设为 0 则不等待；默认 6 个月。

出击比例、情境规则、防守兵种比例和初期等待时间都需要 `WeightedRoles`。原有招兵间隔和配额继续生效。

### 进攻与反击

- `AttackTargetPolicy`：`Inherit` 使用 `TargetChoice`；其他选项分别选择平民最少（`LowestPopulation`）、士兵最少（`FewestTroops`）、军力最弱（`LowestCombatPower`）、随机对手（`Random`）或最近符合反击条件的敌人（`LastAggressor`）。
- `AttackTargetCommitment`：`PerAttack` 在一次进攻中保持目标；`UntilDefeated` 在多次进攻间保持目标。`Default` 在使用新选敌规则或提前备战时采用 `PerAttack`，否则沿用原有行为。
- `AttackPreparation`：`DuringAttack` 让军队进攻时在城内准备下一波；`Native` 沿用原有节奏。
- `AttackActivation`：`AfterProvocation` 等待达到反击条件的敌方进攻后再派出主力军或袭扰部队；`Immediate` 不等待。防御性出击仍可进行。
- `ProvocationRules`：`ThreatPower` 和 `CombatTicks` 决定主堡附近的威胁条件；`LossPower` 和 `WindowTicks` 决定损失与统计时间。领主受伤会立即触发。四个值需一起填写。

### 袭扰

- `RaidTargetPolicy`：`Native` 沿用原有袭扰；`NearestReachable` 选择附近可到达的建筑；`Opportunistic` 还会考虑目标偏好和危险。
- `RaidGroupCount`（1–4）拆分现有袭扰兵力；`RaidMinGroupSize`（1–256）让人数不足的小队等待或合并。
- `RaidFocus` 可选 `Any`、`Food`、`Industry` 或 `HighValue`，需要 `Opportunistic`。`RaidRiskTolerance` 设置可接受的危险程度（`Low`、`Medium`、`High`）。`RaidEnemyScope` 决定只袭扰主目标还是任何敌人。

`RaidTargetPolicy` 后的五个字段需要新袭扰策略。原有 `RaidUnitsBase`, `RaidUnitsRandom`、`RaidUnit1..8` 和 `RaidRetargetDelay` 继续生效。

### 攻城与工程兵

- `SafeSiegePlacement` 避开已被占用的建造位置（模块默认开启）。
- `ActualSiegeResourcePayment` 要求材料和金币；AI 通过正常贸易购买缺少的资源（默认关闭）。
- `CoordinatedSiegeHarassment` 集结攻城器械，再前往可到达的射击位置（默认关闭）。`SiegeHarassMinEngines` 设置期望的最少数量（0–20，默认 3）；`HarassingSiegeEnginesMax` 仍限制总数。
- `LargerSiegeForces` 为主攻重复建造设定的器械组合（默认关闭）。`SiegeForceMax` 限制现有和待建器械（0–20，默认 10）；0 保留原有的一轮建造。`AttMaxEngineers` 仍限制操作人员。
- `CorrectEngineerRoleCounting` 将已分配的工程兵计入对应部队配额；攻城和油锅任务单独计算（默认开启）。

攻城与工程兵选项中，AIC 明确填写的值（包括 `false` 和 0）优先于模块设置；省略时采用模块设置。
