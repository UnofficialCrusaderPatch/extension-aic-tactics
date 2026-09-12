# Shared recruitment and identity bindings

Corrections after the [native audit](native-integration-audit.md). Shared pool
bindings are implemented; the remaining function/hook port and full Extreme
acceptance are still unfinished.

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

`native-bindings.lua` passes the resolved values into the DLL's 184-byte
`NativeBindings` structure before installing any AIC callbacks. Both Lua state
serialization and C++ integrity snapshots use these same bindings. Recruitment,
combat, raid observation and configuration admission consume the same clock.
The duplicated recruitment/combat RNG adapters now share `nativeRandom`, which
reads the existing sample and invokes the resolved native generator once.
No independent RNG or clock is introduced.

## Pools and native army layout

`native-layout.lua` uses eight identifying native instruction contexts through
the same framework scanner. It checks uniqueness, repeated capacity/stride
operands and agreement between allocation, membership, pathfinding and building
target writes. It decodes roots and the three army fields that move after
Extreme's larger membership bitset. OpenSHC's corresponding `UnitsState`,
`TribesState`, `Tribe`, `PlayerData`, `BuildingsState` and `EntityState` declarations
are reconstruction evidence; none is assumed to be an exported runtime API.
The framework and Loader provide no army/pool access API at the pinned revisions,
so these bindings stay with the existing AIC policy/native adapter owner.

| Native layout | Crusader fixture | Extreme fixture |
|---|---:|---:|
| Unit capacity / record bytes | 2,500 / 1,168 | 10,000 / 1,168 |
| Army capacity / record bytes | 1,250 / 820 | 1,250 / 1,672 |
| Membership words covering the unit pool | 157 | 625 |
| Building capacity / record bytes | 2,000 / 812 | 2,000 / 812 |
| Projectile capacity / record bytes | 3,000 / 232 | 6,000 / 232 |

The native allocator still partitions army IDs between the eight players.
Its function and native membership/path functions are reused. No pools are
expanded. Recruitment, census, reserves, raids and save validators consume
these shared bounds; reserve/raid transfer remains capped at 64 membership
words and 16 transferred units per update. Saved cursor validation now covers
Extreme's final word and rejects the first word beyond the pool.

This also fixes a classic projectile-attribution bug: the production combat
adapter's former array origin was 36 bytes past the actual first record. The
isolated observation fixture used the correct origin and missed that wiring
error. The new binding derives the origin from the native owner-field operand,
cross-checks projectile type and alliance operands, and is exercised through
the production damage bridge.

Legacy source is unchanged. Inspected `ai_attackwave.lua` patches the native
wall/building/breach dispatch sites, outside these eight contexts. Full enabled
Legacy composition testing remains part of the unfinished hook integration.

`native-group-actions.lua` adds twelve contexts for the native assignment,
group-selection/removal, return and raid-order boundaries. Full assignment and
membership functions identify the intended role and ABI; decoded operands must
agree with the already resolved player/unit/tribe roots, stride, IDs and UIDs.
Allocator and membership call targets must agree with the existing bindings.
The native building-attack and movement callers must use the same five-argument
order relay, whose jump must reach the identified native group dispatcher.
The game still owns group allocation, movement and order submission.

Raid map rows and offensive role-to-group slots are decoded from those native
consumers. C++ no longer casts fixed addresses for these group actions/tables.
`native-context.lua` shares the framework uniqueness/call checks across layout,
group actions and recruitment; it adds no private scan cache or patch manager.

## Recruitment and Legacy census composition

`native-recruitment.lua` adds eleven contexts for the sortie functions, their
scheduler, recruitment interval/opportunity, moat vacancy query and surviving
Legacy defense-census instructions. The full native sortie bodies establish the
AIC fields, role assignment and thiscall argument/cleanup convention. Both
sorties must call the same acquisition and group owners. The moat recruitment
and active-group callers must agree on the singleton and identified query.
Scenario fields are decoded from recruitment's existing override predicate.

