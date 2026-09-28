# Siege placement in an individual AIC

`SafeSiegePlacement` is an optional boolean. `true` skips occupied attack-angle
sites, tries another point in the game's search, and prevents construction on
friendly units. A rejected harassment tent keeps the game's grid mark so the
next search can try another site, and waits before retrying. If final assault
placement fails, it restores the old reservation and
returns the selected assault engineers to their attack group. `false` retains
native placement. If absent, the
AI inherits the module's **Protect units at siege sites** switch
(`safeSiegePlacement`, default ON).
Advanced siege features enable safe placement automatically. An AIC cannot
set it to `false` while enabling one of them.

`ActualSiegeResourcePayment` is also an optional boolean. `true` checks the
configured material and gold costs after the game's site check and before
siege construction, including directly spawned defensive equipment. Missing
goods enter the game's existing
AI trade queue; construction retries at a later opportunity. The module keeps
the native debit and skips the defensive path's extra gold subtraction when
this policy is enabled. `false` retains native admission. Its module
fallback, `actualSiegeResourcePayment`, is OFF.

`CoordinatedSiegeHarassment` is an optional boolean. `true` gathers mobile
catapults and fire ballistas before sending them to separate reachable firing
positions using their current native target ranges. Its module fallback is OFF. `SiegeHarassMinEngines` is an optional
integer from 0 to 20; it defaults to the module fallback of 3. The AI waits for
that many ready engines, then permits a reachable pair after one game month (800
ticks). Zero or one permits a single ready engine. The existing
`HarassingSiegeEnginesMax` remains the total limit; it can already exceed eight
without enlarging the eight-entry AIC composition array. These two fields are
independent of payment.

`LargerSiegeForces` is an optional boolean, OFF by default. When true, the AI
repeats its authored main-assault `SiegeEngine1..8` mix while engineers, native
tribe slots and safe construction sites remain available. A partly free native
tribe pool can still produce a partial batch. `SiegeForceMax` is an
optional integer from 0 to 64, with module fallback 0. It limits equipment
already active or under construction in the current wave plus new construction;
0 lets the available engineers determine the force size. This policy is
separate from harassment and resource payment. The existing `AttMaxEngineers`
still limits available crews.

```json
{
  "SafeSiegePlacement": true,
  "ActualSiegeResourcePayment": true,
  "CoordinatedSiegeHarassment": true,
  "SiegeHarassMinEngines": 4,
  "LargerSiegeForces": true,
  "SiegeForceMax": 10,
  "HarassingSiegeEnginesMax": 10
}
```

These fields control only an installed and enabled AIC Tactics module. Existing
`HarassingSiegeEnginesMax` and the eight siege composition entries keep their
native meanings; these fields do not add AIC slots.
