# AI pack setup

Merge a JSON fragment into the `aic` object of a copied personality. Existing
recruitment intervals, troop lists, quotas and wave settings still supply those
values; the fragments do not overwrite them. `retaliating-reserve.json` waits for
provocation and prepares one reserve; `random-reserve.json` keeps one randomly
selected opponent until invalid and lets split raids choose random buildings.

The `ai-pack` directory is a manifest/configuration template for an AI author,
not an installed AI pack: add the existing AIC Loader file option and the actual
personality file using the same layout as your pack. Its module dependency declares
the extension capability so older loaders cannot silently drop the new parameters.
Its required Legacy values cover a pack using all new subsystems. Omit unrelated
requirements for a pack using only one subsystem, following the compatibility matrix.

Before changing a working profile, copy enabled Legacy recruitment timing, initial
timer and target-choice values to AIC Tactics' `legacyRecruitInterval`,
`nativeInitialDefenseMonths` and `nativeTargetPolicy`. This preserves those global
behaviors for Native personalities. The template does not guess the user's former
values. Keep existing assault-switch, wave-dispatch, growth and cap options.

Installing the module alone changes no global Legacy options: a personality
without new recruitment, target or raid settings keeps those native policies.
The default-ON engineer-counting and safe-placement corrections are separate
module switches. Required Legacy values belong to an AI pack that opts into the
matching tactics; runtime preflight also rejects conflicting direct launches.
