# AIC Tactics

AIC Tactics adds optional per-personality recruitment, opponent, raid, attack patrol and siege policies to UCP 3. Every feature keeps the game's behaviour by default, including the engineer-counting and safe-siege fixes. An explicit siege setting in a personality's effective AIC takes precedence over a module fallback, including `false` and valid zero, except that advanced siege features require safe placement.

AIC Tactics requires the UCP 3.0.8 unique code scanner, AIC Loader 1.1.10 and Map Extensions 1.2.2.

For the test bundle and installation steps, see [the testing guide](docs/runtime-testing.md). The [JSON Schema](schema/aic-tactics.schema.json) describes the extension fields. [Siege configuration](docs/configuration-siege.md) gives defaults and a per-AI example. [Attack patrol configuration](docs/configuration-attacks.md) lets every patrol group join an assault, splits patrols into more groups on ring routes and stations siege-defence groups in front of their engines, split into more groups if wanted. How UCP2-Legacy options combine with AIC Tactics is in [the compatibility matrix](docs/compatibility-matrix.md).

Customisations shows the patrol, siege escort and crews, lord hunt and ally help choices under **AI → Improvements**. Presets can still set the other module values by their existing keys. See the [engineer](docs/configuration-engineers.md), [siege](docs/configuration-siege.md), [patrol](docs/configuration-attacks.md), [lord hunt](docs/configuration-lord-hunt.md) and [ally help](docs/configuration-allies.md) guides for the settings and defaults. The preset value `diagnostics: true` enables the AI decision log.

The native module uses the unique AOB scanner shipped with UCP 3.0.8 and verified Crusader/Extreme 1.41 bindings. The x86 build, policy tests and six executable binding fixtures pass. The signed 0.0.15 bundle started to the Extreme menu; an earlier 0.0.14 bundle cold-loaded a native save during an attack. Siege construction, harassment, replay and simulation overhead still need live gameplay acceptance. [Binding and acceptance evidence](docs/shared-native-bindings.md) separates those checks from component tests.

The module is licensed under [GPL-3.0-only](LICENSE). Required module dependencies keep their own licenses.
