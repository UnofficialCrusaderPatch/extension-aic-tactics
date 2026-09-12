# UCP integration audit, 12 September 2026

## Correction status

The findings below describe the published 0.0.2 bundle and its original revisions.
Source 0.0.3 now uses runtime bindings and native layout/ABI checks; its component
tests pass on six local/official EFIGS/Polish Crusader 1.41 and Extreme 1.41.1-E
fixtures. See [current bindings and evidence](shared-native-bindings.md).
Recorder's dependent [binding stack through PR25](https://github.com/Krarilotus/ucp_recorder/pull/25)
replaces the fixed profiles and temporary header/hash whitelist, reuses the
framework version API and consolidates context verification. Required shared
capabilities remain in Loader, Files, Map, Protocol and UI.

The old 0.0.2 download is still SHC-only. No corrected combined bundle, live
match/save/replay, physical multiplayer or whole-game performance acceptance is
claimed here. Licensing/provenance, combined architecture review, CI/review and
normal merges remain release work.

## Original bundle findings

**Result: the current bundle does not meet the native integration release
requirements. Do not test AIC Tactics 0.0.2 with Extreme.** Its manifest correctly
declares only SHC 1.41, but its fixed runtime bindings must still be corrected
before it can become the intended reusable UCP extension. Component test counts
and a successful SHC startup do not waive this requirement.

## Basis and reproducible comparison

Inspected actual framework `02a7a6bc8ab956a91fc752e8c8ed215c149855e7`,
Legacy `caa50aba9fc85c5fc766c413b23085ddfbba4a79`, AIC `7dff60ef6af7`,
Loader `7e92bd4722d0`, Map `04449b7f7b38`, Protocol `a6d940357432`,
Chat `8f0c58a52cdc`, and Recorder `7b6217fe256d`. Native integration contract:
`Roadmap/NATIVE-INTEGRATION-CONTRACT.md` in the shared UCP workspace.

The read-only investigation script
`Roadmap/Investigations/AIC-Tactics/audit_native_bindings.py` records source
revisions, literal AoB calls, all match addresses and candidate absolute-address
lines in `native-binding-audit.json`. It scans the mapped images of these private
licensed references without opening a game process:

| Executable | SHA256 |
|---|---|
| SHC 1.41 | `3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a` |
| Extreme 1.41 | `55648e6b05d67d37a5773fe699bbb17a2d6ad4de1bb9dbded9a21caef82bd7fb` |

| Source | Literal `AOBScan` patterns | Unique in SHC | Unique in Extreme |
|---|---:|---:|---:|
| Legacy | 164 | 164 | 164 |
| AIC Tactics | 5 | 5 | 1 |
| AIC Loader | 1 | 1 | 1 |
| Map Extensions | 8 | 8 | 8 |
| Protocol before correction | 17 | 17 | 17 |
| Chat | 18 | 18 | 18 |

This is a literal-call inventory, not a complete semantic binding test. It does
not include dynamically constructed patterns or `utils.AOBExtract`, nor prove
ABI, hook composition, pool layout or gameplay. In particular, Protocol's new
fixed admission bridge is outside its 17 existing scans. Hex candidates include
valid instruction encodings and bit masks and are not counted as defects without
manual inspection.

## Framework/Legacy comparison and reuse decisions

| Capability | Actual existing owner inspected | Result and necessary action |
|---|---|---|
| Runtime discovery | Framework `core.lua:core.AOBScan` and `data/cache.lua:AOB.retrieve`; Legacy `port/ai_attacktarget.lua`, `ai_defense.lua`, `ai_attackwave.lua` | Use the existing cached scan, decode absolute operands and relative calls, retain displaced instructions. AIC's scan-equals-fixed-address assertions do not satisfy this. |
| Initialization/patching | Framework `main.lua` loads module environments before enabling them; Legacy `init.lua` initializes its selected features before applying them | Resolve original context before patching or use verified surviving context/owner exports when Legacy already owns a site. AIC currently follows Legacy detours using fixed SHC addresses. Do not create another patch manager. |
| AIC storage/application | Loader `addresses.lua:getAIStartAddress`, `init.lua:getAICValue`, `registerAICUpdateProvider`, `transactions.lua`; AIC `config/provider.lua` and `config/backend.lua` | Provider registration correctly uses Loader. AIC `state.lua`/`src/integrity.cpp` bypass Loader's discovered storage with a fixed native AIC array. Resolve the missing shared storage/identity binding through its owner. |
| Defense census | Legacy `ai_defense.lua` owns its wall counter and reset/count patches | AIC consumes that counter and chains its trampolines, but discovers them by fixed locations and SHC operands. Preserve Legacy source and this ownership while fixing discovery. |
| Recruitment/acquisition/group orders | Existing native functions, with faithful OpenSHC references and SHC component probes | Original functions are reused, but their casts/data roots are fixed. Pass verified named bindings to native consumers; verify Extreme capacities and layouts separately. |
| Saves and snapshots | Map `mapextensions/game.lua` owns native save hooks; required-state APIs in `init.lua` | AIC registers a required section, rather than adding another save hook. Existing Map literal scans match both images. Actual Extreme save/load with the corrected AIC state is unperformed. |
| MP transport/admission | Protocol `protocols/common.lua`, `game/hooks.lua`, `init.lua`, `admission/init.lua` | Transport is reused. The new fixed admission binding required correction in Protocol; see the correction below. Physical MP acceptance remains pending. |
| Local status display | Chat `init.lua:fireChatEvent` | Existing display API is reused; Chat 1.0.0 is unchanged. |
| Replay/input | Recorder `code/native.lua`, `code/engine-sites.lua` and required-state integration | Existing Recorder owner is retained. It has SHC/Extreme fixed profiles selected by PE-header identity, not AoB discovery. This is a separate unresolved owner integration issue, not evidence the whole bundle is AoB compliant. |
| Content enumeration | Protocol `admission/content.lua`, Recorder `code/replay-assets.lua` | Protocol uses framework VFS/SHA, but separately implements archive/directory traversal following Recorder's rule. Shared ownership and removal of avoidable duplication need further review before architectural acceptance. |

