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

Release provenance remains incomplete: AIC Tactics and the inspected Files tree
have no root license file. Do not treat packaging success as license clearance
or a release-ready result.
