# Engineer role counting in an individual AIC

`CorrectEngineerRoleCounting` is an optional boolean in each AI's effective AIC.
`true` counts engineers who are currently assigned to a defense, raid, sortie or
attack group toward that role's native quota. Engineers assigned to siege
construction, mounted on equipment or doing oil duty do not fill those quotas.
Role 10's original engineer count and moat digger accounting stay separate.

```json
{
  "CorrectEngineerRoleCounting": false
}
```

An explicit AIC value takes precedence. When the field is absent, that AI uses
the module's **Count engineers in AI troop quotas** fallback, default ON.
`false` keeps the native role recount. The field has no effect when AIC Tactics
is not enabled. Fixed Engineers 0.2.0 independently handles general siege-crew
lifecycle fixes; it is not a required dependency for this quota policy.
