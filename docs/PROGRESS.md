# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-17 — Behaviour-key exposure (D217)

The owner's wish, and NEXT's item: every driving-behaviour key the engine already accepts can now be
edited in the program ([evidence](evidence/behaviour-key-exposure.md)).
- **Behaviour editor:** perception (5), lateral (one box for its three keys) and the curve key are
  off-until-ticked rows for either model. Before, they were only kept.
- **Behaviour Lab panel:** the same rows for both behaviours; *Apply to project* writes them back.
  The D95 keys and kerb bias stay out, because the circuit clears lane changes.
- **Tests:** both UI rows failed first. A Lab row shows that each group runs on the circuit with no
  clamp, also when ticked mid-run. The curve acts on curved Connectors only (Urban left turns, Oval
  joins), and a tooltip says so.
- No engine, codec or data change.

`check` 166/166, Linux only.

## 2026-10-17 — Keep-to-kerb slice 1 and the settings scope (D215, D216)

The owner asked to continue, and to design which settings are project-wide and which are local.
- **D215** implements OVERTAKING ([evidence](evidence/kerb-bias.md)).
  - What changed: `kerbLaneBias` (schema 35); `towardKerb` compiled from lane indices; returns need
    `T − β`, passing and undertaking `T + β`; an optional row in the behaviour editor.
  - Tests: six rows failed first, and both mutations are caught. Shipped outputs are byte-identical.
  - A53 measured with the bias still fails: repeats are 3.3–7.2 % up to β 2, and above that nobody
    passes. No D95 value ships.
- **D216** adds BEHAVIOUR_PARAMETERS §6, with six scope levels: project, catalog default, vehicle
  type, driver behaviour (local by road through the existing behaviour assignment), network object,
  and per vehicle. The more local level wins.

`check` 166/166, Linux only.

## 2026-10-17 — B19 decided on the owner's delegation (D214, docs only)

The owner answered B19 with "ช่วยตัดสินใจ" ("help decide"). D213's tiers stand, except that the
50 km/h lateral reference moves to fixed: no study needs it, and Vissim fixes it too. Tier B is ranked
by its effect on Thai study outputs: kerb bias, commitment deceleration, merge deceleration, speed
limits, speed distributions, Stop seconds, lane preference by type, driver-factor spread, then
red-light running ([BEHAVIOUR_PARAMETERS](reference/BEHAVIOUR_PARAMETERS.md) §3). No code changed.

## 2026-10-17 — Behaviour configurability and the keep-to-kerb contract (D213)

NEXT's item, M3.4 overtaking with the driving side. The owner chose an asymmetric bias, with
undertaking allowed but costlier and lane preference by type later. They then widened the question:
driving behaviour should be configurable for other countries, with judgement on what should be
settable.
- [BEHAVIOUR_PARAMETERS](reference/BEHAVIOUR_PARAMETERS.md) puts every value that shapes behaviour
  into a tier: settable now (A), should become an optional key (B, ten items), or kept fixed (C).
  Regions are catalog data. This is a proposal (OWNER_SITTING B19).
- [OVERTAKING](reference/OVERTAKING.md) defines `kerbLaneBias`. A return needs `g ≥ T − β`; passing
  and undertaking need `g ≥ T + β`. The kerb side comes from the lane index. Rows BA64–BA71 are
  written; no engine code changed.
- The inventory found that D212 had left a real bug: the behaviour editor's W74 save dropped
  `curveLateralAcceleration`. The UI test failed first and the one-line fix is in. `check` passes,
  Linux only.

## 2026-10-17 — M3.6 slice 1: curve speed (D212)

NEXT's item: SPEED_CONTROL §2–§5 (D211). Rows BA56–BA63 came first; BA57, BA58 and BA62 failed
([evidence](evidence/curve-speed.md)).
- `curveLateralAcceleration` is read, validated and written at schema 34. Nine codec tests that used
  34 as the "future" schema now use 35.
- `Segment::curvature` is compiled from the Connector polyline's minimum circumradius, in a helper
  shared with the tight-radius advisory.
- `speedTarget` replaces `desiredSpeed` in the tick's cap.

BA58's first wording was stricter than the contract and was restated. §4 now measures `d` from where
the front will be after the tick, because the present front lagged by a tick. Switched off, every
output is byte-identical. Switched on, four-leg delay rises by 3–13 s per seed, which is the intended
effect, and `speedTarget` costs 4.6 % Ir.

Measuring found that D210 had cost the switched-off run 1.7 % Ir by moving `closestVehicle` out of
line. It is inline again, with the covered scan kept out of line: 894 → 886 M against D209's 879 M.
`check` 165/165, Linux only.

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

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
