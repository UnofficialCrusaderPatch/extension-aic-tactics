设置 AI 的招募方式和进攻目标。

将需要的字段加入 AIC 配置。未添加新设置的 AI 个性保持原有行为。

### 进攻目标

| `AttackTargetPolicy` | 选择方式 |
| --- | --- |
| `Inherit`（默认） | 沿用现有的 `TargetChoice`。 |
| `LowestPopulation` | 选择平民最少的对手。 |
| `FewestTroops` | 选择军事单位最少的对手。 |
| `LowestCombatPower` | 选择估计军事实力最低的对手，不考虑距离。 |
| `Random` | 从符合条件的对手中等概率随机选择。 |
| `LastAggressor` | 选择最近一名攻击行为符合反击条件的进攻者。 |

`AttackTargetCommitment` 决定保持同一目标多久：

- `Default`：新策略使用 `PerAttack`；搭配 `Inherit` 时沿用原有行为。
- `PerAttack`：进攻开始时选择目标，整个进攻期间不变。
- `UntilDefeated`：只要对手仍是有效目标，多次进攻都针对该对手。

每次进攻随机选择对手，可能再次选中同一对手：

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### 招募

`RecruitPolicy` 默认为 `Native`。选择 `WeightedRoles`，可在防御、袭扰、主攻军队和出城迎敌之间分配招募。兵种列表、招募间隔和配额继续生效。

**每个**实力档位（`Default`、`Weak`、`Strong`）的四个整数权重之和必须为 **100**。`RecruitProbSortie…` 默认为 0。权重控制招募选择，不保证最终军队比例。

示例：AI 较弱时加强防御；防御兵力未补齐时优先招募防御部队：

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

`RecruitConditions` 可省略（默认 `[]`，最多八条规则）。第一条匹配的规则替代当前实力档位的权重。每条规则必须包含全部四个权重，总和为 100。

在 `When` 中，可用 `Strength` 指定实力档位，也可使用以下条件：

| 条件 | 检查是否…… |
| --- | --- |
| `HomeUnderThreat` | 基地受到威胁。 |
| `AttackActive` | 正在进攻。 |
| `DefenseIncomplete` | 防御兵力尚未补齐。 |
| `EquipmentSurplus` | 有多余装备。 |

`true` 要求条件成立，`false` 要求条件不成立。所有已填写条件都必须匹配。省略的条件不参与判断；空的 `When` 始终匹配。

### 修改设置

相关字段应一起修改。部分更新会保留未填写的值；用 `Default` 取消先前的 `UntilDefeated` 设置。恢复两个系统的原有行为：

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

先前的出击权重和规则在 `Native` 下仍会保存，但不生效；此次更新不要填写这些字段。在 `WeightedRoles` 下将出击权重设为 0，并不能恢复 `Native`。
