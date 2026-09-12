# AIC Tactics

Optional recruitment, attack and raid policies for AI personalities in UCP3.

**Under development. No gameplay-ready release is available.** Native policy
implementation and compatibility testing are not complete.

The package covers conditional recruitment and sortie probabilities, preparation
of one next attack wave during an offensive, opponent selection and retaliation,
and bounded split raids. Existing personalities must keep their established
behavior unless their author explicitly enables a new policy. Each subsystem
is independently opt-in.

New opponent policies default to PerAttack commitment; UntilDefeated is an
explicit choice. Inherit with Default commitment preserves Native behavior,
including existing Legacy target stability. This decision is tracked in
[targeting issue #6](https://github.com/UnofficialCrusaderPatch/extension-aic-tactics/issues/6).

One native module owns the four policies. AIC Loader retains field registration
and update handling; AI Swapper retains personality selection and application.
Faithful native reconstruction stays with OpenSHC, and save/replay integration
uses the existing subsystem owners. Legacy source remains unchanged.

Prerequisites and coordination:

- [AIC Loader update contract](https://github.com/UnofficialCrusaderPatch/extension-aicloader/issues/17)
- [Exclusive field ownership](https://github.com/UnofficialCrusaderPatch/extension-aicloader/pull/18)
- [Atomic personality updates](https://github.com/UnofficialCrusaderPatch/extension-aicloader/pull/19)
- [Existing OpenSHC AIC reconstruction](https://github.com/sourcehold/OpenSHC/pull/165)
- [Replay state integration](https://github.com/Corax34/ucp_recorder/issues/47)

GUI descriptions live in `locale/description-<language>.md`, using the launcher's
language identifiers: `ch`, `de`, `en`, `es`, `fa`, `fr`, `hu`, `ru`, `tr`.
`ch` is UCP's Chinese identifier. Keep every description aligned with the actual
available features and release status. In-game language and encoding are separate
from the launcher's selected GUI language.

GUI descriptions are short parameter references: purpose, values, defaults and
dependencies. Full examples are in separate author guides:
[English](docs/configuration-en.md), [German](docs/configuration-de.md),
[French](docs/configuration-fr.md), [Spanish](docs/configuration-es.md),
[Hungarian](docs/configuration-hu.md), [Turkish](docs/configuration-tr.md),
[Russian](docs/configuration-ru.md), [Chinese](docs/configuration-ch.md),
[Persian](docs/configuration-fa.md).

The descriptions and guides document the configuration contract. Gameplay
integration for the described parameters remains incomplete; no installable
release or gameplay acceptance is claimed. Development status belongs here and
in issue/PR records, not in the GUI parameter descriptions.

For GUI review, the **Description preview ZIP** workflow attaches an artifact to
the PR. Follow its [three-sentence test instructions](tools/description-preview/TESTING.md).
The preview uses a separate plugin name and an empty configuration; it supplies
no gameplay code and cannot satisfy an AI pack's `aic-tactics` dependency.
