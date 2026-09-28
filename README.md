# AIC Tactics

AIC Tactics adds optional per-personality recruitment, opponent, raid and siege policies to UCP 3. Existing AIC behavior remains the default. An explicit setting in a personality's effective AIC takes precedence over a module fallback, including `false` and valid zero.

For the test bundle and installation steps, see [the testing guide](docs/runtime-testing.md). The [AIC parameter reference](locale/description-en.md) lists recruitment, attack and raid settings; [siege configuration](docs/configuration-siege.md) gives siege defaults and a per-AI example. Legacy replacement settings and migration are in [the compatibility matrix](docs/compatibility-matrix.md).

The native module uses the cached AOB scanner shipped with UCP 3.0.7 and verified Crusader/Extreme 1.41 bindings. The x86 build, policy tests and six executable binding fixtures pass. The signed 0.0.15 bundle started to the Extreme menu; an earlier 0.0.14 bundle cold-loaded a native save during an attack. Siege construction, harassment, replay and simulation overhead still need live gameplay acceptance. [Binding and acceptance evidence](docs/shared-native-bindings.md) separates those checks from component tests.

The module is licensed under [GPL-3.0-only](LICENSE). Required module dependencies keep their own licenses.
