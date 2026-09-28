# Siege placement in an individual AIC

`SafeSiegePlacement` is an optional boolean. `true` skips occupied attack-angle
sites, tries another point in the game's search, and prevents construction on
friendly units. If final placement fails, it restores the old reservation and
skips the construction order. `false` retains native placement. If absent, the
AI inherits the module's **Protect units at siege sites** switch
(`safeSiegePlacement`, default ON).
The AIC value always takes precedence, including an explicit `false`.

```json
{
  "SafeSiegePlacement": false
}
```

This field controls only an installed and enabled AIC Tactics module. Existing
`HarassingSiegeEnginesMax` and the eight siege composition entries keep their
native meanings; this field does not change their limits or add AIC slots.
