# Initial defense recruitment

`RecruitInitialDefenseMonths` requires `RecruitPolicy: WeightedRoles`. It accepts
whole months from 0 to 30. Unset, it follows the game's initial timer: 6 months,
or the value of Legacy `ai_recruitstate_initialtimer`. Zero disables the grace
period. While the match is younger than this duration and defense is below its
existing quota, raid and main-army recruitment are ineligible. Defense and
sorties still compete using the first matching condition row or the base
strength row.

Grace never gives a zero-weight role a positive weight. If no positive-weight
role can recruit, this opportunity hires nobody and makes no role draw. A filled
defense quota releases the restriction early; casualties can reapply it until
the duration expires. Recruitment intervals, spending gates, unit lists and
quotas remain authoritative. No additional opportunities are created.

## Legacy timer and Native personalities

The original timer is a global immediate, not an AIC field:
`updateAIStrengthState` compares the match tick with 4800 at `0x4D34AB`.
Legacy `ai_recruitstate_initialtimer` replaces the immediate at `0x4D34B1` with
its slider value times 800. Legacy owns that immediate; AIC Tactics never writes
it. Native personalities use it through the original chooser.

For a WeightedRoles personality, an authored `RecruitInitialDefenseMonths` takes
precedence over the global timer; unset, the personality uses the timer's
current value. This is an intentional eligibility rule, not a claim that
WeightedRoles reproduces the native chooser.

Before activation the runtime checks the comparison's surrounding instructions
and branch; the immediate itself may hold any value.

## Time, updates and saves

One simulation month is 800 ticks: the original scheduler wraps every 200 ticks
(`0x45CC19`); its calendar call advances one quarter, and four quarters advance
the month (`0x4566EA–0x456728`). The match clock increments at `0x45CE58`.
Grace uses this clock, not wall time or a scenario's displayed calendar, which
can be paused separately. Its end is exclusive: 6 months blocks at tick 4799
and ends at tick 4800. Loading a save keeps elapsed match time; it does not
restart the grace period.

Getters return authored months. Partial updates preserve omitted values;
whole-personality reset clears the value (the global timer applies) and restores Native mode. Changes to an active policy
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
bounds and atomic commit/rollback. `check_grace_native.py` runs unchanged Legacy Lua for
all 31 durations, checks that AIC Tactics resolves and accepts its bytes and executes
9,600 ticks of original calendar instructions with its pause predicate stubbed
false. These are component checks; in-game continuation, Native command/RNG
baseline equality, two-peer multiplayer and replay acceptance remain required.
