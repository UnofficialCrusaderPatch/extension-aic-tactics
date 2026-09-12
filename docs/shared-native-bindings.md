# Shared recruitment and identity bindings

First correction after the [native audit](native-integration-audit.md); this
does not complete AIC's unit, group, building or hook port to Extreme.

`config/grace.lua` resolves the original recruitment/RNG context through
framework `core.AOBScan`, with a second framework scan rejecting ambiguity.
The context is the same owner used by unchanged Legacy
`port/ai_recruitstate_initialtimer.lua`. It decodes the tick operand, timer
immediate, RNG singleton/value and relative next-RNG call. The 16-bit RNG value
must be two bytes into that singleton. Every subsequent preflight checks the
resolved original instructions, permitting only the module's own timer value
change. There are no repeated scans or fixed-address fallbacks.

Loader 1.1.4's `getNativeAICLayout` exposes the storage already resolved by its
`addresses.lua:getAIStartAddress`. The existing value getter could read one
field at a time, but it did not expose the contiguous native record layout
needed by the bounded C++ identity snapshot. This small module-only API keeps
that ownership in Loader. Its version, 16-record count and 676-byte stride are
validated before writing native binding memory.

`native-bindings.lua` passes the six resolved values into the DLL's 24-byte
`NativeBindings` structure before installing any AIC callbacks. Both Lua state
serialization and C++ integrity snapshots use these same bindings. Recruitment,
combat, raid observation and configuration admission consume the same clock.
The duplicated recruitment/combat RNG adapters now share `nativeRandom`, which
reads the existing sample and invokes the resolved native generator once.
No independent RNG or clock is introduced. The remaining classic-specific
adapters are still explicitly unfinished.

Validation performed:

- Actual SHC and Extreme instruction images resolve the complete context and
  Loader storage; all 31 timer settings match unchanged Legacy patch bytes.
- The 24-byte binding ABI is checked against actual Lua writes on both images;
  repeated timer changes perform zero scans. Classic calendar test executes
  9,600 ticks. That calendar test has not been run on Extreme.
- Ten new failure checks cover missing/ambiguous/RNG-layout contexts and invalid
  DLL/Loader contracts, rejecting before native binding writes.
- MSVC2005 SP1 runtime build passes. Classic native harness passes 2,380 runtime,
  98 acquisition, 37 damage, 13 damage-validation, 32 tribe-allocation, four
  tribe-input and 60 combat/reserve/raid-layout checks.
- Seventy-three AIC Python/Lua tests and forty Loader tests pass. The initial
  full AIC run lacked `AICLOADER_TEST_ROOT`; rerunning with the actual prerequisite
  checkout passed.

The private `tests/check_grace_native.py` runner now accepts `--variant SHC` or
`--variant SHCE`, `--loader`, `--legacy`, `--reference` and `--output`. Fixture
hashes remain in the research tests only. Exact-build harness addresses stay
in test code. No new game, MP, replay or performance acceptance is claimed.

Source version 0.0.3 requires Loader 1.1.4 and Protocol 1.1.1. No 0.0.3 tester
bundle is published while the remaining native integration is incomplete.
