# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-15

- `main` is `65c627a`; `native.yml` (Linux and Windows) last passed on `c2bacd1` (run 529,
  2026-10-14), which pins `trafficsim-cli 42` = 45 completed / 31.75870154816726 s / 1 clamp.
- Branch `claude/eloquent-carson-qgorzf` adds the cited `w74` preset (D176) and the
  [driving-behaviour review](audits/DRIVING_BEHAVIOUR_REVIEW.md) with milestones M3.5–M3.9
  (D177; motorcycles are core scope), the owner's answers (D178) and the perception
  contract (D179); not yet merged.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — pick one, as the user asks

Without a choice from the owner, take item 1. One system per session; contract and acceptance
rows first for engine work. Items 1–1c follow the review's order (§5), which the owner asked for
on 2026-10-15 (D177) and accepted (D178).

1. **M3.7.0 — the two-dimensional occupancy contract, docs only** (review §4.2, D177/D178).
   Write `docs/reference/LATERAL.md` in [PERCEPTION](reference/PERCEPTION.md)'s shape. Core today
   knows no width or lateral adjacency: `Segment` is `{id, length, next}`
   (`src/core/types.hpp:13`), `OccupiedSpan` is longitudinal only (`src/core/routes.hpp:9`), the
   type's `width` is display only, and lanes relate only through compiled lane-change spans. The
   contract must define:
   - what the compiler adds: lane width, lateral offset of each lane-level segment within its
     Link's cross-section, adjacency, and `drivingSide` (D178's keep-left needs it)
   - the vehicle's lateral offset as `SimState` data
   - spans with lateral extent, and how leader search, `around`'s "alongside" and conflict tests
     work on rectangles
   - between-lanes occupancy, which M3.4's lane-change duration uses next
   - the unset case: every vehicle centred and byte-identical (BA18)
   - a per-tick performance budget against `trafficsim-engine-benchmark`

   Also: rows BA39+ and a decision. Physical non-overlap stays separate from perception, as D179
   does. No engine code.
   - 1b. The M3.5 codec slice (PERCEPTION §9 step 1, BA37), then its motion slices.
   - 1c. Then M3.4 lane-change duration on M3.7, M3.6 turning speeds, M3.4 overtaking with the
     driving side, M3.8 motorcycles, M3.5 attention, M3.3.3b W99, M3.9 (review §5).
2. **Multi-level S0: the Node contract** (D175). Polygon, mapping of Micro detail to
   legs/turns/conflicts/g/C, coarse values and lifecycle, merged with M5.1's evaluation nodes
   ([workflow](plans/MULTI_LEVEL_WORKFLOW.md) §3, §10). Then S1 StudyPeriod + PCU. M9 keeps its
   scope but no longer leads the order.
3. **M3.4 parts that need neither M3.5 nor M3.7** (R7): `laneChangeDistance`, cooperative
   lane change. Discretionary changes (D95/D101, off) wait for M3.5's look-past (B2 route (i)).
4. **Smaller engine or editor slices** — each has its context in ENGINEERING_NOTES §Session work:
   - vehicle turning beyond phase 3 (swept-body clearance rows first)
   - M3.2.8a.1 moving merge anticipation
   - M3.2.4e merge admission contract
   - a per-lane share gesture (D32/D71)
   - keyboard-only route and input gestures (M1.25/M1.26)
   - M3.2 closure rows (a diverge fixture row, M3_ACCEPTANCE §4)
   - the third `validateDocument` walk set (D163)
5. **M5.7 rehearsal — prepare only:** ready a copy of the M2.6 template for the owner's real counts
   (sitting A3/B4) and a checklist; a session cannot close it.
6. **Behaviour Lab next slices, only on the owner's say:**
   - W74 parameters in the panel: needs `retuneSimulation` to allow a prototype → W74 change, so
     traits must exist
   - lane-change fields: need a lane-change wrap rule on circuits first (BEHAVIOUR_LAB §1.1)

   Give-way intersections in the lab wait for a circuit wrap rule for conflict zones and priority
   rules.

## Waiting on the owner (details in OWNER_SITTING)

- **Gates (A):** M0 plausibility, M3.2.7d T-junction exercise, M5.7 rehearsal.
- **Decisions (B):** the name, D95/D101, `sharedMouth`/`level`, the M2.6 template's real data,
  the standstill cap, routing stations, the example's cool-down (D146), the M6 benchmark choice
  (B9, before any comparable run), Thai sources for multi-level values (B12, D175), and whether
  the cited `w74` preset ships in the catalog with an amber value (B13, D176).
- **Milestone closures the owner makes:** M4.2 amber (D147), M5 (M5.8 look C7 included) and M8.
  M8 needs a development-access study run end to end, plus C8/C9.
- **Looks (C–F):** includes the new C10 Lab, C11 review fixes, C12 MSVC timing, E10 D151 GUI and
  E11 MAP10 OSM.
- Motorcycles are core scope (D177); they need M3.7 first (review §5).
- After the sitting, items still open and older than 14 days become ROADMAP carve-outs (R4).

## Known limits to keep in mind

- **Driving behaviour** ([review](audits/DRIVING_BEHAVIOUR_REVIEW.md) §2): one-dimensional
  vehicles, no lateral position, instantaneous lane changes, no reaction delay, one observed
  vehicle ahead and behind at unbounded distance, no turning-speed reduction.
- **M8:**
  - exit weights are one set per run (the row is summed over matrices)
  - lot rows' and compositions' time intervals are not used for departures or initial types
  - the lot report has one window
  - `--compare` has no lot block
- **Delay:** completed-trip delay includes source waiting and acceleration; it is not HCM control
  delay. The entry-acceleration bias needs travel-time sections, not a correction factor.
- **Before/after comparisons of one movement need ≥ 40 seeds (42–81, D88).**
