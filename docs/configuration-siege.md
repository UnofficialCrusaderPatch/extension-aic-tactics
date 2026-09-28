# Siege placement in an individual AIC

`SafeSiegePlacement` is an optional boolean. `true` skips occupied attack-angle
sites, tries another point in the game's search, and prevents construction on
friendly units. If final placement fails, it restores the old reservation and
skips the construction order. `false` retains native placement. If absent, the
AI inherits the module's **Protect units at siege sites** switch
(`safeSiegePlacement`, default ON).
The AIC value always takes precedence, including an explicit `false`.

`ActualSiegeResourcePayment` is also an optional boolean. `true` checks the
configured material and gold costs after the game's site check and before
siege construction, including directly spawned defensive equipment. Missing
goods enter the game's existing
AI trade queue; construction retries at a later opportunity. The module keeps
the native debit and skips the defensive path's extra gold subtraction when
this policy is enabled. `false` retains native admission. Its module
fallback, `actualSiegeResourcePayment`, is OFF. Both AIC fields are independent.

`CoordinatedSiegeHarassment` is an optional boolean. `true` gathers mobile
catapults and fire ballistas before sending them to separate reachable firing
positions. Its module fallback is OFF. `SiegeHarassMinEngines` is an optional
integer from 0 to 20; it defaults to the module fallback of 3. The AI waits for
that many ready engines, then sends those available after one game month (800
ticks). Zero sends the first ready engine. The existing
`HarassingSiegeEnginesMax` remains the total limit; it can already exceed eight
without enlarging the eight-entry AIC composition array. These two fields are
independent of placement and payment.

```json
{
  "SafeSiegePlacement": false,
  "ActualSiegeResourcePayment": true,
  "CoordinatedSiegeHarassment": true,
  "SiegeHarassMinEngines": 4,
  "HarassingSiegeEnginesMax": 10
}
```

These fields control only an installed and enabled AIC Tactics module. Existing
`HarassingSiegeEnginesMax` and the eight siege composition entries keep their
native meanings; these fields do not add AIC slots.
