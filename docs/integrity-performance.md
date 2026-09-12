# Replay boundary observation timing

SHC 1.41 private instruction host, MSVC2005 SP1 `/O2`, GamerGrill,
12 September 2026. Three separate host runs, each with 1,001 paired samples of
32 calls, rotating operation order and 100 warmup copies. Nonzero synthetic raid
grids prevent reliance on an all-zero state. The timer is QueryPerformanceCounter.
No game was launched; the machine was shared with other background work.

All values below are microseconds per call, without baseline subtraction:

| Operation | Run | Median | p95 | p99 | Maximum batch average |
| --- | --- | ---: | ---: | ---: | ---: |
| Boundary copy | 1 | 3.256 | 4.772 | 8.525 | 61.284 |
| Boundary copy | 2 | 2.463 | 3.444 | 4.931 | 19.238 |
| Boundary copy | 3 | 3.109 | 3.809 | 7.388 | 30.928 |
| Live state digest | 1 | 47.044 | 56.231 | 77.853 | 219.522 |
| Live state digest | 2 | 46.688 | 52.988 | 70.463 | 102.978 |
| Live state digest | 3 | 46.819 | 58.269 | 81.584 | 153.041 |

The empty volatile loop's median was 0.003125 microseconds and p99 0.006250
in all three runs. The variation and maxima show scheduling noise; percentiles
are of batch averages, not individual game ticks. This cache-warm component
measurement excludes Lua callbacks, state-owner admission, recorder I/O, game
simulation, pathfinding and additional living armies. It proves neither a tick
regression percentage nor full-match performance acceptance.

The observation uses a fixed 128 KiB buffer, copies only explicit state words,
and does not allocate, read files, call RNG or drive simulation decisions. Hashing
still occurs at existing checkpoints and on completion/copy. A compiled size
assertion protects the buffer; native checks compare its digest against live
state and verify that later raid changes cannot alter the retained observation.

Reproduce with `tests/run_shc141_probe.py --benchmark-integrity` after the
MSVC2005 runtime test build, supplying the pinned private reference executable,
test executable, compiler and output directory. Local JSON evidence records
source, compiler, reference and test executable hashes under
`build/integrity-timing-1`, `-2` and `-3`.
