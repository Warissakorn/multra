# The remaining moving clamps at merges and red heads (M3.2.8a.1)

Written 2026-10-18 (D249). The owner chose the recommended package the same day, as three project
switches that are on by default (D250, §3), implemented by D251
([evidence](../evidence/merge-clamps.md#d251-the-package-implemented)). This
is the contract that ROADMAP M3.2.8a.1's gate requires before code. It covers the 41 safety clamps
left in the shipped M2.6 project, traced in the
[evidence](../evidence/merge-clamps.md). It names every cause, sets out options measured with
temporary prototypes, and drafts the rows for the recommended package. No clamp-free claim follows
from it, and no gate is closed.

## 1. What the clamps are

A safety clamp is reported when the move the following model wants passes the tick's hard cap.
Every remaining shipped clamp is in M2.6, with 41 in seeds 42–81. The other shipped projects have
0, except the lane-change lab's 1. There are four causes:

| Class | Count | Cause |
|---|---|---|
| M | 20 | A minor creeping at its give-way line meets a major vehicle entering the shared lane beside it. The minor's line is 1 m from the join (D50), but the standstill cap wants 2 m behind any leader. The entering vehicle's occupancy of the shared lane starts at the join, so the cap is 0 m |
| J1 | 9 | A major vehicle meets a minor that pulled out 0.6–3.4 s earlier and enters the shared lane just ahead of it. The reading is that the minor was creeping at its line, so it counted as `committed` and passed the headway and gap tests |
| J2 | 8 | A major vehicle catches a minor released long before, which is still accelerating from rest along its connector |
| R | 4 | A vehicle creeping at a red head that could stop within its maximum. GIVE_WAY_STOP §2 brakes only to give-way lines |

Two facts underlie M, J1 and J2:
1. **A merge's rule sees only the major connector.** It releases the minor once the major
   vehicle's rear leaves that connector.
2. **Car-following sees another stream only from the join on.** It sees the other stream's
   vehicle only when that vehicle's front enters the shared lane. Its rear then reads at the join,
   standing still, until the real rear crosses.

## 2. Options

Each option was prototyped, measured on seeds 42–81 of the four shipped projects, and reverted
([evidence](../evidence/merge-clamps.md#the-options-measured)). A, B, M and R act only on a tick
that would clamp today, so a run without such a tick keeps its bytes.

### B: an anticipating cap

On a tick that would clamp, the vehicle-leader cap credits the distance the leader's rear must
still travel along this vehicle's route this tick, even at the leader type's `maxDeceleration`:
- `credit = max(0, travel − hidden)`;
- `travel = vL·dt − ½·bL·dt²`, or `vL² / (2·bL)` when the leader stops within the tick;
- `hidden` is the part of the leader's body not yet on this vehicle's route.

The cap stays a guarantee. If the leader brakes at its maximum, the end gap is still at least the
standstill distance. A leader entering beside the vehicle earns no credit until its body is on the
route. This is D201 B5's "standstill cap treating a moving leader as standing".

- **Measured:** 41 → 35, mean delay within ±0.001 s. 5 M2.6 seeds and 1 lane-lab seed change.
- **Pins kept:** the pin and the TS rows are unchanged.

### M: a side entry at a held line

On a tick that would clamp, a vehicle held by a give-way line that is nearer than its vehicle
leader is capped by the line, not by that leader, when part of the leader's body is off its route.
The leader is entering beside it, and the rule, not car-following, keeps the two apart. GIVE_WAY_STOP's
stop then applies at the line.

- **What it accepts:** a vehicle stopped at its line stays inside the 2 m buffer of traffic
  entering at the join. That is already the geometry, because the line is 1 m from the join.
- **The row it changes:** it reverses
  `core.moving_merge_follower_still_reports_the_impossible_stop`. That is D105's row pinning the
  moving case as a clamp: the vehicle now brakes to its line.
- **Measured:** alone, 41 → 31, because some clamps come back a few ticks later once the leader is
  fully on the lane. With B, 41 → 21.

### Q: a stop at a red head, with `amberDeceleration`

GIVE_WAY_STOP §2's feasible stop also applies when a signal head's line is the binding cap (a red
head, or an amber one that holds the vehicle), but only for a behaviour that carries
`amberDeceleration`. The same scope keeps D147's amber rule out of bare
scenarios.

- **Why the scope:** without it (option R), the M0 crossing demo changes. The `trafficsim-cli 42`
  pin then differs, and 2 of the 5 TypeScript reference rows fail. Those rows must never be
  regenerated (AGENTS). With the scope, both stay unchanged.
- **Measured:** removes all 4 R clamps.

### The package: B + M + Q (recommended)

| | Today | B+M+Q |
|---|---|---|
| M2.6 clamps in 40 runs | 41 | 17 (J1 9, J2 4, M 3, 1 new) |
| M2.6 mean delay, s (±95 %) | 46.013 (±0.383) | 46.003 (±0.381) |
| Completed trips per run | 1980.80 | 1980.80 |
| Body overlaps (every span pair, every tick) | 0 | 0 |
| Four-leg, T-junction, `trafficsim-cli 42`, TS reference rows | — | byte-identical, 5/5 |
| Lane-change lab | 1 clamp | 1 clamp; 1 seed's output differs |
| Ir (prototype) | — | +0.6 % four-leg, +1.3 % lane lab, +0.6 % M2.6 |

### Measured and not recommended

- **A, a merge release margin.** The rule also waits while a vehicle on the shared lane is closer
  than the buffer plus the minor's largest move. 41 → 40, because the clamp comes from the *next*
  major's entry, not the one that has just left.
- **S, a 3 m derived setback** (instead of D50's 1 m). 41 → 35:
  - it clears M and J1 but adds 24 new clamps, because a minor past its line is now on a 3 m stretch
    the rule no longer checks;
  - it moves every M2.6 seed and one four-leg seed.
- **J, a held vehicle is not committed.** 41 → 49:
  - a creeping minor held at a line it has reached cannot stop in about 0 m, and clamps itself;
  - that comes from the line approach (D227, D232), not from commitment.

### Not prototyped: anticipation across a join

A vehicle approaching a join would see the other stream's vehicles before their fronts enter the
shared lane, ordered by their distance to the join. A held vehicle would not count as entering.
This is the only option aimed at J1 and J2, and at what is left after B+M+Q.

It is a new interaction. It would change trajectories at every join, including in clamp-free runs,
and would need its own contract and rows. It would be booked as M3.2.8a.1b if the owner wants it.

## 3. The owner's choice and its settings (D250)

The owner chose B + M + Q on 2026-10-18, each rule switchable at project level (level 1 of
[BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md#6-scope-project-wide-or-local-d216)), and on by default.

- **The keys.** An optional `clampRules` object on the project definition, with three booleans:
  - `leaderTravel` (B);
  - `sideEntryAtLine` (M);
  - `redLineStop` (Q, still only for a behaviour with `amberDeceleration`).
- **Absent means on.** A missing key, or a missing object, is `true`.
- **What the file holds.** The object is written only when some key is `false`, and then only the
  `false` keys. A project with all three on keeps its old schema.
- **Schema 45.** A file that writes the object is schema 45. An older schema carrying it is refused
  with `EDIT_UNSUPPORTED_FIELD` at its path. A non-boolean value, or an unknown key inside the
  object, is refused with `INVALID_CLAMP_RULE` at its path.
- **Bare M0 scenario files** take no key and always run with all three on. Under them the
  TypeScript reference rows and the CLI pin keep their bytes (§2, Q's scope).
- **Editor.** Run settings gains three checkboxes, labelled with the key names. Confirming the
  dialog is one Undo step with the other run settings.
- **Engine.** `Scenario::clampRules`, defaulting to all three on. With all three off, every run is
  byte-identical to today.
- **Implemented (D251):**
  - `tickCap` and `relievedCap` (`src/core/conflicts.hpp`, `src/core/caps.cpp`). Only a tick whose
    move passes the plain cap leaves the inline test.
  - The codec is in `src/project/clamp_rules.*`, the command is `changeClampRules`, and the
    checkboxes are in Run settings.
  - Measured with the defaults: M2.6 41 → 17, delay 46.013 → 46.010 s, and no overlaps. All off
    is byte-identical to today on all four projects.

## 4. Rows (BA158–BA167; implemented, D251)

Each row first asserts that its forcing worked (AGENTS). The rows use `test::straight()`-style
fixtures with a two-connector join.

| Row | Case | Asserts |
|---|---|---|
| BA158 | No tick clamps today: a free merge, a queue, the shipped four-leg and T-junction | Byte-identical, with the pin and TS reference rows 5/5 |
| BA159 | B: a follower 2.5 m behind a leader at 12 m/s, which today clamps | No clamp. The end gap is at least the standstill distance if the leader brakes at its maximum |
| BA160 | B: the same leader, standing | Unchanged: clamped as today |
| BA161 | B: a leader entering beside, with 4 m of its body still off the route | No credit, until its body is on the route |
| BA162 | M: D105's moving-merge fixture (minor at 0.9 m/s, 0.5 m from its line, a major entering) | Brakes to its line, with no clamp. `core.moving_merge_follower_still_reports_the_impossible_stop` keeps D105's clamp with `sideEntryAtLine` off |
| BA163 | M: the same minor not held by its rule (committed), or held with a leader whose whole body is on its route | Unchanged: clamped |
| BA164 | Q: a behaviour with `amberDeceleration`, creeping at a red head, stop within the maximum | Brakes to the line, with no clamp; a stop beyond the maximum goes as committed (AMBER §1), the same with Q off |
| BA165 | Q: the same without `amberDeceleration` | Unchanged (the M0 crossing demo keeps its pin) |
| BA166 | Each switch off in turn, then all three | Each off restores today's tick for its rule; all off is byte-identical to today on M2.6 |
| BA167 | Codec and editor | Schema 45 round-trips; all on writes no key and keeps the old schema; 44 carrying the key refused; a non-boolean or unknown key refused at its path; the Run settings checkboxes write the keys as one Undo step |

## 5. What stays out

- J1 and J2 after the package (13 of the 17). They need anticipation across a join, or a change to
  how a vehicle stops at its line (D227, D232).
- Source-insertion clearance (M3.2.8a.1a, D108) and the lane-change lab's one clamp, which is not
  traced.
- Gap time and headway values: they are study inputs. No fidelity claim (rule 4).
