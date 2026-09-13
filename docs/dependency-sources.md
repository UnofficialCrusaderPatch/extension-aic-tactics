# Dependency sources in the development ZIP

`test-manifest.json` records exact source revisions and SHA256 hashes for every
packaged file. AIC Loader's Lua source is included directly from
<https://github.com/UnofficialCrusaderPatch/extension-aicloader>.

The Map Extensions Lua prerequisite is based on
<https://github.com/gynt/ucp-extension-map-extensions>. Its GPLv3 license is included
at `ucp/modules/map-extensions-1.1.0/LICENSE`. The unchanged `luamemzip.dll` comes
from the checksum-pinned Map Extensions 1.0.0 asset in the UCP 3.0.7 store release;
its asset URL and checksum are in the manifest. Its source owner is
<https://github.com/gynt/luamemzip>; the base Map Extensions tree pins that submodule
at `101f73fc05147be856edb0331c5193122741fd56`.

Protocol's admission prerequisite is based on
<https://github.com/gynt/ucp-extension-protocol>, with the current source revision
recorded in the manifest. Files 1.4.0 comes from
<https://github.com/UnofficialCrusaderPatch/extension-files> and owns bounded
VFS traversal shared by Protocol and Recorder. Chat is the
unchanged <https://github.com/gynt/ucp-extension-chat> source
`8f0c58a52cdc3aa5bca2cd4fd731ad1fcf1b9921`. Their source and licenses are included
under their module directories. Chat's existing local display reports admission
status; its existing chat availability changes also apply when enabled.

Recorder is a separate prerequisite from <https://github.com/Corax34/ucp_recorder>.
Its required-state changes are not included in this gameplay ZIP. Use the exact
dependent recorder build for recorder acceptance. Original game/framework binaries
are not bundled, and no modified Legacy source is included.

AIC Tactics uses GPL-3.0-only with the author's approval on 13 September 2026.
The root `LICENSE` is the unchanged GPL version 3 text also present in Map
Extensions (upstream license blob `f288702d2fa16d3cdf0035b15a9fcbc552cd88e7`).
The shared payload builder includes it in local and Store module ZIPs from 0.0.10.

Release provenance remains incomplete: the inspected Files tree has no root
license file. Its upstream owner's permission is tracked separately in
<https://github.com/UnofficialCrusaderPatch/extension-files/issues/12>.
The AIC license does not grant permission for dependency code.
