# Compatibility and migration

The table distinguishes code paths from acceptance evidence. The integrated
preview has SHC startup and limited gameplay evidence at recorded revisions;
full behavior acceptance remains incomplete. The published 0.0.2 preview does
not support Extreme. Source 0.0.3 declares both game families and now resolves
native hooks/functions/layouts through UCP. See [current binding evidence](shared-native-bindings.md)
and the historical [integration audit](native-integration-audit.md). Component
checks and fixed work limits do not establish gameplay compatibility or performance.

| Original control | Confirmed native meaning | Status / replacement | Activation and precedence | Defaults / conversion | Get, set, reset | Legacy requirement | Evidence / remaining acceptance |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `RecruitProbDef/Raid/Attack` for each strength | Three-role probabilities; sorties have their own native handling | Conditionally superseded by four-role `WeightedRoles` rows and first matching `RecruitConditions` | Only `RecruitPolicy: WeightedRoles`; zero weights remain disabled after eligibility filtering | Native by default. Author all four weights totaling 100; no automatic conversion | Original authored values retained; Native restores their original use | `ai_recruitinterval.enabled` OFF; `ai_recruitstate_initialtimer.enabled` OFF; `ai_defense.enabled` ON | Earlier native recruitment checks; integrated Native/mixed command/RNG comparison pending |
| Recruitment interval fields | Per-character, per-strength cadence | Reused unchanged | WeightedRoles and reserve recruitment run at the existing opportunity | No extra opportunity or free hire. Global `legacyRecruitInterval` optionally reproduces Legacy interval 1 for Native AIs | Unchanged | Interval option OFF when WeightedRoles owns cadence; replacement default OFF | Earlier 11,520 register/stack/flags checks; integrated game comparison pending |
| Sortie types/minima | Troop types and capacity for local sorties | Reused unchanged | Positive sortie weight under WeightedRoles; Native keeps original handling | New sortie weights default 0 | Unchanged | Shared defense/recruitment requirements above | Earlier actual sortie recruitment; integrated census/game acceptance pending |
| `DefTotal`, defense limits, `DefUnit1..8`, `DefDiggingUnit/Max` | Defense target, unit list and separate moat-digger quota | Reused; `DefRecruitComposition: PreserveSlots` conditionally preserves roster shares | WeightedRoles only; diggers stay outside ordinary `DefTotal` | Native composition by default; repeated entries increase share | Unchanged; composition override is separate | `ai_defense.enabled` ON; reuse its wall census | Earlier composition/moat native checks; integrated running-game acceptance pending |
| Initial recruitment timer | Native global 4,800 ticks / six months unless Legacy changes it | Conditionally superseded by `RecruitInitialDefenseMonths` for WeightedRoles; `nativeInitialDefenseMonths` retains the global Native setting | Defense grace blocks raid/main-army recruitment only while defense is incomplete; does not enable a zero-weight role | Per-AI and Native defaults six months; 0–30, zero disables grace. Copy former Legacy months before disabling it | Old AIC values unchanged; new values reset independently | `ai_recruitstate_initialtimer.enabled` OFF when the new gate is used | Earlier grace instruction checks; full mixed acceptance pending |
| `TargetChoice` | Original native opponent selector, including historical Any/Balanced behavior | Conditionally superseded by explicit `AttackTargetPolicy`; reused for Inherit and LastAggressor fallback | Target lock takes precedence during an active attack; UntilDefeated takes precedence across attacks | Inherit default. Global `nativeTargetPolicy` migrates Legacy Nearest/Richest/Weakest for Native/fallback selection | Original value retained; resetting the new policy restores its use | `ai_attacktarget.enabled` OFF for extended target ownership; copy its choice to `nativeTargetPolicy` | Original selector/Legacy sites inspected; integrated decisions and replacement behavior pending |
| Native attack commitment | Native selector can change targets during an attack; Legacy assault-switch adds stability | Conditionally superseded by `AttackTargetCommitment` | New target policies and DuringAttack resolve Default to PerAttack; Inherit + Native preparation + Default keep the existing path | UntilDefeated is explicit; default promotion does not overwrite authored Default | Original TargetChoice getter unchanged | `ai_assaultswitch.enabled` retained/unrestricted: original Native selector still owns it; active extended commitment bypasses that selector | Shared-site source/assembly audit; runtime composition acceptance pending |
| `AttForceBase`, random size, growth, caps, subrole/main troop settings | Existing wave writer computes next requirement at committed launch; main roster is also native fallback | Reused by one `DuringAttack` reserve | Reserve never increments wave or invokes its writer/RNG; active/reserve counts separate | Native preparation default. Random PerAttack uses the undiscounted requirement before drawing its target | Original authored values unchanged | `ai_attacklimit`, `ai_addattack` retained/unrestricted; use their actual writer result | Earlier 6,144 native/unchanged-Legacy writer checks; integrated handovers and capacity/performance pending |
| Wall/breach dispatch | Attack tribes' wall allocation and follow-through after breach | Unchanged | Native attack routine remains the caller | Preserve the selected baseline | Unchanged | `ai_attackwave` retained/unrestricted; no patch at its dispatch owner | Source/site composition inspected; wall/breach gameplay pending |
| `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` | Total raiding force and composition | Reused by split groups | A new raid policy divides that force, never multiplies its quota | Native raid policy, one group, minimum four by default | Original values and roster retained | No new conflict with size/growth settings | Native group/roster/candidate owners inspected; actual split raids pending |
| `RaidRetargetDelay` | Existing AI-update counter for retargeting | Reused | Saved pending flags spread the same interval's work across groups; invalid targets permit bounded replacement | No second interval field | Unchanged | No extra requirement | Native counter inspected; max-group/no-path benchmark pending |

No deprecated aliases are introduced and no original parameter is removed.
`AttackActivation`, `ProvocationRules`, `RaidFocus`, `RaidRiskTolerance` and
`RaidEnemyScope` add behavior instead of redefining old enum values.

GUI required values belong in the opting-in AI pack's existing `config-sparse`
metadata (see `examples/ai-pack`). Runtime preflight covers direct launches too.
Installing AIC Tactics alone must not turn off a global Legacy fix. Retained Legacy
source is unchanged. Every profile comparison must pin the executable, AI files,
module versions/content and Legacy settings.

Apply related fields together through the transactional AIC Loader API. Returning
to Native requires a fresh process: `RecruitPolicy: Native`,
`AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`,
`AttackPreparation: Native`, `AttackActivation: Immediate`, `RaidTargetPolicy: Native`.
Previously authored new values remain stored but inactive; do not re-author their
mode-specific fields in the same Native update. Whole-personality reset clears them.

When any personality opts in, the match lock covers all AIC updates, including
Native neighbours. Pure Native single-player sessions keep the original loader lifecycle.
This prevents mixed matches from bypassing the lock through an untouched AI.

Native-only saves do not declare AIC Tactics as required. Its optional Native
state can be ignored when the module is absent or changed. New policy saves require
Map Extensions 1.1.0's required-provider checks and exact
package/AIC identities. Old Native saves without policy state may initialize only
Native behavior. Existing policy saves/recordings need their original artifacts;
there is no implicit upgrade, downgrade or live army conversion. A plain game or an
older Map Extensions reader cannot acquire new validation retroactively; loading
new-policy saves through those readers is unsupported. Protocol 1.1.0's admission
prerequisite compares full content/configuration identities before the native
host Start path. Once that exchange begins, all AIC edits require a fresh process,
including Native-only multiplayer. Physical-peer acceptance remains outstanding.
