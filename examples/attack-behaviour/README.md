# Attack behaviour examples

These fragments are documentation and test fixtures, not presets. Merge one into
the `aic` object of a copied personality whose `AttUnitPatrol` names a troop
type and whose `AttUnitPatrolMax` is at least 16. Fields you leave out use the
module settings in Customizations.

| File | What a player should see |
| --- | --- |
| `two-patrols.json` | Both attack patrol groups leave camp during an assault; each picks its own steps. |
| `wing-patrol.json` | The two patrols advance side by side, about 14 tiles apart. |
| `pincer-patrol.json` | Near the target keep, the second patrol works the other side or a flank. |
| `native-patrol.json` | The game's own order: the second group waits at its staging point. |

Named combinations for an AI pack belong in that pack's own files.