Legacy's `ai_defense` keeps its counter and source. Its implementation allocates
that counter privately and exports no accessor. AIC discovers the unmodified
instructions following the three patches, validates their native callers/fields,
then validates every trampoline instruction, shared counter operand and return
destination before chaining. It also verifies its own installed chain identities.
The framework AOB cache deliberately invalidates a signature whose bytes were
patched; relying on a stale pre-Legacy cached address would be incorrect.

`native.lua` now uses these resolved sites and operands for every recruitment
patch and displaced instruction. It uses the existing framework assembly/write
facilities; `core.detourCode` is a Lua callback boundary rather than the required
native C++ callback ABI. No counter, scan cache or patch manager was introduced.

Validation performed:

- Actual SHC and Extreme instruction images resolve the complete context and
  Loader storage; all 31 timer settings match unchanged Legacy patch bytes.
- The recruitment binding ABI is checked against actual Lua writes on both images;
  repeated timer changes perform zero scans. Classic calendar test executes
  9,600 ticks. That calendar test has not been run on Extreme.
- Ten failure checks cover missing/ambiguous/RNG-layout contexts and invalid
  DLL/Loader contracts, rejecting before native binding writes.
- `tests/check_layout_native.py` resolves all eight contexts on each actual
  fixture; each passes 30 negative resolution/operand checks, 32 original
  allocator cases and four original membership cases including the last valid
  unit ID. It verifies native stack cleanup, membership, group UID and leader.
- Its group-action checks resolve twelve additional contexts per executable,
  reject 38 absent/ambiguous/inconsistent action bindings, and execute four
  native removals plus five native role assignments using the final valid unit
  ID. Empty-group order/return calls check stack cleanup separately; active
  pathfinding and battlefield orders still require the gameplay acceptance.
- Classic MSVC2005 SP1 native harness passes 2,380 runtime, 98 acquisition,
  37 damage, 13 damage-validation, three production projectile-attribution,
  32 tribe-allocation, four tribe-input and 65 combat/reserve/raid-layout checks.
  The latter include detached Extreme-sized pools and unit ID 9,999; this is
  consumer evidence, separately from the original Extreme instruction tests.
- Three additional save-state tests cover high IDs/cursors, rejection beyond
  the native boundary and index reconstruction using resolved roots/stride.
- `tests/check_defense_bridge.py` runs unchanged Legacy Lua, production discovery
  over the resulting patched image and actual AIC FASM wrappers on both fixtures.
  Each passes 480 census equivalence cases, 48 opportunity-bridge cases, 24
  native sortie/moat cases, 37 negative resolution/operand cases and 81 negative
  Legacy trampoline-byte checks. Repeated preflights perform no scans. Sortie
  cases cover the no-personality return path; these do not prove recruitment in
  an active game. The classic combat-wrapper regression also passes 297 cases.
- The actual interval wrapper passes 11,520 FASM/Unicorn cases covering explicit
  Legacy interval fallback, opted-in intervals, registers, flags and caller stack.
- Seventy-six AIC Python/Lua tests and forty Loader tests passed. The initial
  full AIC run lacked `AICLOADER_TEST_ROOT`; rerunning with the actual prerequisite
  checkout passed.

The private `tests/check_grace_native.py` runner now accepts `--variant SHC` or
`--variant SHCE`, `--loader`, `--legacy`, `--reference` and `--output`. Fixture
hashes remain in the research tests only. Exact-build harness addresses stay
in test code. No new game, MP, replay or performance acceptance is claimed.

Source version 0.0.3 requires Loader 1.1.4 and Protocol 1.1.1. No 0.0.3 tester
bundle is published while the remaining native integration is incomplete.

Remaining native work includes combat Lua hook discovery/Legacy composition, the other
native function casts and data tables, and verification of every reused player
field against all required executable variants. Pool discovery alone does not
establish those capabilities or whole-game compatibility.
