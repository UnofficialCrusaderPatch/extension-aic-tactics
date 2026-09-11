# AIC Tactics

Optional recruitment, attack and raid policies for AI personalities in UCP3.

**Under development. No gameplay-ready release is available.** Native policy
implementation and compatibility testing are not complete.

The package covers conditional recruitment and sortie probabilities, preparation
of one next attack wave during an offensive, opponent selection and retaliation,
and bounded split raids. Existing personalities must keep their established
behavior unless their author explicitly enables a new policy. Each subsystem
is independently opt-in.

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
