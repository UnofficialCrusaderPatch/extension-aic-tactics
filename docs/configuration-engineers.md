# Engineer counting

AIC Tactics corrects the AI's engineer role count by default.
An engineer assigned to a defense, raid, sortie or attack group counts toward
that group's ordinary troop quota. Siege crews, oil workers and moat diggers
keep their separate native roles. This is a fix, so AIC authors do not set a
field for it. The module's **Count engineers in AI troop quotas** switch can
restore the original count for comparison; its default is ON.

If an earlier test AIC contained `CorrectEngineerRoleCounting`, remove that
field before loading it with this version.

Fixed Engineers 0.2.0 owns general crew cleanup, dismount safety and siege
attack commands. AIC Tactics does not duplicate those handlers. The two modules
can be used together.
