# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-15

- Engine base `54f76be` (M3.5 slice 4, D184); this session adds the BA38 evidence tool
  and the [reaction-time discharge record](evidence/perception-discharge.md) (D185).
- The fixed experiment completed 240 runs (two models × three delays × 40 seeds), plus
  80 tick-by-tick absent-key controls and 32-tick copied-state replay per experimental run.
  Raw inputs/settings, source/output hashes and compiler flags are captured with the evidence.
- Local evidence is Linux headless; no desktop or Windows verification is claimed.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — pick one, as the user asks

Without a choice from the owner, take item 1. One system per session; contract and acceptance
rows first for engine work. Item 1 and its follow-ons use the review's order (§5), which the owner asked for
on 2026-10-15 (D177) and accepted (D178).

1. **M3.7 LATERAL slice 1: compile and codec** ([LATERAL](reference/LATERAL.md) §3, §9, §12,
   D180; acceptance BA45, with BA39 in [DRIVING_BEHAVIOUR](plans/DRIVING_BEHAVIOUR.md)).
   - Read the model/core/project module READMEs, ARCHITECTURE and the contract before adding
     types. Compile carriageway cross-sections and `drivingSide` only with the lateral switch on.
   - Store/validate `lateralMinDistanceStanding`, `lateralMinDistanceAt50` and `maxLateralSpeed`
     from one key table. Any present key requires all three. Reject missing, nonfinite,
     out-of-range and unused invalid entries with their paths; no shipped values or code defaults.
   - The next free schema is **33** (32 is perception). Write it only for an owned lateral key;
     refuse those keys in an older schema, and preserve older files' bytes without the keys.
   - BA45: codec, compile and both driving sides. BA39: all keys absent keeps today's Scenario
     and the prototype/W74/perception replay, CLI seed 42 and frozen baselines unchanged.
     Keep storage/compile support distinct from runtime occupancy; spans/predicates are slice 2.
   - `src/core/types.hpp` is near its 500-line budget; put substantial new lateral structures in
     their own core header rather than exceeding it. Do not add Qt/model/I/O dependencies to core.
   - Then lateral slices 2–4 (BA40–BA44, BA46), M3.4 lane-change duration, M3.6 turning speeds,
     M3.4 overtaking with driving side, M3.8 motorcycles, M3.5 attention, M3.3.3b W99 and M3.9
     (the review's §5 order, D177/D178).
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
  vehicles, no lateral position, instantaneous lane changes, no turning-speed reduction.
  Perception limits, multi-anticipation and reaction time are optional; with no keys the old
  single-leader, unbounded-sight, zero-delay path remains. A second rear follower is not modelled.
- **M8:**
  - exit weights are one set per run (the row is summed over matrices)
  - lot rows' and compositions' time intervals are not used for departures or initial types
  - the lot report has one window
  - `--compare` has no lot block
- **Delay:** completed-trip delay includes source waiting and acceleration; it is not HCM control
  delay. The entry-acceleration bias needs travel-time sections, not a correction factor.
- **Before/after comparisons of one movement need ≥ 40 seeds (42–81, D88).**
