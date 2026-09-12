# Initial defense recruitment

`RecruitInitialDefenseMonths` requires `RecruitPolicy: WeightedRoles`. It accepts
whole months from 0 to 30 and defaults to 6. Zero disables the grace period.
While the match is younger than this duration and defense is below its existing
quota, raid and main-army recruitment are ineligible. Defense and sorties still
compete using the first matching condition row or the base strength row.

Grace never gives a zero-weight role a positive weight. If no positive-weight
role can recruit, this opportunity hires nobody and makes no role draw. A filled
defense quota releases the restriction early; casualties can reapply it until
the duration expires. Recruitment intervals, spending gates, unit lists and
quotas remain authoritative. No additional opportunities are created.

## Legacy migration and Native personalities

The original timer is a global immediate, not an AIC field:
`updateAIStrengthState` compares the match tick with 4800 at `0x4D34AB`.
Legacy `ai_recruitstate_initialtimer` replaces the immediate at `0x4D34B1` with
its slider value times 800. It changes the native role chooser, whose other
branches, RNG calls and defense-full fallback remain in place.

When using WeightedRoles, turn **OFF**
`ucp2-legacy.ai_recruitstate_initialtimer.enabled`. If it was enabled, copy its
slider value to the module option `aic-tactics.nativeInitialDefenseMonths` to
retain that duration for Native personalities. This option accepts 0–30,
defaults to the original 6, and changes only the original immediate. Native
personalities continue through the original chooser. Existing AIC values are
never rewritten. Legacy source is unchanged.

For each WeightedRoles personality, choose `RecruitInitialDefenseMonths`
independently. It takes precedence over the global timer for that personality's
effective recruitment. This is an intentional eligibility rule, not a claim
that WeightedRoles reproduces the native chooser.

The runtime rejects an enabled Legacy timer even when its value is 6 and its
bytes happen to match the original. It also verifies the comparison and branch
before activation. The default module with only Native personalities makes no
timer patch and leaves the existing Legacy setting available. Resolver-level
conditional requirements still need integration before release.

## Time, updates and saves

One simulation month is 800 ticks: the original scheduler wraps every 200 ticks
(`0x45CC19`); its calendar call advances one quarter, and four quarters advance
the month (`0x4566EA–0x456728`). The match clock increments at `0x45CE58`.
Grace uses this clock, not wall time or a scenario's displayed calendar, which
can be paused separately. Its end is exclusive: 6 months blocks at tick 4799
and ends at tick 4800. Loading a save keeps elapsed match time; it does not
restart the grace period.

Getters return authored months. Partial updates preserve omitted values;
whole-personality reset restores 6 and Native mode. Changes to an active policy
require a fresh process. Returning to Native retains authored extension values
inactive, as for the other recruitment settings.

Recruitment schema 3 uses a 288-byte compiled record, including the per-AI grace
duration in ticks. Saved identity version 3 includes these records, native AIC
records, interval migration and the native timer immediate. Different settings
or earlier development state formats are rejected. No separate running timer
is needed; the existing saved match clock owns elapsed time.

## Verification scope

Production runtime tests cover the start/end boundary, unsigned clock, defense
completion, zero defense weight and allowed sorties. Loader/backend tests cover
bounds and atomic commit/rollback. `check_grace_native.py` compares the actual
replacement bytes with unchanged Legacy Lua for all 31 durations and executes
9,600 ticks of original calendar instructions with its pause predicate stubbed
false. These are component checks; in-game continuation, Native command/RNG
baseline equality, two-peer multiplayer and replay acceptance remain required.
