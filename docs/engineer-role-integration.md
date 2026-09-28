# Engineer quota integration record

This is an AI counting correction in AIC Tactics 0.0.14, dependent on the
safe-placement PR. It does not replace Fixed Engineers 0.2.0's general
crew, death or dismount handlers.

| Need | Existing owner inspected | Reuse decision |
| --- | --- | --- |
| AIC field validation and merge | AIC Loader 1.1.5 transactional `registerAdditionalAICValue` and `registerAICUpdateProvider`, already used by `config.provider` | Add one boolean to that provider and its native record. No parallel AIC parser or personality table. |
| Role counters and recruitment threshold | Original AI recount role dispatch, `aiRecruitUnits`, `assignUnitToATribe`; AIC Tactics `native-combat-bindings` unit census and `army.cpp` reserve exclusion | Reuse the original role dispatch/counters at its engineer skip. A generic combat census or a second full-unit scan would be too late or duplicate work. |
| Current group identity | Native `TribesState::addUnitToTribe` bitset, UID and lifecycle layout already resolved by `native-layout.lua` and consumed by `army.cpp` | Read the native group identity and reciprocal membership for this unit only. The existing reserve predicate applies only to reserved attack groups, so cannot classify every engineer. |
| Siege and oil duty | Original `assignRequiredIdleEngineersToNewTribe`, native construction-group behavior write, unit manning/duty fields | Exclude construction behavior `0x410`, mounted state 2 and nonzero oil/duty flag. Role 10 and moat accounting keep their native paths. |
| Runtime binding | UCP 3.0.7 cached `core.AOBScan` through `native-context.lua` | Resolve a unique role-recount context and the construction-duty store at module load; verify decoded counter operands and group layout. Refuse missing, ambiguous or overwritten sites. No production executable VA/hash table. |
| Save and replay | Existing Map Extensions required section and `state.lua` identity; `integrity.cpp` boundary digest | Add the per-AI word and module fallback to the existing identity/digest. No new mutable plan or separate save section. A save converted to a new map initializes its policy state. |

The native hook replaces the 10-byte engineer type comparison after role 10 is
handled. For non-engineers it enters the original role dispatch. For engineers
it enters that dispatch only when the individual AI's policy is enabled and
the engineer is alive, still in a valid current troop group, and free of
mounted, oil and construction duty. Otherwise it follows the original skip.
The hook preserves registers, flags and stack around the C++ predicate; the
original dispatch performs all counter writes.

Evidence: the binding and construction-duty signatures resolve uniquely in
six SHA-pinned normal/Extreme 1.41 executables (local, EFIGS and Polish).
The x86 DLL and focused component tests build and pass. This does not yet
establish sustained recruitment, casualty replacement, GUI, save/replay or
Fixed Engineers composition in a running game. Those remain acceptance gates.
