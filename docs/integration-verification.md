# Integration verification, 12 September 2026

This records component evidence for `feat/combat-integration`, not release or
running-game acceptance. The native DLL builds with MSVC2005 SP1 `/W4 /WX /O2`.

The private, SHA256-pinned SHC 1.41 instruction host passes 2,380 recruitment/runtime
checks, 98 acquisition checks, 32 group allocation comparisons plus four input
checks, 37 damage comparisons plus 13 attribution checks, and 60 combat/reserve/raid
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

The integrated Lua suite passes 58 checks. Portable target and incident tests also
compile and pass with MSVC2005. Map Extensions has eight required-state tests,
including read-only capture, strict validation, optional Native-only state and
the installed framework proxy on Lua 5.4 and LuaJIT 2.1.
Recorder's regression suite passed 405 tests and 2,386 subtests, with one skip.
These tests do not replace native replay playback. Final boundary snapshot checks
confirm that its digest matches the observation and remains unchanged after live
raid state changes; a 128 KiB fixed buffer defers hashing until recording completion.
The subsequent final-boundary/session run passed 43 tests, including divergence
at tick 65 and retaining that boundary after menu code advances the live clock.
Cache-warm native copy/digest timings are recorded in `integrity-performance.md`;
they do not measure game ticks or establish the full performance gate.

Dependent source checkpoints:

- AIC Loader: `24ea47c20f62082f8a1b06969089898a8285add9`.
- Map Extensions required-state API: `04449b7f7b38dccf52e376a5fe62cc230fa5f596`.
- Recorder required-state capture/checkpoints: `7b6217fe256dacd8cc02e4ff1f67c67c96c2ed46` (0.50.5 preview).
- Protocol admission: `a6d940357432bbd63b87bbb26e6e67d973090eb8`.
- Unchanged Chat: `8f0c58a52cdc3aa5bca2cd4fd731ad1fcf1b9921`.

The test harness originally collided with a native damage function below its
reference reservation. It now uses the damage probe's established linker layout,
placing host code after the original image. Production preflight also had an
ambiguous damage prologue search; it now verifies the established owner address.
Neither failure is represented as a successful gameplay test.

Protocol's implemented host Start gate passed 1,152 actual FASM/native-instruction
comparisons, including registers, flags, stack, RNG, SP/MP and fail-closed branches.
Its consensus tests cover all eight peers, missing/mismatched replies, stale
rosters, host migration and malformed/non-lobby messages. These are transport
test doubles, not physical-peer admission evidence.

The packaged 8eeaf4b source reached the SHC 1.41 skirmish menu on GamerGrill with
all prerequisites and Recorder 87c0a10 loaded. Runtime preflight accepted the
344-byte configuration and state ABI 7; Recorder installed its replay hooks.
The private profile initially omitted UI's LuaJIT/cffi dependencies and their
option defaults. Correcting that profile resolved startup without a production
code change. A subsequent Green Haven 8-player spectator match reached tick 65.
Read-only memory confirmed the configured personalities and four adjacent allied
pairs: players 1/2, 3/6, 4/8 and 5/7. No recruitment or combat had occurred.

At the first checkpoint, Recorder failed because Map Extensions omitted the
framework proxy metadata for its returned snapshot tables. Map commit 04449b7
declares those detached copies; the regression reproduced the original failure
through the actual installed proxy and passes on both Lua runtimes after the fix.
The corrected package then ran to tick 1,118 with 17 recorded required-state
checkpoints (ticks 64 through 1,088). All four allied pairs were verified in
memory: 1/4 Wolf/Pig, 3/6 Saladin/Richard, 5/7 Wolf/Caliph, 2/8 Caliph/Saladin.
The snapshot contained valid enemy selections but no active attack or new hires.
Saving a replay copy exposed Recorder's rejection of native observer slot zero.
Recorder 615e54c accepts that slot only for commandless single-player recordings;
its full suite passes 410 tests and 2,386 subtests, with one skip. This correction
still needs native save/playback verification. Both test games closed normally.

Native loading of that starting save in PID26204 restored the same roster and
advanced to tick 2,207, but the older Recorder did not start recording. Refreshing
the existing Recorder owner's branch found its already-implemented fix 237570a:
the native load-return callback must inspect requestedView, because currentView
still refers to the load dialog. Recorder 7b6217f now includes owner 02014385,
preserving native retained-boundary memory, compact 1,024-tick release checks,
64-tick diagnostic checks and snapshot seeking. Its merged full suite passed
477 tests/3,189 subtests with one skip; two additional integration regressions
passed in a 59-test focused run. Initial and frozen worlds share required-state
export. This updated Recorder has not yet passed a fresh native AIC replay run.

Review chain: [Map Extensions PR3](https://github.com/gynt/ucp-extension-map-extensions/pull/3),
[Protocol PR3](https://github.com/gynt/ucp-extension-protocol/pull/3), and
[Recorder stacked PR3](https://github.com/Krarilotus/ucp_recorder/pull/3), based on
the existing upstream Recorder PR46 owner. AIC integration is
[PR17](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/pull/17),
following focused recruitment completion PRs13–16. All remain drafts.

Still required: actual multiplayer content/config admission, actual new-policy
games and memory evidence, two complete reserve cycles, distinct/no-path raid
targets, two physical peers, exact saved continuation and offline replay/state
restore, native-only baselines and measured median/p95/p99/full-match performance.
No simulation-speed or replay acceptance claim follows from bounded code alone.
