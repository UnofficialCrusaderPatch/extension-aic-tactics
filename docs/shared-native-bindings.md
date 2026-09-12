# Shared recruitment and identity bindings

Corrections after the [native audit](native-integration-audit.md). The AIC production
function, hook and pool bindings now use native discovery. Full installed-framework,
gameplay and variant acceptance remains unfinished.

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

`native-bindings.lua` passes the resolved values into the DLL's 256-byte
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

Legacy source is unchanged. Inspected `ai_attackwave.lua` patches native
wall/building/breach dispatch sites outside these pool contexts. Actual Legacy
defense and target-policy composition checks are recorded below; the complete
installed baseline remains a gameplay acceptance requirement.

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

## Native AIC queries and recruitment tables

`native-aic-queries.lua` resolves the complete native building filter, attack-role
selector and raid-limit function, and the native consumers of the defense/raid
unit lists, special-defender list and defensive group slots. Identifying loops
encode the 20-entry and seven-entry bounds; repeated table operands and record
fields must agree. The native acquisition function's equipment-recipe consumer
is verified at its already resolved address, without another scan. Recipes remain
owned by native acquisition, including armor and horses. The recruitment caller
must reach the identified building/role queries with their existing thiscall ABI.
No fixed executable addresses remain in production `src/runtime.cpp` or
`src/army.cpp`. The combat and raid adapters use the same binding structure.

## Combat, raid and targeting hooks

`native-combat-bindings.lua` resolves 23 identifying contexts for native targeting,
nervousness, attack/raid state updates, return orders, combat values, damage,
unit/building census, wave readiness, tunneler membership and scheduler callers.
Repeated operands must agree with the shared pools, player fields, tick, census
indices and original owners. The combat-value switch table and all dispatch
indices are validated. Buy/sell price arithmetic is shared, so the native caller
identifies the price function and its body is verified in place; the decoded
price field is checked against the player layout on each game family.

`combat-native.lua` consumes named hook sites and captured original instructions.
It rejects occupied bytes and redirected calls before patching. Existing
framework allocation/write facilities preserve the displaced instructions and
original native calls; the three damage gateways adapt native C++ callbacks.
Framework `core.hookCode`/`detourCode` expose Lua callbacks, so using them here
would add Lua dispatch to every observed hit. No additional dispatcher was added.
The explicit Legacy target-policy replacement reaches the original selection
paths and is compared with unchanged `ai_attacktarget.lua` below.

No fixed executable addresses remain in AIC production Lua/C++ bindings. The
remaining long hex values are integer limits, digest constants and a building
cost-table field offset. Hash-pinned executable addresses remain in private tests.
Framework `core.AOBScan` uses its existing cache; RPS 1.5.2 scans committed image
regions, while the Lua wrapper supplies its documented range. Installed-process
scanner/module composition still needs the real-game acceptance below.

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
- `tests/check_aic_queries_native.py` resolves nine query/table contexts on both
  actual images and rejects 35 missing/ambiguous/inconsistent/occupied bindings.
  Each executes 1,920 native role-selection cases (including main-roster cursor
  writes), 1,280 native raid-limit cases and five native building-filter cases,
  including the highest valid building record. Acquisition recipes are verified
  at the existing function binding rather than scanned again.
- Both actual images pass 81 combat binding/operand/switch failures, 200 native
  price calls, 80 native combat-value calls (identical output vectors), and 40
  native lifecycle early-return/nervousness cases. These do not test active armies.
- Actual FASM combat/raid/damage gateways pass 297 cases on each image, preserving
  displaced effects, callback arguments, registers, flags and stack. Ninety-one
  occupied hook-byte/call checks reject before allocation. Forty-five cases per
  image compare the explicit target-policy replacement with unchanged Legacy
  through the original selection loop, including its stack and flags effects.
- Seventy-six AIC Python/Lua tests and forty Loader tests passed. The initial
  full AIC run lacked `AICLOADER_TEST_ROOT`; rerunning with the actual prerequisite
  checkout passed.

The private `tests/check_grace_native.py` runner now accepts `--variant SHC` or
`--variant SHCE`, `--loader`, `--legacy`, `--reference` and `--output`. Fixture
hashes remain in the research tests only. Exact-build harness addresses stay
in test code. No new game, MP, replay or performance acceptance is claimed.

Source version 0.0.3 requires Loader 1.1.4 and Protocol 1.1.2 (which requires
Files 1.4.0 for shared asset traversal). No 0.0.3 tester
bundle is published while the remaining native integration is incomplete.

