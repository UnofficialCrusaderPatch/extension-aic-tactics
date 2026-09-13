# extension-aic-tactics
Opt-in recruitment, attack and raid policies for AI personalities in UCP3; under development

Source 0.0.8 declares Crusader and Extreme 1.41 and uses verified UCP native
bindings. [Component evidence and remaining acceptance](docs/shared-native-bindings.md)
are recorded separately. Installed-runtime acceptance remains in progress.

Native contexts resolve during the framework's module-load phase, before Legacy
patches them. Activation checks the current hook bytes and retains Legacy's
target stability. Discovery uses UCP 3.0.7's existing cached `core.AOBScan`, as
established modules do. It requires no newer scanner or private address table.
The extra full-process duplicate scan that stalled earlier previews is removed.
The signed 0.0.7 bundle passes combined AIC/Recorder startup on stock secure
UCP 3.0.7 in Crusader and Extreme. An Extreme spectator match also verifies custom
recruitment and short offline replay with backward seeking. Its later active-combat
cold-load check failed: an untouched Native record's defaults depended on AIC
update order, so configuration validation rejected the save before restoring state.
Source 0.0.8 canonicalizes Native records and requires Map Extensions 1.1.2, whose
native error boundary stops rejected loads. Corrected-package native acceptance,
full combat, multiplayer and simulation-performance acceptance remain open.

The store uses `build.ps1` and `files.xml` to compile the x86 runtime and stage the same payload as local previews. Both paths share `tests/module_payload.py`, including all nine GUI descriptions and the multiplayer package identity.

AIC Tactics is licensed under the GNU General Public License, version 3
([GPL-3.0-only](LICENSE)). The license is included in the module ZIP starting
with 0.0.10. Dependencies retain their own licenses. Files' owner added its
GPL-3.0 license upstream; the dependent Files 1.4.2 package includes it.
