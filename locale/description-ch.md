通过 AIC 设置 AI 的招募、下一波备战、进攻目标和袭扰。未设置的新选项保留原有行为。

### 招募

- `RecruitPolicy`：`Native`（默认）沿用原有招募方式。`WeightedRoles` 在防御、袭扰、主力军队和出城迎敌之间分配招募。
- `RecruitProbSortieDefault`、`RecruitProbSortieWeak`、`RecruitProbSortieStrong`：AI 处于普通、较弱和较强状态时的出击权重。取 **0–100** 的整数，默认 **0**。与现有防御、袭扰和进攻权重相加，每档必须为 **100**。需要 `WeightedRoles`；兵种列表、招募间隔和配额继续生效。
- `RecruitConditions`：最多 **8** 条按顺序匹配的规则，默认为空。第一条匹配规则替代当前实力档位的权重。每条规则包含 `When` 和总和为 100 的四个权重：`Defense`、`Raid`、`Attack`、`Sortie`。需要 `WeightedRoles`。
- `DefRecruitComposition`：`Native`（默认）沿用原有行为。`PreserveSlots` 按 `DefUnit1..8` 的条目保留兵种份额；重复条目会增加该兵种的份额。缺少装备时，相应名额保持空缺。需要 `WeightedRoles`。
- `RecruitInitialDefenseMonths`：**0–30** 个月，默认 **6**。在此期间，若防御兵力未满配额，暂停招募袭扰部队和主力军。仍可招募出击部队；权重为 0 的角色仍不招募。**0** 关闭等待期。需要 `WeightedRoles`。

`When` 可检查 `Strength`（`Default`、`Weak`、`Strong`）、`HomeUnderThreat`（基地受威胁）、`AttackActive`（进攻进行中）、`DefenseIncomplete`（防御兵力未补齐）和 `EquipmentSurplus`（有多余装备）。所有已填写条件必须匹配；`true` 要求条件成立，`false` 要求不成立。空的 `When` 始终匹配。

### 进攻目标

`AttackTargetPolicy`:

- `Inherit`（默认）: 沿用现有 `TargetChoice`。
- `LowestPopulation`: 平民最少的对手。
- `FewestTroops`: 军事单位最少的对手。
- `LowestCombatPower`: 估计军事实力最低的对手，不考虑距离。
- `Random`: 从符合条件的对手中等概率随机选择。
- `LastAggressor`: 最近一名符合反击条件的进攻者。

`AttackTargetCommitment`：

- `Default`：新目标策略或 `DuringAttack` 使用 `PerAttack`；其他情况保留原有行为。
- `PerAttack`：整个进攻期间保持同一目标。
- `UntilDefeated`：只要对手仍是有效目标，多次进攻都针对该对手。

### 备战与反击

- `AttackPreparation`：默认 `Native`；`DuringAttack` 允许当前军队进攻时在城内招募下一波兵力。原有兵力上限和波次增长仍然有效。
- `AttackActivation`：默认 `Immediate`；`AfterProvocation` 在遭受达到反击条件的攻击后才发动主力进攻或袭扰。防御性出击仍然可用。
- `ProvocationRules`：反击阈值，默认 `ThreatPower` **100**、`CombatTicks` **200**、`LossPower` **100**、`WindowTicks` **800**。主堡附近持续交战或足够的军队损失可触发反击；领主受到伤害立即满足条件。修改时须同时提供四个值。**800 tick = 一个游戏月**。

### 袭扰

- `RaidTargetPolicy`：默认 `Native`；`NearestReachable` 选择附近可到达的建筑；`Opportunistic` 综合距离、偏好和危险程度。
- `RaidGroupCount`：**1–4**，默认 **1**。拆分现有袭扰部队，不额外招兵。
- `RaidMinGroupSize`：**1–256**，默认 **4**。人数不足的小队等待或合并。
- `RaidFocus`：`Any`（默认）、`Food`（食物）、`Industry`（生产）或 `HighValue`（重建成本）。需要 `Opportunistic`。
- `RaidRiskTolerance`：`Low`、`Medium`（默认）或 `High`，控制对附近敌军和防御设施的风险容忍度。
- `RaidEnemyScope`：`PrimeTarget`（默认）或 `AnyEnemy`，不改变主力军队的目标。

以上五项设置需要新的 `RaidTargetPolicy`。`RaidUnitsBase`、`RaidUnitsRandom`、`RaidUnit1..8` 和 `RaidRetargetDelay` 仍然有效。恢复默认：`AttackPreparation: Native`、`AttackActivation: Immediate`、`RaidTargetPolicy: Native`。
