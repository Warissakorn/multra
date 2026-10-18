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
- **Clamps.** None on the four-leg. M2.6 rises by up to 3 in 40 runs at a 3 s window. D241 traced
  them (below): none is a red-head hold.
- **No crossing traffic reacts.** The shipped signalised projects have no conflict areas (§1).

## D241: the added M2.6 clamps

Traced on 2026-10-18, on `e7ae358`, with no code change. A temporary stderr probe in the step was
never committed; it is the method of D232. It printed:
- every phase-1 clamp, with the hold that bound it;
- every phase-2 cap and every run;
- a tick-by-tick trace of chosen vehicles.

The runs were `--project m2.6-study-template --seeds N --threads 1`, with the keys absent and with
D238's 2 s and 3 s copies.

**Where they are.** Only two seeds change, and every other seed's count is the same:

| Seed | No keys | 2 s | 3 s |
|---|---|---|---|
| 51 | 3 | 5 | 5 |
| 73 | 0 | 0 | 1 |

The probe's counts equal D238's batch JSON in all six runs. No phase-2 cap clamped. The three
shipped clamps at seed 51 are unchanged in time, vehicle and numbers.

**Every added clamp is bound by a vehicle leader, not by a red head.** D238's guess, a runner caught
short of the line when the window ends, does not happen directly.

| Seed | Window | t (s) | Vehicle | Leader (gap, its speed) | Speed | Needed | Class |
|---|---|---|---|---|---|---|---|
| 51 | 2 s, 3 s | 3513.2 | 1945 (runner) | 1903 (0.55 m, 12.0 m/s) | 2.25 m/s | — (inside the 2 m standstill gap) | Merge after a run |
| 51 | 2 s, 3 s | 3514.8 | 1914 | 1945 (2.60 m, 2.75 m/s) | 9.12 m/s | 69.6 m/s² | Behind the merge clamp |
| 73 | 3 s | 3183.1 | 1792 | 1788, a runner (2.16 m, 0.06 m/s) | 1.86 m/s | 10.6 m/s² | Behind a runner caught at the window's end |

"Needed" is `v² / (2 · allowed distance)`. The car's maximum is 8 m/s².

**Seed 51: a runner meets the green flow at a merge.**
- The shipped default behaviour has `amberDeceleration: 3`. Vehicle 1945 met amber at `head-69`
  (South, lane 2) about 70 m out at 15.4 m/s. It could stop at 3 m/s², so it braked at 2.2 m/s².
- At red onset (t ≈ 3507.1) it was 32 m short at 9.0 m/s. It was a runner, inside the window and
  moving, so the head released it. It accelerated at 2.5 m/s² and crossed at t = 3509.7, with red
  2.6 s old, at 14.1 m/s.
- On `connector-55` (South through) the derived merge rule into `link-44` held it for East-right
  traffic on `connector-54`. That traffic had just started from `head-67` on green: 1903, then 1914.
  1945 braked at 8 m/s² to the rule line.
- Once 1903's rear cleared the point and 1914 was outside the headway and gap time, the rule let
  1945 go, 0.55 m behind 1903. The standstill cap (`gap − standstillGap`) allowed 0 against a leader
  moving at 12 m/s, so 1945 clamped.
- 1914, at 9.1 m/s, then found 1945 at 2.6 m and clamped.
- These are M3.2.8a.1's remaining merge clamps behind a shared leader's standstill buffer (D105,
  D201 B5). The shipped clamps at 673.3 s and 1692.5 s have the same signature: zero allowed
  distance behind a leader 1–2 m ahead at 11.5 m/s. The run only brings a vehicle to the merge
  against the conflicting green flow (RED_RUNNING §1: no new interaction).

**Seed 51 at 2 s: a run that is not counted.**
- With the 2 s window the trajectory is the same. When the window ends (t ≈ 3509.1), 1945 is 9.35 m
  short at 13.4 m/s. Stopping needs 9.6 m/s², above the 8 m/s² maximum, so AMBER §1's commitment lets
  it through.
