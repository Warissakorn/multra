# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-16

- UI slice e1b (D199) is in: run vehicles have a casing painted outside their hit shape, and run
  signal bars read red/amber/green by shape. Owner review is OWNER_SITTING E20. The owner answered
  E19's north question (D198): north shows whenever there is a georeference.
- Stacked, unmerged branches: `claude/ui-slice-c-object-groups` (D194) → `claude/ui-slice-d1-properties`
  (D195) → `claude/ui-slice-d2-tables` (D196) → `claude/ui-slice-e1-hud` (D197–D199). Slices a (D192)
  and b (D193) are on main. E14–E20 wait on the owner, not a session.
- The casing costs paint time (D199: ≈ +9–16 % in a Debug offscreen run); a Release measurement is
  still owed if the owner reports slowness.
- `map-ui` failed once in a parallel `check` on 2026-10-16 and passed in every rerun (PROGRESS D196
  entry); if it fails again, capture its output before re-running.
- M3.7 lateral slices 1–3a are in (D187–D189, [evidence](evidence/lateral-motion.md)). Only tests
  set a target, so no product run moves sideways; BA39 holds. Zones still read route positions
  (slice 3b, deferred below).
- M3.5 is implemented through BA38 (D181–D186).
- Local evidence is Linux offscreen (Qt 6.4.2); no Windows or high-DPI verification is claimed.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — UI presentation slice e2: measured LOD refinement

Read the [slice plan](plans/UI_PRESENTATION_SLICES.md) §e2, [specification](plans/UI_PRESENTATION_SPEC.md)
§5 (the LOD table) and [D28](decisions/RECORD.md#d28) before any cache idea. Merge or rebase onto
main first if the stacked UI branches have merged by then.

**Do, as one session** (`src/editor/canvas.cpp`, `canvas_run.cpp` and the overlay files the
thresholds touch; reuse `viewScale()` and the grid helpers in `ui_design_tokens.hpp`):
1. Tests first, at the threshold boundaries: lane width just under and at 3 px (minor decoration
   suppressed only below), vehicle length just under and at 14 px (windshield/cab only at or above,
   minimum body 4×2.5 px kept); selected handles still available; Ctrl+A wireframe drawing and
   picking unchanged; no head, counter or control hidden; results unaffected. Use object tags and
   geometry assertions plus screenshots.
2. Decide what "minor decoration" is from the code before suppressing anything, and write the list
   into the D-row.
3. Any performance claim needs before/after `trafficsim-run-view-benchmark` and
   `trafficsim-editor-benchmark` runs on the same fixture and toolchain (D31). No new cache, spatial
   index, GPU renderer, snapshot or gesture change.
4. Accept: new D-row, before/after images, OWNER_SITTING §E row, full desktop `check`.

After e2 the UI presentation slices are done; the deferred engine queue below resumes only at the
owner's direction.

To build here: Ubuntu 24.04 needs `qt6-base-dev libproj-dev nlohmann-json3-dev lld` (BUILDING);
a cold Debug desktop build takes about 7 minutes on 8 cores, about 15 on 4.

## Deferred work — preserved from the preceding NEXT

The engine queue and owner-directed alternatives below are deferred during the UI slices,
not cancelled. Resume them only at the owner's direction; ROADMAP retains every gate.

1. **M3.7 LATERAL slice 3b: zone occupancy by span** ([LATERAL](reference/LATERAL.md) §5, §6,
   §10, D188/D189; BA39 kept in [DRIVING_BEHAVIOUR](plans/DRIVING_BEHAVIOUR.md)).
   - `summarizeZones`, `zoneHold` and `resolveRequests` (`src/core/conflicts.cpp`) read route
     positions; a straddler's other-lane span (`own = false`) must occupy that lane's zone too.
     Write the failure-first rows first (a straddler into a zone on the next lane; switch off
     byte-identical). Tests set `lateralTarget` or `lateral` to make a straddler (D189).
   - The hand mirrors stay one-dimensional until then: `placeVehicle`, Lab telemetry,
     `dead_end_waits.cpp` (LATERAL §10).
   - Re-run `trafficsim-engine-benchmark` with the switch off and time a switched-on run (§11).
   - Then slice 4 (display, BA46), M3.4 lane-change duration (the first product writer of
     `lateralTarget`), M3.6 turning speeds, M3.4 overtaking with driving side, M3.8 motorcycles,
     M3.5 attention, M3.3.3b W99 and M3.9 (the review's §5 order, D177/D178).
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
- **Looks (C–F):** includes the new C10 Lab, C11 review fixes, C12 MSVC timing, E10 D151 GUI,
  E11 MAP10 OSM and the UI rows E12–E20 (slice 0, D190, D192 icons, D193 palette, D194 Objects groups, D195 Properties, D196 tables, D197 HUD, D199 run casings).
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
