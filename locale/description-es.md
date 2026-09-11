Añade ajustes de reclutamiento y objetivos de ataque a tu AIC. Sin ajustes nuevos, la IA mantiene su comportamiento.

### Reclutamiento

- `RecruitPolicy`: `Native` (predeterminado) conserva el reclutamiento existente. `WeightedRoles` lo distribuye entre defensa, incursiones, ejército principal y salidas.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: pesos de las salidas para una IA normal, débil o fuerte. Enteros de **0–100**, valor predeterminado **0**. Junto con los pesos existentes de defensa, incursión y ataque, cada nivel debe sumar **100**. Requiere `WeightedRoles`; se mantienen listas de tropas, intervalos y cupos.
- `RecruitConditions`: hasta **8** reglas ordenadas, ninguna por defecto. La primera coincidente sustituye los pesos del nivel. Cada regla contiene `When` y cuatro pesos que suman 100: `Defense`, `Raid`, `Attack`, `Sortie`. Requiere `WeightedRoles`.

`When` puede comprobar `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (base amenazada), `AttackActive` (ataque en curso), `DefenseIncomplete` (defensa incompleta) y `EquipmentSurplus` (equipo sobrante). Todas las condiciones indicadas deben cumplirse; `true` exige la condición, `false` su ausencia. Un `When` vacío coincide siempre.

### Objetivos de ataque

| `AttackTargetPolicy` | Objetivo |
| --- | --- |
| `Inherit` (predeterminado) | Según el `TargetChoice` existente. |
| `LowestPopulation` | Menos civiles. |
| `FewestTroops` | Menos unidades militares. |
| `LowestCombatPower` | Menor fuerza militar estimada, sin tener en cuenta la distancia. |
| `Random` | Un oponente válido al azar, con la misma probabilidad para todos. |
| `LastAggressor` | El último agresor que cumpla los criterios de represalia. |

`AttackTargetCommitment`:

- `Default`: `PerAttack` para las políticas nuevas; comportamiento existente con `Inherit`.
- `PerAttack`: mantener el objetivo durante todo el ataque.
- `UntilDefeated`: mantenerlo entre ataques mientras siga siendo un oponente válido.

Las actualizaciones parciales conservan los valores omitidos. Para recuperar el comportamiento existente: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. Los pesos de salida y las reglas anteriores se conservan inactivos; omítelos al cambiar a `Native`.
