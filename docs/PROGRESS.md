# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-16 — M3.6 curve speed contract (D211, docs only)

NEXT's item after D210: M3.6, which the review ranks as the cheapest large effect (turners cross
junctions at full desired speed). The owner chose:
- curve speed from Connector geometry first, with areas later;
- a driver-behaviour key;
- delay kept against the desired speed.

[SPEED_CONTROL](reference/SPEED_CONTROL.md) fixes the key `curveLateralAcceleration` (schema 34,
absent = today), the radius (the minimum polyline circumradius, the advisory's formula) and the bound
`sqrt(a·R)`. The bound is anticipated at `comfortableDeceleration` through D191's cap. Rows BA56–BA63
are written. No code changed; the review's rule is contract → rows → code, never combined.

## 2026-10-16 — A straddler follows the other lane's leaders (D210)

NEXT's item. LATERAL §6 counts leaders on the follower's carriageway, but `closestVehicle` and
`perceivedLeaders` walked only the straddler's own route, so a body over the line drove into a slower
car on the lane it covered. Rows BA55 failed first ([evidence](evidence/straddler-leaders.md)).
- `closestVehicle` moved to `src/core/leaders.cpp`, so `simulation.cpp` shrank to 477 lines.
- `followerFilter` lists the lanes the body covers at its front segment.
- Both searches also scan those lanes, walking each through its only successor.

BA54's step-back row started 1 m behind a slower car on the covered lane, which a following straddler
can no longer reach; it now starts 4 m behind. Switched off, every output is byte-identical. Switched
on, there are no clamps and mean delay moves by ≤ 0.29 s. Cost: +6.0 % / +4.8 % Ir against
`5847cb9`, inside §11. `check` 164/164, Linux only.

## 2026-10-16 — Switched-on engine round (D209)

NEXT's item, owed since D206. Callgrind on the switched-on lab: 24 % in `occupiedSpans` (11 % its
lateral copies), 10.5 % in `moveLaterally`, most of it rebuilding every body for the few that move.
Three output-preserving changes ([evidence](evidence/lateral-perf.md)):
- `moveLaterally` builds only bodies near movers;
- `appendLateralSpans` skips its neighbour loop inside a precomputed, conservative clear band;
- `lateralCounts` is inline.

Switched on, the lab and four-leg now cost +3.5 % and +3.0 % Ir against `5847cb9`, inside LATERAL
§11's 10 %; lab wall time −22 % against `347fba9`. Switched off, outputs are unchanged and the run is
6 % cheaper. Every shipped and switched-on output is byte-identical. Linux only.

## 2026-10-16 — Mid-change gap loss: waiting at the line (D208)

NEXT's item. D206 left two clamps where a faster car passed a slow changer mid-move. The owner chose
**wait at the line**: an outward lateral move that would cover another lane must pass the start's
safety tests there (nothing alongside, the leader ahead and the follower behind accept it). A body
already over the line steps back. `src/core/lateral_motion.cpp` (`coversSafely`); rows BA54 failed
first ([evidence](evidence/lane-change-gap.md)).

The first version, checking only the leader per segment, made things worse: 29 clamps instead of 2.
Each cause was probed and fixed in turn: waiting over the line, a follower just behind, and a follower
on the previous segment. The test now compares every vehicle in its own route's distances. Both
switched-on projects run with 0 clamps and unchanged completed trips; switched-off runs and the CLI
pin are byte-identical. `check` 163/163, Linux only.

## 2026-10-16 — Swapping pair at shared dead ends: the rear one yields (D207)

NEXT's item, found by D206's runs. Two stubs side by side, each needing the other's lane, were each
alongside the other's target place, so neither ever changed and both approaches jammed
(lane-change-lab seed 44, switch off too). The owner chose the rule from three: **the rear one
yields** (level: the higher id). It holds back behind the front one's target place at comfortable
deceleration, and the existing rules then let both change. `src/core/lanes.cpp` (`yieldMutual` in
`courtesyHolds`); `CourtesyHold::cap` carries each hold's braking limit, so `simulation.cpp` keeps 500
lines. Rows in `tests/lane_swap_tests.cpp` failed first ([evidence](evidence/lane-swap.md)).

Every lab stall is gone with the switch off and on. Shipped four-leg and M2.6 outputs move slightly
(≤ 0.2 s mean delay), because such pairs form there too; the CLI pin is byte-identical. `check`
163/163, Linux only.

## 2026-10-16 — M3.4 slice 1: lane-change duration (D206)

NEXT's item. `src/core/lanes.*`, `lateral_motion.cpp`, one line of `simulation.cpp`. With the lateral
switch on, an accepted lane change now sets `lateralTarget`; the body moves at `maxLateralSpeed`,
the route remaps at the crossing and the target clears on arrival. Rows BA48–BA53 failed first
([evidence](evidence/lane-change-duration.md)).

