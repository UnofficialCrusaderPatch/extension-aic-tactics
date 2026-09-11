AIC Tactics está en desarrollo para que los autores de personalidades de IA puedan configurar reglas opcionales de reclutamiento, ataques e incursiones.

Las funciones previstas incluyen probabilidades de reclutamiento y salidas defensivas según las circunstancias, la preparación del siguiente ejército durante un ataque, la selección de oponentes y las represalias, e incursiones divididas en un número limitado de grupos. Las personalidades existentes deben conservar su comportamiento salvo que su autor active expresamente una nueva regla.

Las nuevas reglas de selección de oponentes están diseñadas para mantener el mismo objetivo durante todo el ataque de forma predeterminada (`PerAttack`). Los autores pueden elegir `UntilDefeated` para conservar ese oponente entre ataques. Las personalidades sin modificar mantienen su comportamiento existente, incluida la estabilidad del objetivo proporcionada por Legacy.

**En desarrollo: todavía no hay una versión lista para jugar.**

## Guía de configuración (avance en desarrollo)

Estos campos pertenecen a la configuración AIC de una personalidad de IA; no son nuevos controles de la interfaz. Los ejemplos explican la configuración en desarrollo: no son paquetes completos ni ajustes listos para jugar. Mantén los nombres y valores de los parámetros tal como aparecen, independientemente del idioma de la interfaz.

### Conservar el comportamiento existente

En una personalidad sin modificar, omite los campos nuevos. Valores predeterminados: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit` y `AttackTargetCommitment: Default`. Reclutamiento y selección de objetivos se activan por separado.

### Elegir un oponente

Significado previsto de `AttackTargetPolicy`:

| Valor | Significado |
| --- | --- |
| `Inherit` | Usar el `TargetChoice` original de la personalidad (predeterminado). |
| `LowestPopulation` | Preferir al oponente con menos civiles. |
| `FewestTroops` | Preferir al oponente con menos unidades militares. |
| `LowestCombatPower` | Preferir la menor fuerza militar estimada, sin ponderar la distancia. |
| `Random` | Misma probabilidad para cada oponente válido; puede repetirse. |
| `LastAggressor` | Responder al último incidente hostil que cumpla los criterios, no a cada golpe. |

`AttackTargetCommitment: Default` significa `PerAttack` con las nuevas políticas: elegir al iniciar el ataque y mantener el objetivo durante todo el ataque. Con `Inherit`, conserva el comportamiento Native/Legacy existente. Elige `UntilDefeated` explícitamente para mantener un oponente válido entre ataques. Si el objetivo deja de ser válido, debe cerrarse el ataque actual antes de elegir otro; el ejército desplegado no cambia de oponente a mitad del ataque.

Ejemplo: oponente aleatorio por ataque, sin cambios durante el mismo:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Distribuir el reclutamiento

`RecruitPolicy: WeightedRoles` reparte las decisiones entre defensa, incursiones, ejército de ataque principal y salidas (tropas que abandonan el castillo para combatir enemigos cercanos). Se conservan los intervalos, listas de tropas y cupos existentes. Los pesos afectan a las opciones de reclutamiento válidas, sin garantizar la proporción final del ejército.

Configura los tres niveles de fuerza: `Default`, `Weak` y `Strong`. Los cuatro pesos enteros de cada nivel deben sumar 100. Se reutilizan `RecruitProbDef…`, `RecruitProbRaid…` y `RecruitProbAttack…`; `RecruitProbSortie…` añade las salidas y vale 0 por defecto. No hay reajuste automático. Ejemplo, no una recomendación de equilibrio:

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

### Reclutamiento condicional (avanzado)

`RecruitConditions` es una lista vacía por defecto y admite hasta ocho reglas ordenadas. Cada una necesita `When` y los cuatro pesos `Defense`, `Raid`, `Attack`, `Sortie`, sumando 100. La primera regla coincidente sustituye la fila de fuerza; si ninguna coincide, se usa esa fila.

En `When`, `Strength` selecciona `Default`, `Weak` o `Strong`. Las comprobaciones sí/no son `HomeUnderThreat` (base amenazada), `AttackActive` (ataque en curso), `DefenseIncomplete` (defensa incompleta), `EquipmentSurplus` (equipo sobrante). `true` exige la condición; `false`, su ausencia. Las comprobaciones omitidas se ignoran. Todas las indicadas deben coincidir; un `When` vacío coincide siempre.

### Cambiar o deshacer ajustes

Aplica juntos los campos relacionados. Las actualizaciones parciales conservan los valores omitidos, incluido un `UntilDefeated` anterior. Usa `Default` para recuperar la duración automática. Para desactivar estas políticas:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Cambiar únicamente a `RecruitPolicy: Native` conserva los pesos de salida y reglas anteriores como valores inactivos. No establezcas esos campos mientras uses `Native`. Un peso de salida de 0 en `WeightedRoles` no restaura el reclutamiento Native.

**Pendiente:** integración en el juego, definiciones nativas exactas de amenazas, excedentes y recuentos de oponentes, umbrales de represalias, preparación de la siguiente oleada, incursiones divididas y aplicación de opciones Legacy. Aún no hay una configuración Legacy validada para publicación. Estas descripciones no demuestran compatibilidad multijugador, de guardado ni de replay.
