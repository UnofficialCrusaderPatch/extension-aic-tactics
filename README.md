# extension-aic-tactics
Opt-in recruitment, attack and raid policies for AI personalities in UCP3; under development

Source 0.0.7 declares Crusader and Extreme 1.41 and uses verified UCP native
bindings. [Component evidence and remaining acceptance](docs/shared-native-bindings.md)
are recorded separately. Installed-runtime acceptance remains in progress.

Native contexts resolve during the framework's module-load phase, before Legacy
patches them. Activation checks the current hook bytes and retains Legacy's
target stability. Discovery uses UCP 3.0.7's existing cached `core.AOBScan`, as
established modules do. It requires no newer scanner or private address table.
The extra full-process duplicate scan that stalled earlier previews is removed.
The revised source still needs installed acceptance on the stock framework.

The store uses `build.ps1` and `files.xml` to compile the x86 runtime and stage the same payload as local previews. Both paths share `tests/module_payload.py`, including all nine GUI descriptions and the multiplayer package identity.
