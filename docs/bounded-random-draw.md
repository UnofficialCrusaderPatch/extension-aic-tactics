# Bounded draws from the existing native stream

`drawBounded` maps an existing synchronized 15-bit sample stream to an integer
ticket in `[0, bound)`, for bounds 1 through 100. It owns no generator, seed,
table, index or persistent state. Recruitment callers must first establish
eligibility and the positive weight total. Native/disabled policies bypass it.
A single possible outcome consumes no sample.

For larger bounds, accept only samples below
`floor(32768 / bound) * bound`, then take the remainder. Every ticket has the
same number of accepted inputs. Rejected tail samples consume the existing
stream and retry, without retrying role eligibility or choosing a fallback
role. This removes remainder bias; it does not make the game's pseudorandom
generator independent or cryptographically random.

The callback must return the owner's current sample and advance that owner
exactly once. The production binding is not implemented by this component.
It must verify the executable, original generator/table integrity, simulation
thread ownership and save/replay admission before use. Replacing the generator
or admitting arbitrary restored table contents invalidates the bound below.

## Work bound and reference evidence

Reference: SHC 1.41 executable SHA-256
`3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a`.
`RNG::populate` at `0x46A760` fills 20,000 signed shorts from the native CRT
generator. Its routine at `0x5816FB` uses unsigned 32-bit recurrence
`state = state * 214013 + 2531011`, output `(state >> 16) & 32767`.
`RNG::nextRandomNumber2` at `0x46A7D0` reads that table and wraps its index.

The lowest acceptance limit for supported bounds is 32670, at bound 99.
The component test enumerates all 6,422,528 low-31-bit states whose output is
at least that value. Bit 31 cannot affect any later masked output: changing
it adds `2^31` modulo `2^32` at every step of the odd-multiplier recurrence.
The longest consecutive rejected run is three. Joining an arbitrary generated
table's rejected suffix and prefix at wrap can therefore produce at most six
consecutive rejections. Even if the saved current sample precedes an unrelated
valid table index, it can add only one rejection. Eight calls suffice under
these assumptions. The test also enumerates all 32,768 input samples for every
bound 2 through 100 and checks equal accepted counts.

The implementation always stops after eight calls, including for malformed
callbacks. An out-of-range sample or exhaustion returns an explicit error and
no ticket; consumed samples are reported. There is no biased fallback or
implicit second recruitment attempt. A production caller must treat failure
as an unsupported/corrupt source before issuing game commands, not silently
continue the match with a substituted role. No gameplay, multiplayer,
save/load or recorder acceptance is implied by these component tests.
