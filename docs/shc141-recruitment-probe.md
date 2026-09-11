# Original recruitment eligibility adapter

`SHC141::queryRecruitment` calls the existing unit-acquisition owner in check-only
mode, using the Microsoft x86 thiscall ABI. It does not spend resources or
convert a peasant. It restores the two recruitment diagnostics after the query
and, for knights, all eight cached available-horse counts refreshed by the native
helper. Result diagnostics are detached; a stale required-resource value is not
returned for failures other than missing equipment.

The original entry points are euroRecruit at 0x52E960, nonEuroRecruit at 0x52EC10,
and the knight helper recountStablesAndHorses at 0x4598B0, on the reference
SHC 1.41 executable with SHA256
`3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a`.
Both acquisition routines return before conversion/spending when their fourth
argument is nonzero, but still write failure diagnostics. The knight helper
clears and recounts available horses for every player, even on a failed query.
This adapter deliberately isolates those query side effects. Original actual
acquisition must still run afterward for the chosen candidate.

The whitelist admits original European troop IDs 22–28 and the inspected
non-European acquisition IDs 5, 29, 30, 37, 70–76. Unknown types are rejected
before indexing the European equipment tables. Player IDs must be 1–8 and the
building ID positive. Those checks do **not** establish that a building is live,
owned or able to recruit: the caller must use the native AIC building/scenario
gates, quota checks, group capacity and valid ID/UID ownership. No role eligibility
mask may be derived from affordability alone.

There is no gameplay hook or bootstrap in this change. The eventual binding
must verify the original routines and all invoked helpers before supplying the
service pointers. An already-detoured or otherwise unsupported callee cannot
be treated as the original merely because its address matches. No exception or
hardware fault is swallowed. Run on the native simulation thread with no
reentrant callback or concurrent observer; this is not a general pure API for
arbitrary third-party hooks. Default Native behavior must bypass the adapter.

## Verification

`tests/build_shc141_probe.ps1` compiles with MSVC2005 SP1 x86, /W4 /WX /O2,
and runs `tests/run_shc141_probe.py`. The runner hash-checks a private licensed
reference executable and produces an ignored local image and JSON result.
No original binary bytes belong in the repository or release artifact.

The console test reserves address space in its own executable image. Its host
code occupies the low, unused part; the original routines and game globals are
copied at their unchanged preferred addresses above that host code. A guard
rejects any host layout overlapping required routines. This avoids overwriting
an unrelated allocation or mapping in the test process. No existing game process
is opened or modified, and no game UI is launched.

The test runs original machine instructions directly with synthetic UnitsState,
player and building data. It compares the complete synthetic UnitsState and
the reference address range before/after every query. Cases cover all admitted
types, missing money/equipment/peasants, busy and state-zero peasants, horse
availability, all player slots, and rejected types/arguments/services.

This checks x86 calling convention and bounded state isolation. It is not a
running-game compatibility result, full acquisition test, hook-composition test,
Extreme test, multiplayer, save/replay acceptance or performance benchmark.
The public Windows CI builds the ABI adapter/test host with its current compiler;
it cannot execute the private original-image cases and does not replace the
local MSVC2005 verification. Linux CI continues to test the portable policy core.
