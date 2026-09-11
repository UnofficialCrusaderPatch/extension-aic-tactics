AIC Tactics est en cours de développement pour permettre aux auteurs de personnalités d’IA de définir des règles facultatives de recrutement, d’attaque et de raid.

Les fonctionnalités prévues comprennent des probabilités conditionnelles de recrutement et de sortie, la préparation de l’armée suivante pendant une attaque, le choix des adversaires et les représailles, ainsi que des raids répartis entre un nombre limité de groupes. Les personnalités existantes doivent conserver leur comportement tant que leur auteur n’active pas explicitement une nouvelle règle.

Les nouvelles règles de choix d’adversaire sont conçues pour conserver par défaut la même cible pendant toute une attaque (`PerAttack`). Les auteurs peuvent choisir `UntilDefeated` pour garder cet adversaire d’une attaque à l’autre. Les personnalités non modifiées conservent leur comportement existant, y compris la stabilité de la cible assurée par Legacy.

**En cours de développement : aucune version jouable n’est encore disponible.**

## Guide de configuration (aperçu en développement)

Ces champs appartiennent à la configuration AIC d'une personnalité d'IA ; ce ne sont pas de nouveaux contrôles de l'interface. Les exemples décrivent une configuration en développement, pas des packs d'IA complets ni des réglages prêts à jouer. Conservez les noms et valeurs des paramètres tels quels, quelle que soit la langue de l'interface.

### Conserver le comportement existant

Pour une personnalité inchangée, omettez les nouveaux champs. Valeurs par défaut : `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit` et `AttackTargetCommitment: Default`. Recrutement et choix de cible s'activent séparément.

### Choisir un adversaire

Sens prévu de `AttackTargetPolicy` :

| Valeur | Signification |
| --- | --- |
| `Inherit` | Utiliser le `TargetChoice` d'origine de la personnalité (par défaut). |
| `LowestPopulation` | Préférer l'adversaire ayant le moins de civils. |
| `FewestTroops` | Préférer celui ayant le moins d'unités militaires. |
| `LowestCombatPower` | Préférer la puissance militaire estimée la plus faible, sans pondération par la distance. |
| `Random` | Même probabilité pour chaque adversaire admissible ; les répétitions sont possibles. |
| `LastAggressor` | Réagir au dernier incident hostile répondant aux critères, pas à chaque coup reçu. |

`AttackTargetCommitment: Default` équivaut à `PerAttack` pour les nouvelles politiques : choisir au lancement et garder la cible pendant toute l'attaque. Avec `Inherit`, le comportement Native/Legacy existant reste inchangé. Choisissez explicitement `UntilDefeated` pour conserver un adversaire valide entre les attaques. Si la cible devient invalide, l'attaque en cours doit être clôturée avant d'en choisir une autre ; l'armée déployée ne change pas d'adversaire en pleine attaque.

Exemple : adversaire aléatoire à chaque attaque, sans changement pendant celle-ci :

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Répartir le recrutement

`RecruitPolicy: WeightedRoles` répartit les choix entre défense, raids, armée d'attaque principale et sorties (troupes quittant le château pour combattre à proximité). Les intervalles, listes de troupes et quotas existants restent applicables. Les poids influencent les choix de recrutement admissibles, sans garantir la composition finale de l'armée.

Réglez les trois niveaux de force : `Default`, `Weak`, `Strong`. Les quatre poids entiers de chaque niveau doivent totaliser 100. Les champs `RecruitProbDef…`, `RecruitProbRaid…`, `RecruitProbAttack…` sont réutilisés ; `RecruitProbSortie…` ajoute les sorties, avec 0 par défaut. Aucun ajustement automatique. Exemple, sans recommandation d'équilibrage :

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

### Recrutement conditionnel (avancé)

`RecruitConditions` est une liste vide par défaut, contenant au plus huit règles ordonnées. Chaque règle exige `When` et les quatre poids `Defense`, `Raid`, `Attack`, `Sortie`, totalisant 100. La première règle correspondante remplace la ligne de force ; sinon, cette ligne s'applique.

Dans `When`, `Strength` vaut `Default`, `Weak` ou `Strong`. Les tests oui/non sont `HomeUnderThreat` (base menacée), `AttackActive` (attaque en cours), `DefenseIncomplete` (défense incomplète), `EquipmentSurplus` (équipement excédentaire). `true` exige la condition, `false` son absence ; un test omis est ignoré. Tous les tests fournis doivent correspondre ; un `When` vide correspond toujours.

### Modifier ou annuler

Appliquez ensemble les champs liés. Une modification partielle conserve les valeurs omises, y compris un ancien `UntilDefeated`. Utilisez `Default` pour rétablir la durée automatique. Pour désactiver ces politiques :

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Passer uniquement à `RecruitPolicy: Native` conserve les anciens poids de sortie et règles, mais les désactive. Ne définissez pas ces champs sous `Native`. Un poids de sortie de 0 sous `WeightedRoles` ne rétablit pas le recrutement Native.

**À terminer :** intégration au jeu, définitions natives précises des menaces, surplus et décomptes adverses, seuils de représailles, préparation de la vague suivante, raids divisés et application des options Legacy. Il n'existe pas encore de procédure validée pour les options Legacy. Ces descriptions ne prouvent pas la compatibilité multijoueur, sauvegarde ou replay.
