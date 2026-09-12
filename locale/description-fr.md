Ajoute à votre AIC des réglages de recrutement et de ciblage. Sans nouveaux réglages, le comportement de l'IA reste inchangé.

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

- `Default` : `PerAttack` pour les nouvelles politiques ; comportement existant avec `Inherit`.
- `PerAttack` : conserver la cible pendant toute l'attaque.
- `UntilDefeated` : conserver la cible entre les attaques tant qu'elle reste un adversaire valide.

Une modification partielle conserve les valeurs omises. Retour au comportement existant : `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. Les anciens poids de sortie et règles restent enregistrés mais inactifs ; omettez-les lors du passage à `Native`.
