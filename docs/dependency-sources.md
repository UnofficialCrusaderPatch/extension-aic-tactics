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

Recorder is a separate prerequisite from <https://github.com/Corax34/ucp_recorder>.
Its required-state changes are not included in this gameplay ZIP. Use the exact
dependent recorder build for recorder acceptance. Original game/framework binaries
are not bundled, and no modified Legacy source is included.
