# Test AIC Tactics 0.0.14

Download the signed test bundle linked in [PR #27](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/pull/27). Extract **the outer bundle** and copy its `ucp/modules/*.zip` and matching `*.zip.sig` files into the game's `ucp/modules` folder. Keep the module ZIPs zipped. In UCP 3.0.7, enable AIC Tactics and its required modules, then launch Crusader or Extreme 1.41.

In a copy of one personality's `character.json`, add these keys **inside `aic`**:

```json
"LargerSiegeForces": true,
"SiegeForceMax": 10,
"SafeSiegePlacement": true,
"ActualSiegeResourcePayment": true
```

Give a neighboring opponent `"LargerSiegeForces": false` and `"SiegeForceMax": 0`, then start a new match. The AIC value wins over the module fallback; zero keeps one ordinary assault batch. Compare equipment counts and engineer survival during an attack. Try a crowded building site and low stone, pitch or gold with payment enabled, then save and reload mid-attack. The other siege controls are `CorrectEngineerRoleCounting`, `CoordinatedSiegeHarassment` and `SiegeHarassMinEngines`; see [the field reference](configuration-siege.md) for their ranges and defaults. Fixed Engineers handles general crew lifecycle separately.

Please report the game variant, AIC values, observed behavior and save/replay result on the PR. The signed 0.0.14 bundle has loaded in an isolated Extreme 1.41.1-E skirmish, and that skirmish has cold-loaded from a native save. Larger-force construction, crowded placement, resource shortages, harassment, replay and measured simulation overhead still need gameplay acceptance. Multiplayer testing is left to players.
