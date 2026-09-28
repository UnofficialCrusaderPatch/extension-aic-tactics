AIC Tactics règle le recrutement, les attaques, les raids et les engins de siège de chaque IA installée. Ajoutez ces champs à son AIC. Les nouvelles politiques de recrutement, d’attaque et de raid sont facultatives ; les champs de siège et d’ingénieurs omis suivent les options du module. Les listes et limites de troupes restent valables.

### Recrutement

- `RecruitPolicy` : `Native` garde le recrutement habituel. `WeightedRoles` répartit les recrues entre défense, raids, armée principale et sorties.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong` : part des sorties pour les trois niveaux de force natifs. Défense, raid, attaque et sortie doivent totaliser 100 à chaque niveau.
- `RecruitConditions` : jusqu’à 8 règles ordonnées qui remplacent ces parts pour un recrutement ; la première règle applicable l’emporte. `When` peut vérifier `Strength`, `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` et `EquipmentSurplus`.
- `DefRecruitComposition` : `PreserveSlots` respecte les proportions de `DefUnit1..8` ; `Native` garde la sélection habituelle.
- `RecruitInitialDefenseMonths` : durée pendant laquelle raids et armée principale attendent que la défense atteigne son quota. 0 supprime l’attente ; valeur par défaut : 6 mois.

Les parts de sortie, règles de situation, proportions de défense et délai initial exigent `WeightedRoles`. Intervalles et quotas de recrutement restent applicables.

### Attaques et riposte

- `AttackTargetPolicy` : `Inherit` utilise `TargetChoice`. Les autres valeurs visent le moins de civils (`LowestPopulation`), le moins de soldats (`FewestTroops`), la plus faible puissance militaire (`LowestCombatPower`), un adversaire au hasard (`Random`) ou le dernier agresseur admissible (`LastAggressor`).
- `AttackTargetCommitment` : `PerAttack` garde la cible pendant une attaque ; `UntilDefeated` la garde entre les attaques. `Default` choisit `PerAttack` avec une nouvelle politique de cible ou la préparation de la prochaine vague, sinon le comportement habituel.
- `AttackPreparation` : `DuringAttack` prépare la prochaine vague au château pendant l’attaque ; `Native` garde le calendrier habituel.
- `AttackActivation` : `AfterProvocation` attend une attaque ennemie suffisante avant de lancer armées et raids ; `Immediate` n’attend pas. Les sorties défensives restent possibles.
- `ProvocationRules` : `ThreatPower` et `CombatTicks` règlent la menace près du donjon ; `LossPower` et `WindowTicks` règlent les pertes et leur période. Des dégâts au seigneur suffisent immédiatement. Indiquez les quatre valeurs ensemble.

### Raids

- `RaidTargetPolicy` : `Native` garde les raids habituels ; `NearestReachable` vise des bâtiments proches accessibles ; `Opportunistic` tient aussi compte de la priorité et du danger.
- `RaidGroupCount` (1–4) partage la force de raid existante ; `RaidMinGroupSize` (1–256) fait attendre ou réunir les petits groupes.
- `RaidFocus` choisit `Any`, `Food`, `Industry` ou `HighValue` et exige `Opportunistic`. `RaidRiskTolerance` règle le risque accepté (`Low`, `Medium`, `High`). `RaidEnemyScope` limite les raids à la cible principale ou autorise tout ennemi.

Les cinq champs après `RaidTargetPolicy` exigent une nouvelle politique de raid. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` et `RaidRetargetDelay` restent valables.

### Siège et ingénieurs

- `SafeSiegePlacement` évite les emplacements occupés (module : ACTIVÉ).
- `ActualSiegeResourcePayment` exige matériaux et or ; l’IA achète les manques par son commerce normal (DÉSACTIVÉ).
- `CoordinatedSiegeHarassment` envoie des engins rassemblés vers des positions de tir accessibles (DÉSACTIVÉ). `SiegeHarassMinEngines` fixe le minimum souhaité (0–20, défaut 3) ; `HarassingSiegeEnginesMax` reste la limite totale.
- `LargerSiegeForces` répète la composition des engins lors de l’assaut principal (DÉSACTIVÉ). `SiegeForceMax` limite les engins actifs et prévus (0–20, défaut 10) ; 0 conserve une seule série native. `AttMaxEngineers` limite toujours les équipages.
- `CorrectEngineerRoleCounting` compte les ingénieurs affectés dans leurs quotas de troupes ; siège et huile restent séparés (ACTIVÉ).

Pour le siège et les ingénieurs, une valeur AIC explicite, y compris `false` ou 0, prévaut sur le réglage du module. Si elle manque, ce réglage s’applique.
