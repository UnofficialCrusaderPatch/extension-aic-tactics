# Map initialization and saved policy state

AIC Tactics 0.0.9 requires Map Extensions 1.1.4. New maps initialize empty policy
state using Map's native file context, even if the map contains an editor tick.
The earlier zero-tick test could reject a fresh match as an incompatible save.

Saves and Recorder snapshots still require matching policy state when any
personality opts in. Missing context cannot admit an empty save, including at
tick zero. Native personalities can load older saves without policy state.
Present manifests and payloads retain their format/content/configuration checks.
Map uses the existing UCP fatal logger to stop a rejected native load, because
stock RPS catches ordinary Lua callback errors and otherwise resumes native code.

This uses Map's existing read callback and filename binding. AIC adds no scan,
hook, filename parser or framework dependency. The policy state ABI remains 7;
content fingerprints still identify the exact module build. Start a new match
when upgrading an earlier preview with a different fingerprint.

Portable checks cover nonzero map ticks, zero/nonzero old saves, missing context,
validation before writes and exact policy round trips. Native image discovery
and an installed-game cold save/load are separate acceptance checks.
