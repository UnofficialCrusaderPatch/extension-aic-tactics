# Moat diggers under WeightedRoles

Existing `DefDiggingUnit` and `DefDiggingUnitMax` remain the troop type and quota
for unfinished home moats. WeightedRoles includes these hires in the **Defense**
weight. Zero Defense weight prevents them; they do not bypass a condition row
or spend a separate recruitment opportunity.

When both an ordinary defender and a moat digger can be recruited, selecting
Defense gives each task an equal chance through the existing bounded native RNG
adapter. When only one task is eligible, it is used without an extra unit-choice
draw. Unavailable equipment, a full quota, no pending moat, invalid ownership or
no free native group capacity removes the digger candidate before selection.

Diggers remain in their original native role 5 and tribe slot 10, assigned by
`aiAddUnitToMoatDiggerTribe` at `0x4CC840`. The native moat order/cleanup owner
continues their work. They do not fill `DefUnit1..8` composition shares, `DefTotal`
or wall slots. `DefenseIncomplete` and initial defense grace continue to refer
to the regular defender quota. Authors who want digging to continue after that
quota is filled need positive Defense weight in the applicable row.

`EquipmentSurplus` also allows for the missing diggers' equipment while their
moat work remains pending. It releases that allowance when no unfinished moat
remains. Native personalities keep the original recruitment path, including
the original moat override and its RNG behavior. The WeightedRoles task choice
is an intentional opt-in change; it is not a reconstruction of that override.

## Ownership, bounds and checks

`countUnfinishedMoatTilesForPlayer` at `0x500180` reads the existing registered
moat entries and checks completion flags. It visits 15,999 entries. This module
calls it at most once per recruitment opportunity, only if Defense can use it
or an equipment condition needs its deficit, and only below the digger quota.
It does not scan once per candidate or maintain a second terrain index.
Candidate selection adds at most one acquisition query per attempt.

The adapter verifies the two native entry signatures. Before assignment it
validates the existing group ID, UID, owner, state, capacity and the player's
native allocation partition. Successful diggers decrement the opportunity's
remaining quota without incrementing regular-defender or composition counts.
The original saved group and unit state owns membership; no new save section,
pool size or permanent runtime counter is introduced.

Component tests execute the actual native moat registry query and assignment.
They cover completed/foreign moats, quota exhaustion and reopening, foreign and
stale groups, full own-partition capacity, zero Defense weight, equipment
reservation, and all eight players' group allocation, UID, membership and reuse.
These checks do not replace in-game digging/cleanup, saved continuation,
two-peer MP, replay or measured-performance acceptance.
