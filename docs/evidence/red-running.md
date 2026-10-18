# B-9: red-light running share — 2026-10-18

Branch `claude/m3.4-lane-change-duration-slice1`, on `8f35684` (D237's contract) plus this change (D238).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [RED_RUNNING](../reference/RED_RUNNING.md).

## Rows (`tests/red_running_tests.cpp`, ctest `redrunning`)

The engine rows use `test::straight()`'s road with one head 100 m along it. The program is red for
20 s from t = 0, then green for 40 s, unless a row says otherwise. "Before the change" means the
keys and the `red_running` module were present, but the step did not call them.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA138 | No keys; 10 m/s, 15 m short | Held at the line; no event | passes (it pins today) |
| BA139 | Share 1, time 2 s, the same vehicle | Crosses; exactly one `RedRunningEvent` naming the head | fails |
| BA140 | Red began 5 s before t = 0 (offset 15 s); then a runner 60 m short at red onset | Both held; no event. `redElapsed` reads 5 s | passes (a hold guard; the window mutation fails it) |
| BA141 | Share 1, time 30 s, standing 1 m short | Stays for 15 s | passes (the moving-test mutation fails it) |
| BA142 | The draw | 2000 ids at share 0.3 give 0.3 ± 0.03; the same id gives the same answer; split red phases across the cycle boundary count as one red; an all-red program never began | written with the module |
| BA143 | 1800 veh/h for 60 s with share 0.5 | Every vehicle's scheduled time, desired speed and driver factor equal to no keys | passes (it pins the stream) |
| BA144 | Head on the second road; a runner behaviour assigned to that road, then to the first road only | Runs only when the head's road carries the keys | fails |
| BA145 | Core validation | Each range and each lone key refused at its path on both models; the event JSON kind is `red-running` | written after the code; proved by mutation |
| BA145 | Codec | Schema 44 round-trips; 43 refused; a lone key refused with `INCOMPLETE_RED_RUNNING`; 0, 1.5 and a string refused; removing the keys returns the file to its old schema | written after the code; proved by mutation |
| BA145 | Outputs | On the four-leg with share 1 for 3 s, two seeds of 300 s: `redRunning` appears in batch JSON, per seed, in the CSV and in the movement report. Without the keys none of them carries it | written after the code; proved by mutation |

**Mutations,** each removed in turn:

| Mutation | Caught by |
|---|---|
| No exemption in the step | BA139, BA144, BA145 outputs |
| No window | BA140 |
| No moving test | BA141 |
| No share test | BA142, BA143 |
| `redElapsed` ignores earlier red phases | BA142 |
| No event | BA139, BA145 outputs |
| No parse | BA145 codec |
| No writer, share or time | BA145 codec, once the row checked that both keys are present before reading them. In the first sweep the test binary aborted on the missing key, and the script did not count an abort |
| No schema-44 term, no refusal below 44, no library key | BA145 codec |
| No validation, prototype branch | BA145 core and codec |
| No validation, W74 branch | BA145 core |
| No pair check | BA145 core and codec |
| No report count, no batch estimate | BA145 outputs |
| Event written with the wrong kind | BA145 core |
| No dialog row | `behaviour-library-ui` (`redRunningShareSet`) |

**Dialog (`behaviour-library-ui`).** The either-model block also checks that both rows are off by
default. It ticks them, sets 0.05 and 2 s, and expects them on the edited copy only. The edited W74
file now writes schema 44.

`check` **177/177**: 176 tests plus the new `redrunning`.

The first `check` stopped at the architecture rule. `src/eval` may include only `core/types.hpp`,
and the movement report had called `carriesRedRunning` from `core/red_running.hpp`. The report now
tests the behaviours itself, with the same logic. The mutation above was made on the earlier form.

## Shipped runs

**Byte-identical** (no shipped behaviour carries the keys):
- `trafficsim-cli 42` against D210's baseline;
- the `--seeds 42–51` output (without `buildCommit`) of all four shipped projects against D225's
  baseline.

**With the keys:** scratchpad copies of the data directory with `redRunningShare: 0.05` on the
shared `wiedemann-inspired-prototype`, at three window lengths. Seeds 42–81, 900 s each:

| Project | Measure | No keys | 1 s | 2 s | 3 s |
|---|---|---|---|---|---|
| `four-leg-signalised` | Runs in 40 runs | — | 2 | 8 | 24 |
| | Mean delay, s (95 % half-width) | 46.00 (±0.55) | 45.98 | 45.82 | 45.76 |
| | Safety clamps in 40 runs | 0 | 0 | 0 | 0 |
| `m2.6-study-template` | Runs in 40 runs | — | 7 | 30 | 75 |
| | Mean delay, s (95 % half-width) | 46.01 (±0.38) | 46.00 | 45.90 | 45.86 |
| | Safety clamps in 40 runs | 41 | 41 | 43 | 44 |

- **Runs are rare at a 5 % share.** Only a vehicle still moving within the first seconds of red
  qualifies. Most vehicles that meet red onset are already in a standing queue or far enough away
  to stop. The count rises steeply with the window, about 3–4 times from 2 s to 3 s, because
  arrivals further upstream start to qualify.
- **Delay falls slightly** as runners skip a stop. The change stays inside the half-width.
- **Clamps.** None on the four-leg. M2.6 rises by up to 3 in 40 runs at a 3 s window. These could be
  runners caught short of the line when the window ends and held again. They are not traced here.
- **No crossing traffic reacts.** The shipped signalised projects have no conflict areas (§1).

## Performance (callgrind Ir, Release `-g`, seed 42, no keys)

Both builds were made in the same build directory.

| Project | `8f35684` | This change |
|---|---|---|
| `four-leg-signalised` | 890.44 M | 886.52 M (−0.44 %) |
| `lane-change-lab` | 1250.18 M | 1249.73 M (−0.04 %) |

Without the keys the step adds an empty vector and one branch per red head. The fall is code
layout.

## Not covered here

- Yielding to crossing traffic, a collision model, running from standstill, and a share by type or
  time of day (RED_RUNNING §5).
- Calibrating a share or window against field counts.
- Tracing M2.6's added clamps.
