# Siege placement in an individual AIC

`SafeSiegePlacement` is an optional boolean. `true` skips occupied attack-angle
sites, tries another point in the game's search, and prevents construction on
friendly units. If final placement fails, it restores the old reservation and
skips the construction order. `false` retains native placement. If absent, the
AI inherits the module's **Protect units at siege sites** switch
(`safeSiegePlacement`, default ON).
The AIC value always takes precedence, including an explicit `false`.

`ActualSiegeResourcePayment` is also an optional boolean. `true` checks the
configured material and gold costs before native siege construction, including
directly spawned defensive equipment. Missing goods enter the game's existing
AI trade queue; construction retries at a later opportunity. The module keeps
the native debit and skips the defensive path's extra gold subtraction when
this policy is enabled. `false` retains native admission. Its module
fallback, `actualSiegeResourcePayment`, is OFF. Both AIC fields are independent.

```json
{
  "SafeSiegePlacement": false,
  "ActualSiegeResourcePayment": true
}
```

These fields control only an installed and enabled AIC Tactics module. Existing
`HarassingSiegeEnginesMax` and the eight siege composition entries keep their
native meanings; this field does not change their limits or add AIC slots.
