AIC Tactics 可分别调整每个已安装 AI 的招兵、进攻、袭扰和攻城器械。在该 AI 的 AIC 中填写下列字段。新的招兵、进攻和袭扰策略可按需启用；攻城字段省略时采用模块设置。原有兵种列表和数量上限仍然生效。

### 招兵

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`：各实力状态下的出击比例。设置任一项后，该 AI 使用四项招兵比例；每档的防御、袭扰、进攻和出击比例合计必须为 100。
- `RecruitConditions`：按情况调整比例。例如城堡受威胁时，70% 防御、20% 进攻、10% 出击。每次招兵采用第一条符合条件的规则。
- `RecruitPolicy`：可选的兼容设置。`Native` 沿用原有选择方式；`WeightedRoles` 明确启用四项比例。
- `DefRecruitComposition`：`PreserveSlots` 保留 `DefUnit1..8` 中各兵种的比例；`Native` 沿用原有选择。
- `RecruitInitialDefenseMonths`：防守人数未达配额时，袭扰和主力军等待的时间。设为 0 则不等待；默认 6 个月。

设置出击比例会自动启用 `WeightedRoles`。情境规则、防守兵种比例和 `RecruitInitialDefenseMonths` 仍需明确设置此模式。原有招兵间隔和配额继续生效。

### 进攻与反击

- `AttackTargetPolicy`：`Inherit` 使用 `TargetChoice`；`LowestPopulation`、`FewestTroops`、`LowestCombatPower`、`Random` 和 `LastAggressor` 各为一次进攻选定一个目标。
- `AttackTargetCommitment`：旧 AIC 仍可使用。新 AIC 可在 `AttackTargetPolicy` 中同时设置选敌方式和持续时间，例如 `{ "Choice": "LastAggressor", "UntilDefeated": true }`。
- `AttackPreparation`：`DuringAttack` 让军队进攻时在城内准备下一波；`Native` 沿用原有节奏。
- `AttackActivation`：`AfterProvocation` 等待达到反击条件的敌方进攻后再派出主力军或袭扰部队；`Immediate` 不等待。防御性出击仍可进行。
- `ProvocationRules`：旧 AIC、`AfterProvocation` 和招兵威胁规则仍可使用。新 AIC 可在 `AttackTargetPolicy.Provocation` 中设置 `LastAggressor` 的门槛。

### 袭扰

- `RaidTargetPolicy`：`Native` 沿用原有袭扰；`NearestReachable` 选择附近可到达的建筑；`Opportunistic` 考虑偏好和危险；`RandomNearby` 随机选一个可到达的建筑，再清理周边建筑。
- `RaidGroupCount`（1–4）拆分现有袭扰兵力；`RaidMinGroupSize`（1–256）让人数不足的小队等待或合并。
- `RaidFocus` 可选 `Any`、`Food`、`Industry`、`HighValue`，也可设比例，如 `{ "Food": 60, "Industry": 20 }`；剩余比例不限类别。适用于 `Opportunistic` 和 `RandomNearby`。`RaidRiskTolerance` 设置危险程度；`RaidEnemyScope` 设置敌方范围。

`RaidTargetPolicy` 后的五个字段需要新袭扰策略。原有 `RaidUnitsBase`, `RaidUnitsRandom`、`RaidUnit1..8` 和 `RaidRetargetDelay` 继续生效。

### 攻城与工程兵

- `SafeSiegePlacement` 避开已被占用的建造位置（模块默认开启）。
- `ActualSiegeResourcePayment` 要求材料和金币；AI 通过正常贸易购买缺少的资源（默认关闭）。
- `CoordinatedSiegeHarassment` 集结攻城器械，再前往可到达的射击位置（默认关闭）。`SiegeHarassMinEngines` 设置期望的最少数量（0–20，默认 3）；`HarassingSiegeEnginesMax` 仍限制总数。
- `LargerSiegeForces` 在仍有存活且未分配的进攻工程兵和有效建造地点时循环使用器械配置（默认关闭）。`SiegeForceMax` 可选数量上限（0–64）；0 由可用工程兵决定。`AttMaxEngineers` 仍限制操作人员。

启用高级攻城功能时，安全选址自动生效。工程兵角色计数默认开启；Fixed Engineers 负责器械和操作人员的生命周期。

攻城 AIC 设置优先于模块默认值。
