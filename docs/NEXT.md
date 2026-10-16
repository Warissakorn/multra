# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-16

- UI slices a (D192, icon vocabulary) and b1 (D193, five palette groups, Delete separator, toolbar
  fit/Focus tests) are in, on branch `claude/ui-slice-a-icons` (no PR yet). Owner reviews: E14, E15.
- D191 corrects the six audit findings on main: route attribution, live speed reduction, demand
  retune refusal, signal integer bounds, stale runtime catalogs and empty conflict chains.
- M3.7 lateral slices 1–3a are in (D187–D189, [evidence](evidence/lateral-motion.md)). Only tests
  set a target, so no product run moves sideways; BA39 holds. Zones still read route positions
  (slice 3b, deferred below).
- M3.5 is implemented through BA38 (D181–D186).
- Local evidence is Linux offscreen (Qt 6.4.2); no Windows or high-DPI verification is claimed.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — UI slice b2: icons-only palette toggle

The user split slice b on 2026-10-16; b1 is D193. Read [slice plan](plans/UI_PRESENTATION_SLICES.md)
§b and [specification](plans/UI_PRESENTATION_SPEC.md) §2. The goal is canvas width: the Thai
1024×768 palette now scrolls to reach Measurement and Tools, and the Thai canvas is 471×228.

**Do, as one session** (`editor_palette.cpp`, `editor_workspace.cpp`, both locales;
`workspace_ui_tests.cpp`, `design_system_ui_tests.cpp`):
1. Decide first, with a screenshot of each option, what the narrow palette does with the bottom
   controls: the "Visible level" eyebrow label, the `editorVisibleLevel` combo (its
   `setMinimumContentsLength(10)` in `editor_workspace.cpp` sets the dock's minimum width) and the
   text-only `editorBackgroundButton` (Ctrl+B and the View menu also reach it). The level filter
   has no menu alternative, so it must stay reachable.
2. Tests first: a checkable palette action (new key, both locales) switches icon+name ↔ icons-only;
   in icons-only every tool row keeps its tooltip and accessible name "<name> · <chord>"; arrow
   traversal, chords and the D190 active/focus states still work; group headers stay identifiable;
   the toggle is not remembered (a new window starts with names); Reset layout restores names.
   Measure the canvas viewport at 1024×768 Thai before/after.
3. Keep `paletteItems_`, mode numbers, the hidden `editorTool` combo and the tab order
   tree → level → background → canvas. No new setting, no saved state, no token change.
4. D-row, before/after images, OWNER_SITTING §E row, full desktop `check`.

Then slice c (grouped Objects navigation; slice 0 is green), d1, d2, e1, e2, one session each.
Nested Objects-page toolbars are text-only on purpose (D192); revisit them only in c or d2.

To build here: Ubuntu 24.04 needs `qt6-base-dev libproj-dev nlohmann-json3-dev lld` (BUILDING);
a cold Debug desktop build takes about 7 minutes on 8 cores.

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
  E11 MAP10 OSM and the UI rows E12–E15 (slice 0, D190, D192 icons, D193 palette groups).
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