## AIC binding inventory requiring correction

| Production consumers | Bindings / layout assumptions | Required verification |
|---|---|---|
| `native.lua` | Scheduler/sortie call sites, recruitment opportunity, initial interval, moat owners, Legacy defense trampolines, per-player character and unit fields | Unique original/surviving contexts, decoded native calls and globals, complete overwritten instructions and Legacy ON/OFF combinations on both variants |
| `combat-native.lua` | Unit/building census, damage entry points, launch/wave/raid/target hooks and native branch continuations | Unique semantic contexts; generic damage prologues are insufficient; preserve original call count, ABI and hook interaction |
| `config/grace.lua` | Native timer instruction, immediate and game tick operand | Resolve the comparison rather than matching a fixed byte word/location; keep the original branch/RNG behavior |
| `config/backend.lua`, `state.lua`, `src/integrity.cpp` | Game tick, original AIC table, timer immediate | Consume named owner bindings consistently for configuration, identity, save admission and replay observations |
| `src/runtime.cpp` | Unit/building/tribe roots, recruitment and RNG calls, player fields, recruitment diagnostics/horses | Decode roots/functions; confirm native restriction, resource, capacity and RNG semantics for Extreme |
| `src/combat.cpp` | Unit/entity arrays, alliances, homes, clock, combat value/RNG calls, target lifecycle | Confirm Extreme unit/entity capacities, strides and ID validation; no SHC-only cutoff for Extreme units |
| `src/army.cpp`, `src/raids.cpp`, `army-state.lua` | Tribe allocator/membership/order/path functions, group slots/UIDs, player counters, building registry/costs, map rows | Confirm pool sizes, membership layout, group allocation, target lifetimes and serialization bounds against the native owners |
| `src/shc141_groups.cpp` and saved-state validators | Classic 1,250-tribe admission; classic 2,500-unit/other bounds elsewhere | Derive or explicitly verify capability/layout values before accepting Extreme. Changing address constants alone is inadequate. |

The inventory is grouped by responsibility. The JSON retains individual source
lines; each corrective binding still needs its own resolved evidence before
the table can be marked complete. Legitimate field offsets/enums are not process
addresses. Fixture hashes and package/MP/replay content digests remain useful;
they must not substitute for binding discovery or be removed indiscriminately.

## Corrections and remaining release gates

Protocol correction `bf720cd` replaces admission's fixed bindings with four
framework-discovered contexts, operand/relative-branch decoding and validation
against the existing transport singleton. Version increases from 1.1.0 to 1.1.1;
wire/API version stays 1. The actual Lua/FASM bridge passes 1,152 register, flags,
stack and RNG comparisons plus 12 negative resolution checks **per executable**.
Ten public tests pass. No callbacks scan. This is component evidence, not a
physical multiplayer test, and is not in the existing published bundle.

Packaging now derives Map, Protocol and Chat filenames from source manifests as
it already did for AIC/Loader; it rejects manifest/filename disagreement. This
prevents a future Protocol 1.1.1 payload being mislabeled 1.1.0. Three focused
archive tests pass. Previously published assets remain unchanged and retain their
recorded identities. The five ZIPs' root manifests, payloads and nine AIC GUI
locales passed the earlier package verification and SHC startup; that evidence
does not cover newly corrected code. The AIC repository also has no root license
file yet; repository licensing/source provenance must be settled before release.

Still required: AIC binding correction, Extreme ABI/layout and Legacy composition,
review of shared content enumeration, Recorder owner resolution, updated dependency
pins/new versioned ZIPs, Native/mixed-AI equivalence, actual parameter application
in sustained games, two-physical-peer MP, save/load, replay/seek/state restore,
and measured dense/no-path simulation performance. The existing bounded-work
design and small digest timings do not prove whole-game performance. CI/review
and normal merge remain open. No desktop was acquired for this read-only audit
or the component tests.
