Réglez le recrutement, la préparation des vagues, les cibles et les raids dans l’AIC de votre IA. Sans nouvelles options, son comportement reste inchangé.

### Recrutement

- `RecruitPolicy` : `Native` (par défaut) conserve le recrutement existant. `WeightedRoles` le répartit entre défense, raids, armée principale et sorties.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong` : poids des sorties pour une IA normale, faible ou forte. Entiers de **0 à 100**, défaut **0**. Avec les poids existants de défense, raid et attaque, chaque niveau doit totaliser **100**. Exige `WeightedRoles` ; listes de troupes, intervalles et quotas restent applicables.
- `RecruitConditions` : jusqu'à **8** règles ordonnées, aucune par défaut. La première règle correspondante remplace les poids du niveau. Chaque règle contient `When` et quatre poids totalisant 100 : `Defense`, `Raid`, `Attack`, `Sortie`. Exige `WeightedRoles`.
- `DefRecruitComposition` : `Native` (par défaut) conserve le comportement existant. `PreserveSlots` réserve la part de chaque entrée de `DefUnit1..8` ; les doublons augmentent cette part. Les places restent libres si l’équipement manque. Exige `WeightedRoles`.
- `RecruitInitialDefenseMonths` : **0–30** mois, **6** par défaut. Durant cette période, reporte les recrues de raid et d’armée principale tant que la défense est incomplète. Les sorties restent possibles ; un poids nul reste nul. **0** désactive ce délai. Exige `WeightedRoles`.

`When` peut vérifier `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (base menacée), `AttackActive` (attaque en cours), `DefenseIncomplete` (défense incomplète) et `EquipmentSurplus` (équipement excédentaire). Toutes les conditions indiquées doivent correspondre ; `true` exige la condition, `false` son absence. Un `When` vide correspond toujours.

### Cibles d'attaque

`AttackTargetPolicy`:

- `Inherit` (par défaut): Selon le `TargetChoice` existant.
- `LowestPopulation`: Le moins de civils.
- `FewestTroops`: Le moins d'unités militaires.
- `LowestCombatPower`: La puissance militaire estimée la plus faible, sans tenir compte de la distance.
- `Random`: Un adversaire admissible au hasard, à chances égales.
- `LastAggressor`: Le dernier agresseur remplissant les critères de représailles.

`AttackTargetCommitment` :

- `Default` : `PerAttack` avec une nouvelle politique ou `DuringAttack` ; comportement existant sinon.
- `PerAttack` : conserver la cible pendant toute l'attaque.
- `UntilDefeated` : conserver la cible entre les attaques tant qu'elle reste un adversaire valide.

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
