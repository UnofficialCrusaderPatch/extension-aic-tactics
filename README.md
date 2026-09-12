# extension-aic-tactics
Opt-in recruitment, attack and raid policies for AI personalities in UCP3; under development

The current 0.0.2 preview supports SHC 1.41 only; **do not use it with Extreme**.
The [native integration audit](docs/native-integration-audit.md) records the
binding/layout corrections and acceptance still required before release.

Source 0.0.6 declares Crusader and Extreme 1.41 and uses verified UCP native
bindings. [Component evidence and remaining acceptance](docs/shared-native-bindings.md)
are recorded separately. Installed-runtime acceptance remains in progress.

Native contexts resolve during the framework's module-load phase, before Legacy
patches them. Activation checks the current hook bytes and retains Legacy's
target stability. The signed framework preview in
[PR149](https://github.com/UnofficialCrusaderPatch/UnofficialCrusaderPatch3/pull/149)
is required for current acceptance: stock 3.0.7's full-process ambiguity scans
failed the startup performance gate.

The store uses `build.ps1` and `files.xml` to compile the x86 runtime and stage the same payload as local previews. Both paths share `tests/module_payload.py`, including all nine GUI descriptions and the multiplayer package identity.
