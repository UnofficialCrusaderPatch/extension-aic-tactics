# Recruitment selection component

This C++98 component implements the bounded decision calculation for issue #2.
It is not connected to the game and is not an installable module. The native
eligibility, recruitment, synchronized RNG and lifecycle adapters remain required.
Component checks do not establish native, multiplayer, save/load, replay or
performance acceptance.

The native adapter must enter at the established recruitment opportunity and
provide the verified strength class, one coherent snapshot of condition facts,
and a four-role eligibility mask. A mask bit means the normal resource,
peasant, building, scenario, quota and pool checks allow a recruit for that role.
Do not obtain this information by attempting acquisition or temporarily spending
resources. The facts' native metrics and units remain investigation gates;
the bit names do not prove an implementation of threat or equipment surplus.

The base row combines existing authored defense/raid/attack values with the
additional sortie value. It is a decision input, not another mutable copy of
the original controls. Every explicit row must sum to 100. Up to eight ordered
conditions may select another complete row; the first matching row wins.
Conditions combine required/forbidden facts and an optional strength class.
The base row applies otherwise. All supplied rows are validated, including rows
after the first match, so invalid dormant rows cannot hide behind an earlier rule.

After selection, ineligible roles have zero effective weight. The remaining
weights sum to `totalWeight`; zero means skip without requesting randomness.
Otherwise the synchronized native adapter must supply one unbiased ticket in
`[0, totalWeight)`. The mapping is defense, raid, attack, sortie. There are no
rerolls, zero-weight fallbacks or recruitment calls in this component. It does
not specify a modulo operation on raw native RNG values; unbiased bounded draw
and native RNG phase integration must be verified separately.

Native mode immediately returns `UseNativeRecruitment`, without interpreting
weights, conditions, strength or eligibility. The caller must then run the
original recruitment path with its original RNG/command behavior. Authoring
preflight must still reject explicitly requested new values that Native cannot
honor. This component is not a substitute for that loader validation.

Work is fixed: at most eight four-weight condition validations, four eligibility
checks and two four-role loops to validate/map a ticket. Storage is fixed stack
and caller-owned structures; there is no allocation, RNG state or logging.
This establishes code bounds, not a measured full-match performance result.