Remaining acceptance includes the complete installed framework/Legacy baseline,
all reused field/layout paths in active games, applicable language/distribution
fixtures, native-default equivalence, mixed personalities, physical two-peer MP,
save/load, Recorder state restore/replay and measured whole-game performance.
Shared asset traversal is now owned by [Files PR11](https://github.com/UnofficialCrusaderPatch/extension-files/pull/11),
used by [Protocol 1.1.2](https://github.com/Krarilotus/ucp-extension-protocol/pull/1)
and [Recorder 0.50.6](https://github.com/Krarilotus/ucp_recorder/pull/4).
Their portable tests and CI pass; installed bundle acceptance remains pending.
[Recorder's Windows-service correction](https://github.com/Krarilotus/ucp_recorder/pull/5)
uses the existing RPS export resolver and passes a native console check with
the installed Lua/RPS/CFFI binaries. Recorder's game-address correction now reaches
[PR25](https://github.com/Krarilotus/ucp_recorder/pull/25), source 0.50.27, with the
fixed profiles/header whitelist removed and shared context helpers consolidated.
Combined architecture review and gameplay acceptance remain unfinished.

## Official distribution fixtures

The same component runners now also pass on official EFIGS and Polish Crusader
1.41 and Extreme 1.41.1-E images. Together with the two original local images,
this covers six reference executables. `tests/executable_fixtures.py` records
their identities for reproducible research; the production module never imports
that file or uses those hashes for runtime binding.

Each additional image passed original allocation/membership/removal/assignment,
recruitment role and raid-limit queries, building filters, combat-value/price
queries, native lifecycle early returns, unchanged Legacy census and target
selection comparisons, actual FASM gateways and their negative binding tests.
Grace checks compare all 31 Legacy month settings and Loader's actual storage
metadata. The Extreme grace runner does not execute a calendar; none of these
checks claims active-army behavior, live multiplayer or save/replay acceptance.
The additional fixtures required no production signature or layout change.

## Shared main-executable uniqueness

Source 0.0.4 uses `core.AOBScanUnique` when the framework provides it. Its existing
cache/RPS owner checks the first two overlapping matches in the main executable's
code. A rejection is final: the module never retries through a less restrictive
scan. Stock UCP 3.0.7 keeps the cached AoB plus second-scan path, but an actual
signed installation failed the startup performance gate: after 134.17 seconds
and 131.73 CPU seconds it was still enabling AIC, without a game window.
Current acceptance therefore uses the complete signed secure framework preview;
the shared scanner's final release/minimum-version contract remains outstanding.

Inspected framework `1d78391` ([PR149](https://github.com/UnofficialCrusaderPatch/UnofficialCrusaderPatch3/pull/149))
and RPS `e958409` ([PR16](https://github.com/gynt/RuntimePatchingSystem/pull/16)).
The actual framework core/cache resolves all 64 AIC instruction contexts and 67
resulting fields on all six reference images through a main-code, overlapping
PE oracle. Lua 5.4/LuaJIT tests reject failed or invalid owner results without
falling back. Legacy-path native component checks remain unchanged. This is
component coverage; startup timings require the installed runtime and cannot be
inferred from the private-image scanner. No private scanner, range parser or
per-tick discovery was introduced.

## Load before Legacy patches

The signed framework preview reached AIC enable in 1.745 seconds, then rejected
`targetSelection`: Legacy `ai_assaultswitch` had already patched eight bytes
inside the complete identifying signature. Raw executable fixtures had not
covered that load-order interaction.

Source 0.0.6 uses the framework's existing two-phase lifecycle: `code/main.lua`
loads every module before enabling any module. `init.lua` resolves read-only
bindings at load; `native.new(game)` writes the DLL bindings during enable.
Loader's `getNativeAICLayout` is available in that phase. Legacy retains its
target-stability patch; signatures and activation-time checks are unchanged.
No native DLL is loaded or patched by read-only discovery.

The updated `check_defense_bridge.py` executes the actual AIC entry point,
applies unchanged Legacy 2.15.2 defense and assault-switch ports, proves that
late target discovery would fail, and consumes the saved bindings without a
second scan. All six reference images pass, including 128 original Legacy
target-commitment cases, 480 census comparisons, 297 combat gateway cases and
91 occupied-hook/call rejection cases per image. These are instruction-level
composition checks; the corrected signed module still needs live-game acceptance.


## Existing UCP 3.0.7 scanner (source 0.0.7)

User direction supersedes the provisional newer-scanner prerequisite above.
`native-context.find` uses the shipped `core.AOBScan(signature)` and its cache,
matching unchanged Legacy `port/ai_assaultswitch.lua`. Actual framework
`content/ucp/code/core.lua` and `data/cache.lua` were inspected: unbounded
AOBScan uses cache.AOB.retrieve, which revalidates cached matches. There is no
extension cache, second full-process scan, optional newer API or fixed address
fallback. Binding discovery still runs before Legacy enable; complete identifying
signatures, decoded cross-owner ABI/layout checks and occupied-hook checks remain.

Uniqueness of identifying contexts is checked against supported images in offline
fixtures. Stock AOBScan returns its first match; the extension does not claim an
exhaustive runtime duplicate search on arbitrary modified executables. The extra
runtime duplicate search in earlier previews caused the observed startup stall.
The stock-runtime startup/composition check is pending for this revision.
