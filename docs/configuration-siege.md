# Siege placement in an individual AIC

`SafeSiegePlacement` is an optional boolean. Set it to `true` to reject siege
construction sites occupied by that AI's own living units, or `false` to retain
the native placement check. If the field is absent, the AI inherits the module's
**Protect existing siege engines** switch (`safeSiegePlacement`, default ON).
The AIC value always takes precedence, including an explicit `false`.

```json
{
  "SafeSiegePlacement": false
}
```

This field controls only an installed and enabled AIC Tactics module. Existing
`HarassingSiegeEnginesMax` and the eight siege composition entries keep their
native meanings; this field does not change their limits or add AIC slots.
