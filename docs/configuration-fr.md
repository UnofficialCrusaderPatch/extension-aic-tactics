Définissez comment votre IA recrute ses troupes et choisit ses cibles.

Ajoutez les champs souhaités à votre configuration AIC. Sans nouveaux réglages, les personnalités existantes conservent leur comportement.

### Cibles d'attaque

| `AttackTargetPolicy` | Cible choisie |
| --- | --- |
| `Inherit` (par défaut) | Selon le `TargetChoice` existant. |
| `LowestPopulation` | L'adversaire ayant le moins de civils. |
| `FewestTroops` | L'adversaire ayant le moins d'unités militaires. |
| `LowestCombatPower` | La puissance militaire estimée la plus faible, sans tenir compte de la distance. |
| `Random` | Un adversaire admissible au hasard, avec des chances égales. |
| `LastAggressor` | Le dernier agresseur dont l'attaque remplit les critères de représailles. |

`AttackTargetCommitment` détermine combien de temps conserver cet adversaire :

- `Default` : `PerAttack` avec une nouvelle politique ou `DuringAttack` ; comportement existant sinon.
- `PerAttack` : choisir au lancement et garder la cible durant toute l'attaque.
- `UntilDefeated` : garder le même adversaire entre les attaques tant qu'il reste une cible valide.

Un adversaire aléatoire par attaque ; le même peut être tiré à nouveau :

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Recrutement

`RecruitPolicy` vaut `Native` par défaut. Choisissez `WeightedRoles` pour répartir le recrutement entre défense, raids, armée d'attaque principale et sorties du château. Les listes de troupes, intervalles et quotas restent applicables.

Pour **chaque** niveau de force (`Default`, `Weak`, `Strong`), les quatre poids entiers doivent totaliser **100**. `RecruitProbSortie…` vaut 0 par défaut. Ces poids orientent le recrutement, sans garantir les proportions finales de l'armée.

Exemple : davantage de défense quand l'IA est faible ; priorité à la défense tant que son effectif est incomplet :

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

`RecruitConditions` est facultatif (par défaut : `[]`, huit règles maximum). La première règle correspondante remplace les poids du niveau actuel. Chaque règle exige les quatre poids, totalisant 100.

Dans `When`, utilisez `Strength` pour préciser un niveau de force, ou ces conditions :

| Condition | Vérifie si… |
| --- | --- |
| `HomeUnderThreat` | La base est menacée. |
| `AttackActive` | Une attaque est en cours. |
| `DefenseIncomplete` | L'effectif défensif est incomplet. |
| `EquipmentSurplus` | De l'équipement excédentaire est disponible. |

`true` exige la condition ; `false`, son absence. Toutes les conditions indiquées doivent correspondre. Les conditions omises sont ignorées ; un `When` vide correspond toujours.

### Modifier les réglages

Appliquez ensemble les champs liés. Une modification partielle conserve les valeurs omises ; utilisez `Default` pour annuler un ancien `UntilDefeated`. Pour rétablir le comportement existant des deux systèmes :

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Les anciens poids de sortie et règles restent enregistrés mais inactifs sous `Native` ; ne les incluez pas dans cette modification. Un poids de sortie de 0 sous `WeightedRoles` ne rétablit pas `Native`.

### Préparation et riposte

- `AttackPreparation` : `Native` (défaut), ou `DuringAttack` pour préparer la prochaine vague au château pendant l’attaque. Les limites de troupes et la progression des vagues restent applicables.
- `AttackActivation` : `Immediate` (défaut), ou `AfterProvocation` pour attendre une attaque suffisante avant de lancer armées et raids. Les sorties défensives restent possibles.
- `ProvocationRules` : seuils de riposte, par défaut `ThreatPower` **100**, `CombatTicks` **200**, `LossPower` **100**, `WindowTicks` **800**. Des combats prolongés près du donjon ou des pertes militaires suffisantes déclenchent la riposte ; des dégâts au seigneur suffisent immédiatement. Indiquer les quatre valeurs. **800 ticks = un mois de jeu**.

### Raids

- `RaidTargetPolicy` : `Native` (défaut), `NearestReachable` pour les bâtiments proches accessibles, ou `Opportunistic` selon la distance, la priorité et le danger.
- `RaidGroupCount` : **1–4**, défaut **1**. Répartit les troupes de raid existantes sans en recruter davantage.
- `RaidMinGroupSize` : **1–256**, défaut **4**. Les groupes plus petits attendent ou se regroupent.
- `RaidFocus` : `Any` (défaut), `Food` (nourriture), `Industry` (production) ou `HighValue` (coût de reconstruction). Nécessite `Opportunistic`.
- `RaidRiskTolerance` : `Low`, `Medium` (défaut) ou `High`. Tolérance aux troupes et défenses ennemies proches.
- `RaidEnemyScope` : `PrimeTarget` (défaut) ou `AnyEnemy`. Ne change pas la cible de l’armée principale.

Ces cinq options nécessitent une nouvelle `RaidTargetPolicy`. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` et `RaidRetargetDelay` restent applicables. Pour rétablir les valeurs par défaut : `AttackPreparation: Native`, `AttackActivation: Immediate`, `RaidTargetPolicy: Native`.
