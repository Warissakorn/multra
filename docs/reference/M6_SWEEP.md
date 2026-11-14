# M6_SWEEP — the pre-registered sensitivity sweep, phase 1 (D545)

The owner's choice ([m6-calibration-options](../evidence/m6-calibration-options.md), D544) is no
calibration on M6, plus a sweep that measures what each lever does. **A sweep figure decides
nothing, has no band of its own, and is never adopted.** It is shown beside the first runs. The
candidate values are fixed here, before any sweep run, and are never changed after a figure.
Written before code; rows SW1–SW4 fail first.

## 1. Candidates (phase 1: data-only levers)

| Name | Change to every compiled behaviour | Everything else |
|---|---|---|
| `base` | none | `default.json` |
| `B` | `standstillDistance` = 0.95 m (the jam spacing [AkcBes02] Table 2 implies: 6.9 − 4.5 − 1.45 m of `√v` gap at the creep) | as `base` |
| `C` | `followingTime` = 1.0 s | as `base` |

Phase 2 covers D and E, the engine levers. It needs runtime switches, off by default, each with
its own contract. It is not here.

## 2. The runs

`tools/m6_candidates.hpp` holds `candidate(name)` and `apply(Scenario&, const Candidate&)`. `apply`
patches every `Scenario::behaviours` entry after `compileDocument`. It refuses a behaviour with
`w74` set, because the levers belong to the prototype.

Three tools take an optional last argument, `--candidate base|B|C`:

| Tool | Modes | Figures (as the first runs) |
|---|---|---|
| `multra-m6-s1-run` | — | S1-a, S1-f and S1-d (D535) |
| `multra-m6-s2-run` | `ab` only | S2-b and S2-a (D539) |
| `multra-m6-u-run` | `run` only | U-a and U-c (D542) |

- **The candidate is recorded.** It appears as `candidate` in `summary.json` and `metadata.json`.
- **Without the flag**, every output is byte-identical to today's. The first runs therefore stand
  as `base`.
- **Seeds:** 42–81.
- **S2-c is not swept.** The S2-b and S2-a pair shows the capacity side of the levers.
- **Verdicts** are computed with the first runs' bands and shown as "would pass" or "would miss".
  They are not M6 verdicts.

## 3. Acceptance rows

| Row | Statement | Group |
|---|---|---|
| SW1 | `apply` with `B` sets `standstillDistance` 0.95 and with `C` sets `followingTime` 1.0 on every behaviour, leaving the other fields; `base` changes nothing | `m6sweep` |
| SW2 | `apply` refuses a scenario with a w74 behaviour; `candidate("X")` is refused | `m6sweep` |
| SW3 | `junction(200)` cut to 600 s, seed 42: with `B` the follow-up split's mean `standDistance` is smaller than with `base` (the forcing: the lever reaches the run) | `m6sweep` |
| SW4 | The S1 and U tools refuse an unknown candidate, and the S2 tool refuses a candidate with mode `c` | `cli-m6-sweep-refused` |

## 4. Phase 2: the engine levers D and E (D546)

Written before code, after phase 1's figures ([evidence](../evidence/m6-sweep-phase1.md)) and
before any phase-2 figure. Rows SW5–SW10 fail first.

**The switches.** Each lever is a runtime-only flag on `DriverBehaviour`, like D258's `stopFloor`:
- never in a file and never validated;
- false by default, so every shipped output stays byte-identical;
- set only by `m6sweep::apply`;
- prototype law only (w74 is refused, §2).

| Name | Flag | Definition |
|---|---|---|
| `D` | `standingTarget` | In `followingAcceleration`, behind a leader slower than 0.1 m/s, the desired gap is `standstillDistance` alone, without the `√v` term. This covers a queued vehicle, a red head and the Stop hold's virtual leader. `desiredGap()` and `stopLineReach` are unchanged. |
| `E` | `stopArrival` | In `refreshStops` only (not `atStopLine`, which parking also uses), a Stop is made below 0.5 m/s, not 0.1 m/s, within the unchanged `stopLineReach`. |
| `DE` | both | both |

**E amends D544's wording.** D544 described E as "the target is the line, at the line within
0.5 m". Without D, the `√v` term keeps a car about 1–2.5 m short of the line at 0.1–0.5 m/s, so
it would never reach 0.5 m. The two parts are therefore split: E is the arrival test, D is the gap
target, and DE combines them.

**Runs** (seeds 42–81, as §2):

| Candidate | S1 | S2-ab | U |
|---|---|---|---|
| `D` | run | run | run |
| `E` | not run: no Stop, identical to `base` | not run | run |
| `DE` | not run | not run | run |

**Estimates, pre-registered** from D544 and not revised:

| Candidate | Estimate |
|---|---|
| `D` | jam spacing about 6.5 m; S1 about 2,070 veh/h; S2-b/a misses grow; U `t_f` about 10 s |
| `E` | U `t_f` about 9 s |
| `DE` | none |

Phase 1 found D544's U arithmetic too optimistic for both B and C.

**Rows amended after the first test run, before any sweep figure (as SU4 was):**
- **SW7** first asked for the gap to stay at or above `standstillDistance` in the bare law. With D the
  law is underdamped and dips about 0.08 m below it. In the engine, the hard cap (`leaderCap`, the
  leader's gap less the standstill gap) holds the standstill gap. The row now asks that the vehicle
  does not touch the obstacle.
- **SW9** first asked `E` for more pairs than base. On seed 42's 600 s, `E` alone leaves the pairs
  unchanged: without D, a car enters the Stop reach already below 0.1 m/s, so the 0.5 m/s test
  never applies. E is forced through `DE` against `D` instead.
- No definition changed.

| Row | Statement | Group |
|---|---|---|
| SW5 | `D`, `E` and `DE` set only their flags, and change no number on any behaviour. Every flag is false on a compiled `junction(200)` and after `base` | `m6sweep` |
| SW6 | The prototype law with `D`: behind a leader at 0.05 m/s, the acceleration equals base's with `additiveSafetyDistance` and `multiplicativeSafetyDistance` both 0. Behind a leader at 0.2 m/s, it equals base's | `m6sweep` |
| SW7 | From 10 m/s, 50 m behind a standing obstacle, with `followingAcceleration` and `integrate` at 0.1 s: with `D` the speed reaches 0 within 30 s without touching the obstacle (gap above 0). Base is still moving (speed above 0) at that tick | `m6sweep` |
| SW8 | `stopMade`, the arrival test `refreshStops` uses: a vehicle at 0.3 m/s, 1 m before a Stop line, has made the Stop with `E` and not without it. At 0.05 m/s it has made it either way | `m6sweep` |
| SW9 | `junction(200)` cut to 600 s, seed 42: `D` gives a smaller mean `standDistance` than base, and `DE` gives a shorter mean follow-up headway than `D` (the forcing) | `m6sweep` |
| SW10 | The S2 tool refuses `--candidate E` with mode `c` (SW4's test, now with `E`) | `cli-m6-sweep-refused-mode` |

Return to [the reference index](README.md).