- The tick that crosses the line is no longer exempted by this rule, so no `RedRunningEvent` is
  written. The run counts 2 events at seed 51, not 3.
- This matches RED_RUNNING §3 as written ("a vehicle exempted by this rule"), but a study counting
  violations misses a run that the rule started.

**Seed 73 at 3 s: a runner caught at the window's end.**
- 1788, a runner at 12.1 m/s, was released at red onset (t ≈ 3177.1) 51 m from `head-65` (East).
- With a 3 s window it kept its speed until the window ended. It was then 15.5 m short: stopping
  needed 4.7 m/s², which is not committed, so the head held it again.
- The prototype braked at 6 m/s² and stopped it 2.3 m short. That needs no clamp.
- Its follower 1792 had been 12.5 m (1.0 s) behind at the same speed. It could not stop behind a
  leader stopping this hard: at t = 3183.1 it needed 10.6 m/s² and clamped.
- With a 2 s window, 1788 was held 28.8 m short at about 5 m/s², and 1792 stopped without a clamp.
- This is the case D238 guessed, one vehicle removed: the runner held again stops within its
  maximum, but its follower cannot.

**What it means.** None of the three clamps is a flaw in the red rule's own hold. They are:
- one existing merge clamp class, reached through a run;
- one follower clamp behind an abrupt second stop.

Three behaviours of RED_RUNNING §3 show up here. Each would need the owner's choice before it
changed:
1. A runner that was already braking for amber at the head may start again at red onset (1945).
2. A run whose crossing tick falls after the window, when commitment lets the vehicle through, is
   not counted.
3. A runner may be held again late in the window, close enough to the line to force its follower
   into a clamp.

## Performance (callgrind Ir, Release `-g`, seed 42, no keys)

Both builds were made in the same build directory.

| Project | `8f35684` | This change |
|---|---|---|
| `four-leg-signalised` | 890.44 M | 886.52 M (−0.44 %) |
| `lane-change-lab` | 1250.18 M | 1249.73 M (−0.04 %) |

Without the keys the step adds an empty vector and one branch per red head. The fall is code
layout.

## D247: the owner's answers to D241 (2026-10-18)

