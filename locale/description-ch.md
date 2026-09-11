为 AIC 添加招募和进攻目标设置。未添加新设置时，AI 保持原有行为。

### 招募

- `RecruitPolicy`：`Native`（默认）沿用原有招募方式。`WeightedRoles` 在防御、袭扰、主力军队和出城迎敌之间分配招募。
- `RecruitProbSortieDefault`、`RecruitProbSortieWeak`、`RecruitProbSortieStrong`：AI 处于普通、较弱和较强状态时的出击权重。取 **0–100** 的整数，默认 **0**。与现有防御、袭扰和进攻权重相加，每档必须为 **100**。需要 `WeightedRoles`；兵种列表、招募间隔和配额继续生效。
- `RecruitConditions`：最多 **8** 条按顺序匹配的规则，默认为空。第一条匹配规则替代当前实力档位的权重。每条规则包含 `When` 和总和为 100 的四个权重：`Defense`、`Raid`、`Attack`、`Sortie`。需要 `WeightedRoles`。

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

- `Default`：新目标策略使用 `PerAttack`；搭配 `Inherit` 时沿用原有行为。
- `PerAttack`：整个进攻期间保持同一目标。
- `UntilDefeated`：只要对手仍是有效目标，多次进攻都针对该对手。

部分更新会保留未填写的值。恢复原有行为：`RecruitPolicy: Native`、`AttackTargetPolicy: Inherit`、`AttackTargetCommitment: Default`。先前的出击权重和规则仍会保存，但不生效；切换到 `Native` 时不要填写这些字段。
