# extension-aic-tactics
Opt-in recruitment, attack and raid policies for AI personalities in UCP3; under development

The current 0.0.2 preview supports SHC 1.41 only; **do not use it with Extreme**.
The [native integration audit](docs/native-integration-audit.md) records the
binding/layout corrections and acceptance still required before release.

Source 0.0.5 declares Crusader and Extreme 1.41 and uses verified UCP native
bindings. [Component evidence and remaining acceptance](docs/shared-native-bindings.md)
are recorded separately; no 0.0.5 download or release acceptance is claimed yet.

The store uses `build.ps1` and `files.xml` to compile the x86 runtime and stage the same payload as local previews. Both paths share `tests/module_payload.py`, including all nine GUI descriptions and the multiplayer package identity.
