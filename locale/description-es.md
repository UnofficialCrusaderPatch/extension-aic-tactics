Configura el reclutamiento, la preparación de oleadas, los objetivos y las incursiones mediante el AIC de tu IA. Sin opciones nuevas, se conserva su comportamiento.

### Reclutamiento

- `RecruitPolicy`: `Native` (predeterminado) conserva el reclutamiento existente. `WeightedRoles` lo distribuye entre defensa, incursiones, ejército principal y salidas.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: pesos de las salidas para una IA normal, débil o fuerte. Enteros de **0–100**, valor predeterminado **0**. Junto con los pesos existentes de defensa, incursión y ataque, cada nivel debe sumar **100**. Requiere `WeightedRoles`; se mantienen listas de tropas, intervalos y cupos.
- `RecruitConditions`: hasta **8** reglas ordenadas, ninguna por defecto. La primera coincidente sustituye los pesos del nivel. Cada regla contiene `When` y cuatro pesos que suman 100: `Defense`, `Raid`, `Attack`, `Sortie`. Requiere `WeightedRoles`.
- `DefRecruitComposition`: `Native` (por defecto) conserva el comportamiento actual. `PreserveSlots` reserva la parte de cada entrada de `DefUnit1..8`; repetir un tipo aumenta su parte. Si falta equipo, sus plazas quedan libres. Requiere `WeightedRoles`.
- `RecruitInitialDefenseMonths`: **0–30** meses, **6** por defecto. Durante este periodo, pospone el reclutamiento para incursiones y ejército principal mientras falten defensores. Las salidas siguen permitidas; un peso de 0 sigue siendo 0. **0** desactiva la espera. Requiere `WeightedRoles`.

`When` puede comprobar `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (base amenazada), `AttackActive` (ataque en curso), `DefenseIncomplete` (defensa incompleta) y `EquipmentSurplus` (equipo sobrante). Todas las condiciones indicadas deben cumplirse; `true` exige la condición, `false` su ausencia. Un `When` vacío coincide siempre.

### Objetivos de ataque

`AttackTargetPolicy`:

- `Inherit` (predeterminado): Según el `TargetChoice` existente.
- `LowestPopulation`: Menos civiles.
- `FewestTroops`: Menos unidades militares.
- `LowestCombatPower`: Menor fuerza militar estimada, sin tener en cuenta la distancia.
- `Random`: Un oponente válido al azar, con la misma probabilidad para todos.
- `LastAggressor`: El último agresor que cumpla los criterios de represalia.

`AttackTargetCommitment`:

- `Default`: `PerAttack` con políticas nuevas o `DuringAttack`; en los demás casos, el comportamiento existente.
- `PerAttack`: mantener el objetivo durante todo el ataque.
- `UntilDefeated`: mantenerlo entre ataques mientras siga siendo un oponente válido.

### Preparación y represalias

- `AttackPreparation`: `Native` (predeterminado), o `DuringAttack` para preparar la siguiente oleada en casa durante un ataque. Se mantienen los límites de tropas y el crecimiento de las oleadas.
- `AttackActivation`: `Immediate` (predeterminado), o `AfterProvocation` para esperar un ataque suficiente antes de lanzar ejércitos o incursiones. Las salidas defensivas siguen disponibles.
- `ProvocationRules`: umbrales de represalia; valores predeterminados: `ThreatPower` **100**, `CombatTicks` **200**, `LossPower` **100**, `WindowTicks` **800**. Los combates prolongados cerca del torreón o suficientes bajas militares activan la respuesta; el daño al señor cuenta inmediatamente. Indica los cuatro valores. **800 ticks = un mes de juego**.

### Incursiones

- `RaidTargetPolicy`: `Native` (predeterminado), `NearestReachable` para edificios cercanos accesibles, u `Opportunistic` según distancia, prioridad y peligro.
- `RaidGroupCount`: **1–4**, predeterminado **1**. Divide las tropas de incursión existentes sin reclutar más.
- `RaidMinGroupSize`: **1–256**, predeterminado **4**. Los grupos menores esperan o se reúnen.
- `RaidFocus`: `Any` (predeterminado), `Food` (alimentos), `Industry` (producción) o `HighValue` (coste de reconstrucción). Requiere `Opportunistic`.
- `RaidRiskTolerance`: `Low`, `Medium` (predeterminado) o `High`. Tolerancia a tropas y defensas enemigas cercanas.
- `RaidEnemyScope`: `PrimeTarget` (predeterminado) o `AnyEnemy`. No cambia el objetivo del ejército principal.

Estas cinco opciones requieren una nueva `RaidTargetPolicy`. Se mantienen `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` y `RaidRetargetDelay`. Para restaurar los valores predeterminados: `AttackPreparation: Native`, `AttackActivation: Immediate`, `RaidTargetPolicy: Native`.
