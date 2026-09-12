# Integration verification, 12 September 2026

This records component evidence for `feat/combat-integration`, not release or
running-game acceptance. The native DLL builds with MSVC2005 SP1 `/W4 /WX /O2`.

The private, SHA256-pinned SHC 1.41 instruction host passes 2,380 recruitment/runtime
checks, 98 acquisition checks, 32 group allocation comparisons plus four input
checks, 37 damage comparisons plus 13 attribution checks, and 58 combat/reserve/raid
checks. These include native group allocation/add/remove during reserve handover
for all eight players, the 16-unit transfer bound, continuation of the remaining
unit, stable active group UIDs, target commitment, diplomacy/lord UID invalidation,
census admission and pending raid state in the integrity digest.

The FASM/Unicorn bridge oracle passes 480 comparisons against unchanged Legacy
defense census wrappers and 297 combat comparisons. It checks displaced native
effects, registers, flags, caller stack, callback arguments, admission/skip branches
and all three relocated damage prologues. C++ calls are ABI spies in this oracle;
the separate instruction host exercises their native logic.
The interval bridge additionally passes 11,520 original/Legacy interval, register,
flag and stack comparisons with the 344-byte configuration layout.

The integrated Lua suite passes 57 checks. Portable target and incident tests also
compile and pass with MSVC2005. Map Extensions has four required-state tests,
including read-only capture, strict validation and optional Native-only state.
Recorder's existing regression suite passed 401 tests and 2,386 subtests, with one
skip; the subsequent focused run of required-state and multiplayer-trace tests
passed 54 tests and 29 subtests. These tests do not replace native replay playback.

Dependent source checkpoints:

- AIC Loader: `24ea47c20f62082f8a1b06969089898a8285add9`.
- Map Extensions required-state API: `6a0d6743bcc05d82e3eea504dd4f5209c68f8293`.
- Recorder required-state capture/checkpoints: `dc86969ab447ad521508b97e0092d40d7bc27acc`.

The test harness originally collided with a native damage function below its
reference reservation. It now uses the damage probe's established linker layout,
placing host code after the original image. Production preflight also had an
ambiguous damage prologue search; it now verifies the established owner address.
Neither failure is represented as a successful gameplay test.

Still required: completed multiplayer content/config admission, actual new-policy
games and memory evidence, two complete reserve cycles, distinct/no-path raid
targets, two physical peers, exact saved continuation and offline replay/state
restore, native-only baselines and measured median/p95/p99/full-match performance.
No simulation-speed or replay acceptance claim follows from bounded code alone.