On `d4b52f3` (D246's contract) plus this change, the same host and build. Contract:
[RED_RUNNING](../reference/RED_RUNNING.md) §3 as revised by D246.

**Rows** (`tests/red_running_tests.cpp`, ctest `redrunning`, 16 passing). The fixture's behaviour
has no `amberDeceleration`, so the car's `comfortableDeceleration` (2 m/s²) is its gentle value;
BA152 sets `amberDeceleration` 3 to match the shipped default.

| Row | Case | Asserts |
|---|---|---|
| BA139 (amended) | 10 m/s, 15 m short at red onset: 3.3 m/s² against 2 | It asserts the gentle value first; crosses; one event |
| BA140 | Its 60 m case | Now held by the gentle test from red onset, not by the window |
| BA152 | 9 m/s, 32 m short, `amberDeceleration` 3 (D241's 1945) | Held, no event; the same runner at 10 m/s and 15 m crosses |
| BA153 | No `amberDeceleration`; 10 m/s at 20 m and 30 m | 2.5 m/s² against 2: crosses; 1.67: held; at 20 m with `amberDeceleration` 3: held |
| BA154 | A 0.5 s window, 10 m/s, 15 m short | First asserts it is past the window, short of the line, moving and still let through; then it crosses with one event after 0.5 s |
| BA155 | A hand-set release, 2 m/s, behind a leader standing at 99 m | First asserts it stays let through while moving; then standing (about 16 s in, the prototype's slow approach); the next tick ends the release; with the leader removed it stays short of the line until green, with no event |
| BA156 | Red 0.5 s, then green | First asserts the release; it is empty from green on; it crosses with no event. Without the keys `redReleases` is never set over a 90 s run with arrivals; with them two runs are identical |
| BA157 | A second red head 30 m further on | First asserts it is let through at the first and not at the second; one event, naming the first; held at the second until green |

**Mutations,** each applied in turn to the new code; every one fails at least one row:

| Mutation | Caught by |
|---|---|
| No gentle-stop test | BA140, BA152, BA153, BA155, BA157 |
| The decision does not hold | BA154, BA155 |
| A standing vehicle stays let through | BA155 |
| Last tick's pairs kept when this tick has none | BA155, BA156 |
| A fixed 3 m/s² instead of the type's value | BA153 |
| One release per vehicle, not per head | BA157 |

**Byte-identical** (no shipped behaviour carries the keys): `trafficsim-cli 42` and the
`--seeds 42–51` output (without `buildCommit`) of all four shipped projects, against `ef9ccb2`.

**With the keys,** using D238's method: scratchpad copies of the data directory with
`redRunningShare: 0.05` on the shared `wiedemann-inspired-prototype`, seeds 42–81, 900 s each. The
`ef9ccb2` column reproduces D238's numbers exactly.

| Project | Measure | `ef9ccb2` 1 s / 2 s / 3 s | This change 1 s / 2 s / 3 s |
|---|---|---|---|
| `four-leg-signalised` | Runs in 40 runs | 2 / 8 / 24 | 5 / 5 / 5 |
| | Mean delay, s | 45.98 / 45.82 / 45.76 | 45.98 / 45.98 / 45.98 (±0.54) |
| | Safety clamps | 0 / 0 / 0 | 0 / 0 / 0 |
| `m2.6-study-template` | Runs in 40 runs | 7 / 30 / 75 | 15 / 15 / 15 |
| | Mean delay, s | 46.00 / 45.90 / 45.86 | 46.00 / 46.00 / 46.00 (±0.38) |
| | Safety clamps | 41 / 43 / 44 | 41 / 41 / 41 (the no-key count) |

- **D241's clamps are gone.** Seed 51 is back to its 3 shipped clamps, and seed 73 is back to 0.
- **The window no longer changes anything here.** The 1 s, 2 s and 3 s outputs are identical, apart from
  `buildCommit`. A temporary stderr probe in `letThrough` (D232's method, never committed) printed every
  new release on M2.6 at 3 s. There were 15 releases, one per counted run. 14 were at red onset and one
  0.3 s after it.
  - A vehicle sees a red head from any distance, so one that meets red later has been stopping
    gently since amber.
  - Only a vehicle that went on amber (it could not stop at `amberDeceleration`) is still short of
    the line and unable to stop gently at red onset.
  - So the runs read as amber-goers caught by red. The probe did not record the amber decision, so
    this is a reading, not a measurement. `redRunningTime` matters only when a vehicle
    first becomes unable to stop gently after red begins.
- **Runs also count committed crossings.** In 6 of the 15, stopping needed more than the car's
  8 m/s² maximum, so AMBER §1's commitment would have let them through anyway. The rule is tested
  first, so a runner's committed crossing is counted. A non-runner's is not. D238 behaved the same.
- **Delay** stays inside its half-width.

**Performance** (callgrind Ir, Release `-g`, seed 42, no keys; the two builds are in separate
directories, a baseline worktree and this tree, so a small difference is layout):

| Project | `ef9ccb2` | This change |
|---|---|---|
| `four-leg-signalised` | 887.20 M | 888.81 M (+0.18 %) |
| `lane-change-lab` | 1252.00 M | 1253.26 M (+0.10 %) |

Without the keys the step adds one empty vector moved into the state; the head loop is unchanged.

## Not covered here

- Yielding to crossing traffic, a collision model, running from standstill, and a share by type or
  time of day (RED_RUNNING §5).
- Calibrating a share or window against field counts.