Switched-on product runs exposed two flaws in the start rules as D204 wrote them; both are fixed:
- the span-window rule counted the body's length, so a changer standing near its dead end never
  started (four-leg seed 42 jammed);
- a change under way was invisible to other starts, so two changers converged on one lane 5 cm
  apart. It now holds its target place for the whole change.

Still open, recorded in NEXT: a swap deadlock at shared dead ends that predates this slice
(lane-change-lab seed 44, switch off too); two mid-change clamps in ten lab seeds; and the
switched-on cost (+33 % lab, +17 % four-leg, Release), owed an engine round before any shipped
project carries lateral keys. Switched off: `check` 162/162, CLI pin byte-identical, benchmark
unchanged. Linux only.

## 2026-10-16 — Main's Windows-only UI reds (D205)

NEXT's first item. The windows-desktop job had failed four UI tests since PR #170, on Windows only. Both
causes came from the CI log ([evidence](evidence/windows-ui-red.md)):
- **`canvas-lod-ui`**: with a rotated view, `viewScale()` came out an ulp below exactly 3 px per lane, and
  the threshold comparison had no margin. `laneMarkingsReadable` now allows a relative 1e-9.
- **`properties-layout-ui`** (Thai only): Windows Qt cannot break Thai words, so the 123 px word in
  `editorMedianGap` widened the Link page's column by 3 px and no other page's. The column is now one
  `FormColumn` shared by every form, widened to such a word when it is under a quarter wider.

- **Round 2** (the branch's first Windows run): the threshold was confirmed, and a hidden form kept
  the old column when the width changed back with the appearance fold closed. Hidden forms are now laid
  out at once.

All three now reproduce on Linux. Each row failed before its fix and fails again with the fix reverted.
Checked on Linux offscreen: `check` 161/161, CLI pin byte-identical to main. Windows is confirmed only
by this branch's CI run.

## 2026-10-16 — M3.4 lane-change duration contract (D204, docs only)

The owner asked for recommendations on the contract's three open choices and took all three:
- a change starts only if it can finish before the next conflict area on both lanes (an extension of
  the existing "no change inside an area" rule), so no zone has to hold a straddler;
- the D97 slide stays for switched-off runs, which still change instantaneously;
- no new key: `maxLateralSpeed` sets the duration.
New [LANE_CHANGE_DURATION](reference/LANE_CHANGE_DURATION.md) with rows BA48–BA53; also found that a
change must cross before its span's `fromEnd`, since the crossing needs a covering span (LATERAL §7).
No code changed; `check` was run for the docs.

## 2026-10-16 — M3.7 slice 4: lateral display (D203)

NEXT's next item after slice 3b. `src/editor/canvas_run.cpp`, `src/core/types.hpp`, `lateral_motion.cpp`.
- **Drawing:** a run body sits at lane centre + `lateral`, the front shifted along the left normal of
  its heading. Left of travel is the engine's convention, so left- and right-hand networks need no
  case. `lateral = 0` draws exactly as before.
- **Found while reading:** a crossing made by lateral motion records a `LaneChangeTrace`, and the D97
  slide would have drawn the body about a lane away from where the engine keeps it. The entry is now
  flagged `lateral` (display only, no tick reads it) and such crossings are not slid.
- `types.hpp` stays at 500 lines (the field shares its line). The M3.7.0 contract's four slices are
  all in; the gate stays open (ROADMAP).

Checked on Linux offscreen: `ctest --preset desktop` 161/161, `check` green, CLI pin unchanged and the
Release CLI output byte-identical to main. No Windows or high-DPI run.

## 2026-10-16 — M3.7 slice 3b: zone occupancy by span (D202)

The owner chose the head of the engine queue. `src/core/conflicts.*`, `lateral.*`, `lateral_types.hpp`.
- **What a straddler's other-lane span does to a zone** (switch on only): inside a major area it
  blocks, on the chain's first segment it is anticipated by headway/gapTime (one `blockMajor` helper
  shared with route positions), inside a minor area it holds, and in phase 2 it sweeps a major area.
  `LateralIndex::zoneSides` gives each segment's zone sides once per run.
- **Not done, on purpose:** the straddler is not held at the other lane's zone. That needs the zone
  mapped into its own route's distances; no product writer sets targets until M3.4, so it waits for
  M3.4's evidence rather than being built blind (D202's failure condition names it).
- `zoneHold` code is unchanged; `summarizeZones` keeps a 4-argument overload that builds spans only
  with the switch on, so the ten lane-based test callers did not change. `simulation.cpp` stays at
  500 lines: the step passes its existing spans on the same lines.
- Rows: BA47 (6 tests, `lateralzone`), each with its forcing asserted and a mutation that fails it.

Checked on Linux headless: `ctest` 105/105, `check` green, CLI pin unchanged and the Release CLI
output byte-identical to main. Release timings in the [evidence](evidence/lateral-zones.md): switch
off inside noise; switched-on four-leg +7 % on a noisy container, below D164's 10 %. No desktop or
Windows run.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
