AIC Tactics 正在开发中，旨在让 AI 个性的作者按需配置募兵、进攻和袭扰规则。

计划提供的功能包括：根据战况调整募兵和出城迎敌的概率、进攻期间筹备下一支军队、选择对手与实施报复，以及将袭扰部队拆分为数量有限的小组。除非作者明确启用新规则，现有 AI 个性应保持原有行为。

新对手选择规则的默认设计是在整次进攻期间保持同一目标（`PerAttack`）。作者可选择 `UntilDefeated`，在多次进攻之间继续锁定该对手。未修改的 AI 个性保持原有行为，包括 Legacy 模块防止进攻中途更换目标的修正。

**开发中：目前尚无可供游玩的版本。**

## 设置指南（开发预览）

这些是 AI 个性的 AIC 配置字段，不是新增的 GUI 控件。示例说明的是开发中的配置，并非完整 AI 包，也不能直接用于游戏。无论 GUI 使用哪种语言，参数名称和值都必须保持原样。

### 保留现有行为

对于未修改的个性，不要添加新字段。默认值为 `RecruitPolicy: Native`、`AttackTargetPolicy: Inherit` 和 `AttackTargetCommitment: Default`。招募和目标选择可以分别启用。

### 选择对手

`AttackTargetPolicy` 的预期含义如下：

| 值 | 含义 |
| --- | --- |
| `Inherit` | 使用个性原有的 `TargetChoice`（默认）。 |
| `LowestPopulation` | 优先选择平民最少的对手。 |
| `FewestTroops` | 优先选择军事单位最少的对手。 |
| `LowestCombatPower` | 优先选择估计军事实力最低的对手，不按距离加权。 |
| `Random` | 每个符合条件的对手被选中的概率相同；允许重复选中。 |
| `LastAggressor` | 对最近一次符合条件的敌对事件作出回应，而不是对每次命中作出回应。 |

对于新的目标策略，`AttackTargetCommitment: Default` 表示 `PerAttack`：在进攻开始时选择目标，并在整个进攻期间保持不变。搭配 `Inherit` 时，保留现有 Native/Legacy 行为。若要在多次进攻之间保留同一个有效对手，须明确选择 `UntilDefeated`。目标失效时，必须先完成当前进攻的收尾处理，才能选择替代目标；已出征的军队不得在进攻中途更换对手。

示例：每次进攻随机选择对手，进攻中途不更换：

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### 分配招募

`RecruitPolicy: WeightedRoles` 在防御、袭扰、主攻军队和出击之间分配招募选择。出击指部队离开城堡，与附近敌人作战。原有招募间隔、兵种列表和配额继续生效。权重影响符合条件的招募选择，不保证最终军队的固定比例。

必须设置全部三个实力档位：`Default`、`Weak`、`Strong`。每档的四个整数权重之和必须为 100。沿用 `RecruitProbDef…`、`RecruitProbRaid…`、`RecruitProbAttack…`；新增的 `RecruitProbSortie…` 用于出击，默认为 0。不会自动换算权重。以下仅为示例，不是平衡性建议：

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

### 条件招募（高级）

`RecruitConditions` 默认为空列表，最多允许八条按顺序排列的规则。每条规则都需要 `When` 以及 `Defense`、`Raid`、`Attack`、`Sortie` 四个权重，总和为 100。第一条匹配规则替代当前实力档位的权重行；若无匹配，则使用该档位的权重行。

在 `When` 中，`Strength` 可选 `Default`、`Weak` 或 `Strong`。布尔条件包括 `HomeUnderThreat`（基地受威胁）、`AttackActive`（进攻进行中）、`DefenseIncomplete`（防御兵力尚未补齐）、`EquipmentSurplus`（装备有富余）。`true` 要求条件成立，`false` 要求条件不成立，省略的条件不参与判断。所有已填写条件都必须匹配；空的 `When` 始终匹配。

### 修改或撤销设置

将相互关联的字段一起应用。部分更新会保留省略的值，包括先前设置的 `UntilDefeated`；要恢复自动目标保持方式，请设为 `Default`。要停用这些策略，请应用：

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

仅切换到 `RecruitPolicy: Native` 会保留先前填写的出击权重和规则，但不启用它们。使用 `Native` 时不要设置出击字段或规则。在 `WeightedRoles` 下把出击权重设为 0，并不能恢复 Native 招募行为。

**尚待完成：** 游戏集成、原生威胁与装备富余的精确定义、对手统计口径、反击阈值、下一波准备、分组袭扰和 Legacy 选项强制约束。目前尚无可供正式发布使用的 Legacy 开关配置方案。这些说明不代表已经验证多人游戏、存档或回放兼容性。
