AIC Tactics permite ajustar el reclutamiento, los ataques, las incursiones y las máquinas de asedio de cada IA instalada. Pon estos campos en su AIC. Las políticas nuevas de reclutamiento, ataque e incursión son opcionales; los campos de asedio e ingenieros omitidos usan los ajustes del módulo. Siguen vigentes las listas y límites de tropas.

### Reclutamiento

- `RecruitPolicy`: `Native` conserva el reclutamiento habitual. `WeightedRoles` reparte las nuevas tropas entre defensa, incursiones, ejército principal y salidas.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: parte destinada a salidas en los tres niveles de fuerza nativos. Defensa, incursión, ataque y salida deben sumar 100 en cada nivel.
- `RecruitConditions`: hasta 8 reglas ordenadas que sustituyen esas partes para un reclutamiento; gana la primera que coincida. `When` puede comprobar `Strength`, `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` y `EquipmentSurplus`.
- `DefRecruitComposition`: `PreserveSlots` respeta las proporciones de `DefUnit1..8`; `Native` conserva la elección habitual.
- `RecruitInitialDefenseMonths`: tiempo durante el que incursiones y ejército principal esperan a completar la defensa. 0 elimina la espera; valor predeterminado: 6 meses.

Las partes de salida, reglas de situación, proporciones de defensa y espera inicial requieren `WeightedRoles`. Los intervalos y cupos de reclutamiento siguen vigentes.

### Ataques y represalias

- `AttackTargetPolicy`: `Inherit` usa `TargetChoice`. Las otras opciones eligen menos civiles (`LowestPopulation`), menos soldados (`FewestTroops`), menor fuerza militar (`LowestCombatPower`), un rival al azar (`Random`) o el último agresor válido (`LastAggressor`).
- `AttackTargetCommitment`: `PerAttack` mantiene el objetivo durante un ataque; `UntilDefeated` lo conserva entre ataques. `Default` usa `PerAttack` con una política nueva o preparación de la siguiente oleada, y el comportamiento habitual en los demás casos.
- `AttackPreparation`: `DuringAttack` prepara la siguiente oleada en casa mientras el ejército ataca; `Native` mantiene los tiempos habituales.
- `AttackActivation`: `AfterProvocation` espera un ataque enemigo suficiente antes de lanzar ejércitos o incursiones; `Immediate` no espera. Las salidas defensivas siguen disponibles.
- `ProvocationRules`: `ThreatPower` y `CombatTicks` fijan la amenaza cerca del torreón; `LossPower` y `WindowTicks` fijan las pérdidas y el plazo. El daño al señor basta de inmediato. Indica los cuatro valores juntos.

### Incursiones

- `RaidTargetPolicy`: `Native` conserva las incursiones; `NearestReachable` elige edificios cercanos accesibles; `Opportunistic` también considera la prioridad y el peligro.
- `RaidGroupCount` (1–4) divide la fuerza existente; `RaidMinGroupSize` (1–256) hace esperar o reunirse a los grupos pequeños.
- `RaidFocus` elige `Any`, `Food`, `Industry` o `HighValue` y requiere `Opportunistic`. `RaidRiskTolerance` fija el riesgo aceptado (`Low`, `Medium`, `High`). `RaidEnemyScope` limita las incursiones al objetivo principal o permite cualquier enemigo.

Los cinco campos posteriores a `RaidTargetPolicy` requieren una política nueva. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` y `RaidRetargetDelay` siguen vigentes.

### Asedio e ingenieros

- `SafeSiegePlacement` evita lugares de construcción ocupados (módulo: ACTIVADO).
- `ActualSiegeResourcePayment` exige materiales y oro; la IA compra lo que falte mediante su comercio normal (DESACTIVADO).
- `CoordinatedSiegeHarassment` envía un grupo de máquinas a posiciones de tiro accesibles (DESACTIVADO). `SiegeHarassMinEngines` fija el mínimo deseado (0–20, valor 3); `HarassingSiegeEnginesMax` sigue limitando el total.
- `LargerSiegeForces` repite la composición de máquinas para el ataque principal (DESACTIVADO). `SiegeForceMax` limita las máquinas activas y previstas (0–20, valor 10); 0 deja una sola tanda nativa. `AttMaxEngineers` sigue limitando las dotaciones.
- `CorrectEngineerRoleCounting` cuenta a los ingenieros asignados en sus cupos de tropas; las tareas de asedio y aceite se cuentan aparte (ACTIVADO).

Para asedio e ingenieros, un valor AIC explícito, incluido `false` o 0, prevalece sobre el ajuste del módulo. Si falta, se usa dicho ajuste.
