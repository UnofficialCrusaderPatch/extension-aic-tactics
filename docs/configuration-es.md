Define cómo recluta tropas tu IA y cómo elige sus objetivos de ataque.

Añade los campos que necesites a tu configuración AIC. Sin ajustes nuevos, las personalidades existentes mantienen su comportamiento.

### Objetivos de ataque

| `AttackTargetPolicy` | Objetivo |
| --- | --- |
| `Inherit` (predeterminado) | Según el `TargetChoice` existente. |
| `LowestPopulation` | El oponente con menos civiles. |
| `FewestTroops` | El oponente con menos unidades militares. |
| `LowestCombatPower` | La menor fuerza militar estimada, sin tener en cuenta la distancia. |
| `Random` | Un oponente válido al azar, todos con la misma probabilidad. |
| `LastAggressor` | El último agresor cuyo ataque cumpla los criterios de represalia. |

`AttackTargetCommitment` determina cuánto tiempo mantener al mismo oponente:

- `Default`: `PerAttack` con políticas nuevas o `DuringAttack`; en los demás casos, el comportamiento existente.
- `PerAttack`: elegir al iniciar el ataque y mantener el objetivo hasta que termine.
- `UntilDefeated`: mantener al mismo oponente entre ataques mientras siga siendo un objetivo válido.

Oponente aleatorio por ataque; puede volver a salir el mismo:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Reclutamiento

`RecruitPolicy` usa `Native` por defecto. Elige `WeightedRoles` para distribuir el reclutamiento entre defensa, incursiones, ejército de ataque principal y salidas del castillo. Se mantienen las listas de tropas, los intervalos y los cupos.

En **cada** nivel de fuerza (`Default`, `Weak`, `Strong`), los cuatro pesos enteros deben sumar **100**. `RecruitProbSortie…` vale 0 por defecto. Los pesos orientan el reclutamiento, sin garantizar las proporciones finales del ejército.

Ejemplo: más defensa cuando la IA es débil; prioridad a la defensa mientras no alcance su dotación:

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

`RecruitConditions` es opcional (por defecto: `[]`, máximo ocho reglas). La primera regla coincidente sustituye los pesos del nivel actual. Cada regla necesita los cuatro pesos, sumando 100.

En `When`, usa `Strength` para indicar un nivel de fuerza, o estas condiciones:

| Condición | Comprueba si… |
| --- | --- |
| `HomeUnderThreat` | La base está amenazada. |
| `AttackActive` | Hay un ataque en curso. |
| `DefenseIncomplete` | La defensa aún no ha alcanzado su dotación. |
| `EquipmentSurplus` | Hay equipo sobrante. |

`true` exige la condición; `false`, su ausencia. Todas las condiciones indicadas deben cumplirse. Las omitidas se ignoran; un `When` vacío coincide siempre.

### Cambiar ajustes

Aplica juntos los campos relacionados. Las actualizaciones parciales conservan los valores omitidos; usa `Default` para quitar un `UntilDefeated` anterior. Para devolver ambos sistemas a su comportamiento existente:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Los pesos de salida y las reglas anteriores se conservan inactivos bajo `Native`; no los incluyas en esa actualización. Un peso de salida de 0 en `WeightedRoles` no restaura `Native`.

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
