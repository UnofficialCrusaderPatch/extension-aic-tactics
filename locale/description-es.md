AIC Tactics permite ajustar el reclutamiento, los ataques, las incursiones y las máquinas de asedio de cada IA instalada. Pon estos campos en su AIC. Las políticas nuevas de reclutamiento, ataque e incursión son opcionales; los campos de asedio omitidos usan los ajustes del módulo. Siguen vigentes las listas y límites de tropas.

### Reclutamiento

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: proporción de salidas por nivel de fuerza. Definir una activa las cuatro proporciones para esa IA. Defensa, incursión, ataque y salida deben sumar 100 en cada nivel.
- `RecruitConditions`: proporciones distintas según la situación. Ejemplo: si el castillo está amenazado, 70 % defensa, 20 % ataque y 10 % salidas. Se aplica la primera regla que coincida.
- `RecruitPolicy`: opción de compatibilidad. `Native` conserva la selección original; `WeightedRoles` activa expresamente las cuatro proporciones.
- `DefRecruitComposition`: `PreserveSlots` respeta las proporciones de `DefUnit1..8`; `Native` conserva la elección habitual.
- `RecruitInitialDefenseMonths`: tiempo durante el que incursiones y ejército principal esperan a completar la defensa. 0 elimina la espera; valor predeterminado: 6 meses.

Una proporción de salidas activa `WeightedRoles` automáticamente. Las reglas de situación, la composición defensiva y `RecruitInitialDefenseMonths` requieren activarlo expresamente. Los intervalos y cupos siguen vigentes.

### Ataques y represalias

- `AttackTargetPolicy`: `Inherit` usa `TargetChoice`. `LowestPopulation`, `FewestTroops`, `LowestCombatPower`, `Random` y `LastAggressor` eligen un objetivo por ataque. Para conservarlo entre ataques: `{ "Choice": "LastAggressor", "UntilDefeated": true }`. Se puede añadir `"Provocation": { "LossPower": 200 }` para ajustar la represalia.
- `AttackTargetCommitment`: sigue disponible para AIC antiguos. En los nuevos, elige objetivo y duración juntos con `AttackTargetPolicy`.
- `AttackPreparation`: `DuringAttack` prepara la siguiente oleada en casa mientras el ejército ataca; `Native` mantiene los tiempos habituales.
- `AttackActivation`: `AfterProvocation` espera un ataque enemigo suficiente antes de lanzar ejércitos o incursiones; `Immediate` no espera. Las salidas defensivas siguen disponibles.
- `ProvocationRules`: sigue disponible para AIC antiguos, `AfterProvocation` y la amenaza usada al reclutar. Para `LastAggressor`, un AIC nuevo puede poner los valores en `AttackTargetPolicy.Provocation`.

### Incursiones

- `RaidTargetPolicy`: `Native` conserva las incursiones; `NearestReachable` elige edificios cercanos accesibles; `Opportunistic` considera preferencias y peligro; `RandomNearby` elige uno accesible al azar y después ataca los edificios cercanos.
- `RaidGroupCount` (1–4) divide la fuerza existente; `RaidMinGroupSize` (1–256) hace esperar o reunirse a los grupos pequeños.
- `RaidFocus` acepta `Any`, `Food`, `Industry`, `HighValue` o porcentajes como `{ "Food": 60, "Industry": 20 }`; el resto permite cualquier edificio. Funciona con `Opportunistic` y `RandomNearby`. `RaidRiskTolerance` fija el riesgo; `RaidEnemyScope` elige al rival principal o a cualquier enemigo.

Los cinco campos posteriores a `RaidTargetPolicy` requieren una política nueva. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` y `RaidRetargetDelay` siguen vigentes.

### Asedio e ingenieros

- `SafeSiegePlacement` evita lugares de construcción ocupados (módulo: ACTIVADO).
- `ActualSiegeResourcePayment` exige materiales y oro; la IA compra lo que falte mediante su comercio normal (DESACTIVADO).
- `CoordinatedSiegeHarassment` envía un grupo de máquinas a posiciones de tiro accesibles (DESACTIVADO). `SiegeHarassMinEngines` fija el mínimo deseado (0–20, valor 3); `HarassingSiegeEnginesMax` sigue limitando el total.
- `LargerSiegeForces` repite la composición mientras queden ingenieros de ataque vivos y libres y sitios válidos (DESACTIVADO). `SiegeForceMax` es un límite opcional (0–64); con 0 decide la cantidad de ingenieros. `AttMaxEngineers` limita las dotaciones.

El emplazamiento seguro se activa con las opciones avanzadas de asedio. El recuento de funciones de ingeniero viene activado; Fixed Engineers gestiona la vida de las dotaciones.

Los valores de asedio del AIC prevalecen sobre los del módulo.
