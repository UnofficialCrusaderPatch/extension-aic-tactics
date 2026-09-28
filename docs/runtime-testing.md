# Test AIC Tactics 0.0.17

Download the signed test bundle linked in [Store PR #44](https://github.com/UnofficialCrusaderPatch/UCP3-extensions-store/pull/44). Extract **the outer bundle** and copy its `ucp/modules/*.zip` and matching `*.zip.sig` files into the game's `ucp/modules` folder. Keep the module ZIPs zipped. In UCP 3.0.7, enable AIC Tactics and its required modules, then launch Crusader or Extreme 1.41.

In a copy of one personality's `character.json`, add these keys **inside `aic`**:

```json
"LargerSiegeForces": true,
"SiegeForceMax": 10,
"SafeSiegePlacement": true,
"ActualSiegeResourcePayment": true,
"CoordinatedSiegeHarassment": true,
"SiegeHarassMinEngines": 3,
"HarassingSiegeEnginesMax": 10
```

Give a neighboring opponent `"LargerSiegeForces": false`, `"SiegeForceMax": 0` and `"CoordinatedSiegeHarassment": false`, then start a new match. The AIC value wins over the module fallback. When larger siege forces are enabled, zero lets available engineers decide the force size. Compare equipment counts and engineer survival during an attack, and watch whether harassment engines approach together and fire from range. Try a crowded building site and low stone, pitch or gold with payment enabled, then save and reload mid-attack. Engineer quota counting is built in; Fixed Engineers handles general crew lifecycle separately.

Please report the game variant, AIC values, observed behavior and save/replay result on the PR. The previous 0.0.14 bundle cold-loaded an Extreme 1.41.1-E save with an attack in progress. This 0.0.17 source has not yet completed live siege construction, crowded placement, resource shortage, harassment, replay or measured simulation-overhead checks. Multiplayer testing is left to players.
