# ROADMAP — MULTRA

Not a schedule. A **sequence**, so that any session can see where it sits and what closes the milestone it is in.

Milestone scopes and gates below are authoritative; dated reviews record evidence and proposals, not approvals. `NEXT.md` is the one live session queue.

Navigation: [M0](#m0--vertical-slice) · [M5](#m5--evaluation-and-reporting) · [M8–M12](#m8m12--multi-level-modelling-micro-meso-macro-d145) · [Reviews](#reviews): [2026-10-09](archive/ROADMAP-review-2026-10-09.md) (R1–R8) · [2026-10-03](archive/ROADMAP-review-2026-10-03.md) (O1–O10, S0–S5).

**Two rules, inherited from a prior effort that broke both and paid for it:**

1. A milestone closes **only** when its done-condition is met and its gate has passed. Merged code is not a passed gate.
2. If a milestone must ship incomplete, the missing part is **carved out into a new numbered milestone in this file, immediately**, not left as a note. "We'll come back to it" is how a roadmap stops being true.

---

## M0 — Vertical slice

The smallest thing that is genuinely a traffic simulator, end to end. Every layer exists in miniature, so wrong assumptions surface while they are still cheap.

**Scope:** two crossing one-way roads (four Links in `crossing.json`) · one connector each way · one fixed-time signal (two heads, one program each) · one vehicle input per road · Wiedemann-style car-following · a native 2D view showing vehicles as dots · a headless run that prints average delay.

**Done when:** `multra-desktop` shows vehicles accelerating, queueing at red, and discharging at green in a way a traffic engineer recognises as plausible — and CTest proves the same seed produces the same run on a fixed engine/toolchain.

**Where that observation is made (M1.24):** open `data/scenarios/crossing.json` in the editor and use Run/Pause/Step/Reset there. Until M1.24 this said the same thing about a separate M0 window; that window is gone, the observation is not. The editor reads a bare M0 scenario, steps the unchanged core, and its run status shows the mean trip delay and the safety-clamp count the retired window showed. `scenario-run-ui` pins this path to the `multra-cli 42` baseline — 45 completed trips exactly, mean delay 31.75870154816726 to within 1e-7 (D159 seeding; 31 and 29.249359418430977 before) — so an engineer judging plausibility in the editor is judging the same run the CLI prints. **Closed 2026-10-27 by owner ruling (D369):** the owner answered "ผ่าน แบบ A" after the measured run and a SUMO comparison ([evidence](evidence/m0-plausibility.md)). Caveat kept on the record: a bare M0 scenario holds amber as red, so 1–3 safety clamps per run fall on amber at the line. The unvalidated marker stays (D13).

**Explicitly not in M0:** lane changing, editing anything, saving anything, LOS, multiple seeds, priority control. The slice is about the *shape*, not the feature set.

---

## M0.1 — Native C++ migration (D15)

**Implemented 2026-09-11** (Linux; Windows desktop suites in `native.yml` since `e57b1b5`). C++20
core/model/CLI, CMake/CTest, Qt Widgets desktop, strict JSON loading, four TS baseline fixtures.
It does not close the owner's M0 plausibility gate, M1 or M7. Body in [archive](archive/ROADMAP-implemented-2026-10-15.md#m01--native-c-migration-d15).

---

## M1 — Network editor

The Vissim modelling surface, natively: links are first class, connectors are real objects, junctions are not something the user places.

**Status:** implemented except **M1.22 and M1.23 (open)**, M1.26's gate (open), M1.28's gate (open) and the M1.29 owner looks (open); M1.17 reverted (replaced by M1.18/M1.19, then D80's four-point mouth,
[CONNECTOR_FOUR_POINT_MOUTH](reference/CONNECTOR_FOUR_POINT_MOUTH.md)). **M1 usability accepted by owner ruling on 2026-09-25 (D49)**, after one timed attempt that did not include turn pockets or
an aerial image ([M1_ACCEPTANCE](plans/M1_ACCEPTANCE.md)). M0 plausibility accepted by owner ruling (D369). Carve-outs are filed at their number (M1.3.1, M1.5.1, M1.11.1, M1.12.1–M1.12.3).

**Done when:** an engineer draws a four-leg intersection with turn pockets from scratch, over an aerial image, in under 10 minutes, without reading documentation — and reopening the file gives back exactly what they drew.

**Includes, because D7 makes it non-optional:** left-hand / right-hand traffic as a project setting that actually drives connector and conflict geometry.

---

### M1.1–M1.21.1, M1.24–M1.27 — Implemented slices

Full bodies: [archive](archive/ROADMAP-implemented-2026-10-15.md) (this pass), [ROADMAP-M1-implemented](archive/ROADMAP-M1-implemented.md)
and [ROADMAP-M1.11-M3.1-implemented](archive/ROADMAP-M1.11-M3.1-implemented.md).

| Slice | Status |
|---|---|
| M1.1 Document and commands · M1.2 Canvas and background · M1.3 Link and lane tools · M1.3.1 Split links carrying heads · M1.4 Connector editor · M1.5 Inspection · M1.5.1 Demand tables · M1.6 Persistence | Implemented |
| M1.7 Run handoff and owner acceptance | Implemented; **owner acceptance by ruling, 2026-09-25 (D49)** |
| M1.8 Run in the editor · M1.10 Levels and display types | Implemented (no movement results/LOS in M1.8; no simulation effect from elevation) |
| M1.9 Network Objects sidebar, Vissim gestures | Implemented; **done when** a daily Vissim user draws the four-leg without searching for controls (owner test) |
| M1.11 Body attachments · M1.11.1 Interior attachments compiled · M1.12 Lane edges, markings, selection/copy | Implemented (M1.12 gestures: Windows CI only, no owner check) |
| M1.12.1–M1.21.1 (M1.12.2 not a defect, M1.12.3 closed by M1.19, M1.17 reverted) | Closed |
| M1.24 one-window editor · M1.25 demand by pointer | Closed |

### M1.22 — Remaining authoring and interaction requirements

**Open, narrowed by D143 (R8) to what is unbuilt** (checked in code 2026-10-09): Link spline/arc construction and curve parameters; extending and merging Links; reversing a Link that carries references (`reverseLink` refuses one today); shoulders/median/sidewalk display; per-Link driving side (the Network has one); snap priorities and angle constraints; layer locks; a multi-property inspector beyond bulk behaviour assignment (D128); object context menus beyond the demand menu; shortcut customisation and an accessibility pass. Built and no longer listed: History, nudging, rotation, Connector tapers, route-safe reversal, the fixed shortcut set and the Ctrl+K palette. **Gate:** command/reference roundtrips plus both-side gesture tests and keyboard-only owner exercise. Settle the conflicts in SPEC_AUDIT before changing geometry or gestures.

M1.22.1–M1.22.2 (History dock, nudging, rotation) are implemented and do not close M1.22.

### M1.23 — Interchange, document workflow and measured rendering

**Open; CSV object tables and PNG implemented in M1.23.1–M1.23.2.** Native-to-GeoJSON/CSV/PNG exports — "CSV" means the object tables (Links, Connectors, inputs, routes, decisions), as defined by D143 (R8); results CSV is M5's and exists (D104, D141); GeoJSON/OSM/Shapefile import and CRS mapping; recent files/tabs, spatial indexing/culling/LOD and optional renderer acceleration. Competitor-format imports and 3D require an explicit scope revision before implementation; the UTDF export for Synchro and Vistro is that revision, carried by M14 (D386). **Gate:** known-coordinate import/export fixtures, multi-document recovery isolation and a reproducible real-network benchmark. Do not claim 10k/100k-object performance in advance.

#### M1.23.1 — Authored object table CSV

**Implemented** (D365): File menu/palette exports Links, Connectors, inputs, routes and decisions
from the current document, with exact numeric values, Unicode, nested JSON cells and atomic
replacement. Contract/rows OC1–OC6: [OBJECT_TABLE_CSV](reference/OBJECT_TABLE_CSV.md).
Automated/platform evidence is recorded in PROGRESS; Windows and owner acceptance remain
separate. M1.23 stays open for its other interchange/document/rendering work and full gate.

#### M1.23.2 — Authored network PNG

**Implemented** (D367): full visible network/current-area PNG, bounded pixel dimensions,
optional image/cached map backgrounds with attribution, and atomic destination writes.
Contract/rows PI1–PI5: [NETWORK_IMAGE](reference/NETWORK_IMAGE.md). Linux/Windows evidence and
owner acceptance stay separate; the full M1.23 gate remains open.

### M1.26–M1.27 — Carriageway demand, per-lane shares, optimization · **M1.27 closed; M1.26 implemented, gate open**

M1.26: a route names Links and Connectors, compiled per lane (M1.26.1 `laneShares`, D32). M1.27:
build/redraw/engine optimization. **Gate still open for M1.26** (D143 R5): keyboard-only
equivalents of the route and vehicle-input gestures.

---

### M1.28 — OSM map background (owner request 2026-10-12)

**Scope:** viewport-only raster XYZ background on the existing editor camera, fixed WGS84
UTM reference, schema-29 persistence, asynchronous Qt Network loading, bounded caches,
adaptive reprojection, English/Thai settings/navigation and desktop packaging. The
2026-10-23 extension adds stable panning, wider bounded overview zoom and offline
country/province selection, with manual coordinates still available.
Contract and rows: [MAP_BACKGROUND](reference/MAP_BACKGROUND.md); plan:
[OSM_BACKGROUND](plans/OSM_BACKGROUND.md). No automatic road import or offline archive.
**Gate remains open:** MAP1–MAP9 automated/platform evidence and MAP10 owner alignment
and responsiveness on a real study. This does not close M1.23 interchange or any engine gate.

### M1.29 — Owner looks carved out of the sitting (D201)

**Open, owner.** Carved 2026-10-16 from [OWNER_SITTING](plans/OWNER_SITTING.md) under R4 (b), each
unanswered for more than 14 days; the full steps are in that sheet's history (`git log -p` on it).
(1) D83/D84 controls and gestures: chevrons, one Thai/English baseline, 24 px fields, plain left
click never authors, and is Ctrl+right too slow for heavy route/counter work? (2) PRs #73–#75
interaction cleanup on curved and overlapping roads. (3) M1.19/M1.20 Connectors: none deleted by a
Link drag unexpectedly, half a lane width as the "off the Link" distance, lanes meeting middle on
middle. **Gate:** the owner's yes on each, at 100/150/200 % on Windows. Merged code is not a passed look.

## M2 — Demand, run, first numbers · **CLOSED** (gate passed, D53; closed by D143)

Vehicle inputs per interval, compositions, turning proportions; Run gives delay and queue per movement.
**Done when** the M1 intersection with counted volumes produces a delay table — met. **Gate:** the
owner's pre-registered real study (D8, D38, D51–D53; C1 only, C2/C4 withdrawn), passed as *not
disproven* on 2026-09-25 ([M2_GATE](plans/M2_GATE.md)). M2.1, M2.7 and M2.8 continue as open
sub-milestones and do not reopen M2. Gate text and slice bodies: [archive](archive/ROADMAP-implemented-2026-10-15.md).

| Slice | Status |
|---|---|
| M2.0, M2.0.1 (D35), M2.2–M2.5 (D37, D39, D40) | Implemented 2026-09-24/25 ([M2_PLAN](plans/M2_PLAN.md) §4) |
| M2.6 owner gate study | Passed 2026-09-25 (D51, D53) |
| M2.1.1 routeless inputs and placed decisions (D42, D43) · M2.1.2 per-interval turning proportions (D45) | Implemented 2026-09-24 |

### M2.1 — Link/lane/Connector behavior and demand extensions

**Open.** Vehicle-owned desired speed versus Link limits, distributions, inheritance, road classes,
lane types/restrictions, partial/dynamic routing decisions. Persistence must not imply runtime
support. A desired-speed distribution curve on the vehicle type is implemented (B-5, D229,
[SPEED_DISTRIBUTION](reference/SPEED_DISTRIBUTION.md)); it is not validated. **Gate:** validated distributions and references, deterministic sampling, old seed
fixtures unchanged with defaults, observable runtime effects for every exposed parameter.

#### M2.1.3 — Recognition at an authored station · **In progress** (D119)

Schema 20 and the [station contract](reference/POSITIONED_ROUTING.md): passage-time selection,
discrete recognition boundaries, legacy opt-in. Automated platform checks and the owner's look are
separate gates. Multiple points on one Link and partial/dynamic routing are outside this slice.

### M2.7 — Signal heads by pointer; fixed-time Signal Controllers · **Open** (owner request, made before M2.6)
M2.7a (head = stop line, placed at the pointer's station, D47; by Ctrl+right-click since D84) and M2.7b (controllers, signal groups, schema 13, D48) implemented 2026-09-25, before the gate. The owner's use in M2.6 was to close it; `plans/M2_GATE.md` does not record whether that use met the done-condition. Detail and done-condition: [`plans/M2_PLAN.md`](plans/M2_PLAN.md) §4.

---

### M2.8 — Demand correctness and authoring improvements · **In progress**

Owner requested Demand-only improvement on 2026-10-05.
[DEMAND_IMPROVEMENT.md](plans/DEMAND_IMPROVEMENT.md) specifies validation, lossless and
explicit interval editing, and compiled preview (slices 1–4, PR #106). Slice 5 adds
[schema-18 project catalogs and editing](reference/DEMAND_CATALOGS.md); slice 6 adds
[schema-19 time/type rules](reference/DEMAND_TIME_TYPES.md). D142 (2026-10-09, schema 26) lets an
input take its volume from its entry decision's turning counts (owner's answer to O10's count-sheet
item; [§7](plans/DEMAND_IMPROVEMENT.md)). Existing M2 gate remains
observed; this extension needs its own automated and owner UI checks. Reporting,
LOS, warm-up and multiple-seed summaries belong to M5.

## M3 — Right-of-way: conflict areas and priority rules

**The right-of-way model an engineer controls.**

- Conflict areas as **editable input**: at each conflict point, choose which movement yields, or make it undetermined.
- Priority rules with real **gap time and headway in seconds and metres**.
- Stop and yield control.
- Signal heads placed **anywhere on a link**, not only at a stop line.

**Done when:** an unsignalized T-junction with a minor-road left turn produces plausible, tunable minor-road delay that responds correctly to changing the gap time.

**Preparation (2026-09-24):** [M3_PLAN.md](plans/M3_PLAN.md), [M3_CONTRACT.md](reference/M3_CONTRACT.md) and [M3_ACCEPTANCE.md](plans/M3_ACCEPTANCE.md) specify the ordered work and evidence. Written before M2.6; the M2 gate passed on 2026-09-25 (D53) and runtime work began with M3.2.2a (D54). Interior signal positions already worked in the model/runtime; M3.2.6a (D64, A21) audited the authoring/interaction path rather than adding a second signal-position mechanism.

---

### M3.1 — Merge priority by gap time and headway

**Implemented, and it does not close M3** — derived merge priority by gap time and headway (`PriorityRule`). M3's done-condition is not met by this milestone. Body in [`archive/ROADMAP-M1.11-M3.1-implemented.md`](archive/ROADMAP-M1.11-M3.1-implemented.md).

---

### M3.2 — Lane changing and crossing-conflict control

**Open.** Explicit priority rules/conflict areas, lane-change distances/emergency stopping and cooperation. By D143 (R7), calibrated gap acceptance moved to M6 and the M3.2.8c remainder to M3.4; M3.2 closes on the M3.2.7d owner exercise and its gate rows, with a diverge acceptance-fixture row still to write. Specify signal/right-of-way interaction without disabling collision constraints; account explicitly for any removed blocked vehicle. **Gate:** controlled merges, diverges and crossing conflicts, congestion/no-overlap regression, deterministic replay and the M3 owner exercise; scientific claims remain gated by M6.

#### M3.2.1–M3.2.9 — Ordered implementation slices

Contracts: [M3_PLAN](plans/M3_PLAN.md), [M3_CONTRACT](reference/M3_CONTRACT.md),
[M3_8_CONTRACT](reference/M3_8_CONTRACT.md), rows in [M3_ACCEPTANCE](plans/M3_ACCEPTANCE.md).
The full slice table (scope, evidence files, platforms) is in [archive](archive/ROADMAP-implemented-2026-10-15.md#m321-m329--ordered-implementation-slices).
Passing M3.2.7 alone does not close M3.2; the not-yet-validated marker remains.

| Slice | Status |
|---|---|
| M3.2.1 contracts · M3.2.2a–c (D54–D56) · M3.2.3a–c (D57–D59) · M3.2.5a/b Stop/Yield (D62, D63) · M3.2.6a–c signal positions and queue counters (D64, D65) | Implemented 2026-09-25 |
| M3.2.4a–c conflict-area editor and automatic areas (D60, D61, D68, D72, D84, D86) · M3.2.4d/f measured overlaps and mouth continuation (D115, D117, D118) | Implemented; **owner look pending** |
| M3.2.4e merge admission over the full mouth overlap | **Planned** — acceptance rows before changing the D50/D59 solver |
| M3.2.7a–c T-junction evidence (D66, D67) | Implemented |
| M3.2.7d owner exercise (§3, Windows) | **Pending, owner** |
| M3.2.8a commitment (D69) · M3.2.8b mandatory lane change (D71) · M3.2.8c routing-decision changes (D93) and cooperative braking (D90) | Implemented |
| M3.2.8c discretionary changes (D95) and hold (D101) | Implemented and **off** (D102); remainder moved to M3.4 |
| M3.2.9a–h lane correspondence and Connector ends (D73–D80, D106, D107) | Implemented; **owner look of D80 pending** |

### M3.2.10 — Owner looks carved out of the sitting (D201)

**Open, owner.** Carved 2026-10-16 from [OWNER_SITTING](plans/OWNER_SITTING.md) under R4 (b), each
unanswered for more than 14 days. (1) D90 cooperative braking on the M2.6 East approach: do
lane-2/3 left-turners slip into lane 1 with the vehicle behind slowing, not stopping dead? (2) D93
downstream decision on the four-leg West pocket: changes toward the turn with no LANE_UNSERVED (CLI
on Windows 118 : 41 : 0)? (3) D86/D72 conflict areas follow a dragged Link and go, with Yield and
line, when it is dragged clear; Ctrl+Z restores them. (4) D80 four-point mouths at 45–179°, 1–3
lanes, kerb/median lanes added or dropped; fixed lane-index pairing holds. **Gate:** the owner's yes
on each. This does not close M3.2, which still closes on M3.2.7d.

---

### M3.2.8a.1 — Remaining moving merge and source-clearance clamps

**Open (D105/D108, 2026-10-04).** The moving-leader standstill cap ([W74 clamp trace](evidence/w74-clamps.md)) is booked here by D201 (sitting B5).
Implemented: source first-step clearance (D108), the give-way stop (D240) and `clampRules` (D251, D258), leaving M2.6 at 5 clamps (pull-out 1, late catch-up 4) and the lane lab at 1; `joinAnticipation` ships off (D255); source clearance remains ([history D105–D258](archive/ROADMAP-implemented-2026-10-27.md#m328a1-clamp-history-d105d258)).

**Scope:** define how a minor approaching a merge anticipates the shared leader's standstill buffer before its span appears, and how a source insertion fits its first acceleration step into a small positive clearance. Keep the 1 m derived setback (D50), buffer and maximum deceleration until an explicit new contract says otherwise; do not suppress events or relax collision checks to remove counts.

**Gate:** contract and failure-first regression cases before code; trace every remaining cause; no body/swept crossing overlap or unreported excessive braking; accounting and same-build replay; before/after movement comparison over at least 40 seeds. No clamp-free claim or M3/M6 closure follows from D105 alone.

---

### M3.4 — Lane-change behaviour refinements

**Open; carved out of M3.2.8c by D143 (R7).** Visibility at conflict areas, a between-lanes state
(a change is instantaneous in the engine), `laneChangeDistance` where changes are measured late
(D87/D89), D95's booked route (i), the only way discretionary changes reopen, after M3.5's look-past (D201), and
Vissim's cooperative lane change. Extended by the [driving-behaviour review](audits/DRIVING_BEHAVIOUR_REVIEW.md)
(D177, §3 F–G): lane-change duration with between-lanes occupancy (on M3.7.0; [LATERAL](reference/LATERAL.md) §5, §7), minimum
lane-change headway and safety-distance reduction, politeness, zipper merging and a blocking
timeout, and overtaking with the driving side (keep-left/right bias, return after passing, lane
preference by type; needs `drivingSide` compiled into core). Keep-left is the default bias on
left-hand-traffic networks, held in a behaviour key (D178). **Lane-change duration contract written**
([LANE_CHANGE_DURATION](reference/LANE_CHANGE_DURATION.md), D204, rows BA48–BA53). **Slice 1 implemented** (D206,
[evidence](evidence/lane-change-duration.md)); its switched-on cost is back inside LATERAL §11's budget (+3.0–3.5 % Ir against `5847cb9`, D209,
[evidence](evidence/lateral-perf.md)). A swapping pair of stubs now resolves by the rear one yielding (D207,
[evidence](evidence/lane-swap.md)); a mid-change body waits at the line (D208) and follows the lanes it covers
(D210, [evidence](evidence/straddler-leaders.md), BA55); **keep-to-kerb contract written** ([OVERTAKING](reference/OVERTAKING.md),
D213, BA64–BA71) and **slice 1 implemented** (D215, [evidence](evidence/kerb-bias.md); A53 still fails, no D95 value ships); Vissim's cooperative lane change stays open. **Gate:** contract and failure-first rows per item in the
lane-change lab (D98), then acceptance fixtures; prototype baselines change only by a recorded decision.

### M3.5 — Perception, reaction and attention

**Open; carved by D177** ([review](audits/DRIVING_BEHAVIOUR_REVIEW.md) §3 A–C, K). M3.5.0 has a
perception contract: one bounded observation per vehicle and tick (look-ahead and
look-back distances, observed vehicles ahead and behind), a reaction-time action FIFO
kept in `SimState` (D179), consumed by every following model. Then: mirror checks with a second
follower and a time-to-collision rear test, temporary lack of attention, driver classes as
correlated hashed traits, visibility at conflict areas (from M3.4). **Gate:** contract and
failure-first rows; with every key unset, runs are byte-identical (BA18); no fidelity claim before M6.
**M3.5.0 contract written** ([PERCEPTION](reference/PERCEPTION.md), D179, rows BA30–BA38);
implementation follows its §9 order. **Slice 1 (codec, schema 32) delivered** (D181,
[evidence](evidence/perception-codec.md)); **slice 2 (look-ahead, multi-anticipation) delivered** (D182,
[evidence](evidence/perception-motion.md)); **slice 3 (look-back, rear TTC, perceived lane-change
tests) delivered** (D183, [evidence](evidence/perception-lane.md)); **slice 4 (reaction-time action
FIFO) delivered** (D184, [evidence](evidence/perception-delay.md)); **slice 5 (BA38 discharge record)
recorded** (D185, [evidence](evidence/perception-discharge.md)). Attention, driver classes and later
M3.5 scope remain open; this closes no owner gate.

### M3.6 — Speed control: turning, curves and speed areas

**Open; carved by D177** (review §3 D–E). **Curve speed contract written** ([SPEED_CONTROL](reference/SPEED_CONTROL.md),
D211, rows BA56–BA63); **slice 1 implemented** (D212, [evidence](evidence/curve-speed.md)); no value ships (D178). Reduced-speed areas by type, desired-speed decisions,
turning speed from Connector curvature, speed-dependent acceleration and grades. It delivers
M2.1's "desired speed versus Link limits" item rather than duplicating it. **Gate:** contract and
failure-first rows (curve speed bound, unset byte-identical); turning speeds measured in M6.

### M3.7 — Lateral behaviour: two-dimensional occupancy

**Open; carved by D177** (review §3 H, §4). M3.7.0 is a docs-only contract: lateral offset and
width in core, spans with lateral extent, rectangle tests for leaders, "alongside" and conflicts,
and a per-tick performance budget. Then desired lateral position, speed-dependent lateral
distance, overtaking inside a wide lane, position by turning direction and swept-body clearance.
**Gate:** contract and failure-first rows; a lane-based scenario keeps every vehicle centred and
replays byte-identically; no overlap in 2D at any tick. **M3.7.0 contract written**
([LATERAL](reference/LATERAL.md), D180, rows BA39–BA46). **Slice 1 (compile and codec) done**
(D187, [evidence](evidence/lateral-codec.md), BA45); **slice 2 (spans and predicates) done** (D188,
[evidence](evidence/lateral-spans.md), BA40–BA42); **slice 3a (motion, admission and crossing) done**
(D189, [evidence](evidence/lateral-motion.md), BA43, BA44; targets set only by tests); **slice 3b (zone
occupancy by span) done** (D202, [evidence](evidence/lateral-zones.md), BA47); **slice 4 (display) done** (D203,
[evidence](evidence/lateral-display.md), BA46). The M3.7.0 contract's slices are all in; the gate stays
open until a product writer of lateral targets (M3.4) runs and M3.7's later items are built.

### M3.8 — Motorcycles and lane sharing

**Open; core scope since D177** (answers R3/D143; review §3 I). After M3.7: a motorcycle type,
lane sharing, filtering through standing queues, forming at the stop line, smaller accepted
gaps, and PCU values handed to M10. Values start from cited published standards, flagged
uncertain; the owner's later research and M6 replace them through data (D178). **Gate:** contract and failure-first rows,
then M6 measurements of motorcycle discharge and PCU before any result is reported as more than
not-yet-validated.

### M3.9 — Intersection behaviour extensions

**Open; carved by D177** (review §3 J). Seeded per-driver critical gaps and follow-up times
(values from M6, R7), turn on red for marked movements, U-turn slots and median openings,
anticipating a turning leader, and an amber probability model as an option beside D147 (M4.2
is not reopened). Its place, eighth in the order, is accepted (D178). **Gate:** contract and failure-first rows per item; legacy random stream untouched.

---

### M3.3 — Driving behaviour library and models

**Status:** M3.3.0 contracts (D120); M3.3.1 discharge observer, type selection and passage
tracking (D121–D125, [DISCHARGE](reference/DISCHARGE.md)); M3.3.2 library, class/road assignment,
prototype selection and dialog (D126–D128; owner visual review open); M3.3.3a W74 contract and
implementation, runnable in schema 25 and **not validated** (D129, D135–D139, [W74](reference/W74.md)).
A cited, uncalibrated preset (SUMO's defaults, not auto-loaded) is recorded (D176,
[evidence](evidence/w74-preset.md)). **Next:** the M3.3.3b W99 contract before code, seventh in
the [review](audits/DRIVING_BEHAVIOUR_REVIEW.md)'s order (D177) so it consumes M3.5's observation. Evidence:
[delivery rows](plans/DRIVING_BEHAVIOUR.md), [interface](reference/DRIVING_BEHAVIOUR.md). M0/M6
gates, D102's off state and legacy baselines are unchanged.

### M3.3.4 — Behaviour Lab (D165)

**Status:** implemented 2026-10-14, Linux only ([contract](reference/BEHAVIOUR_LAB.md), rows
BL1–BL12): Simulation → *Behaviour Lab…* follows one vehicle round a stadium ring it laps
indefinitely (core circuit routes), with live retune of the prototype focus/traffic parameters and
a restart on road or vehicle-count changes. D166 adds the default Urban circuit (straights, curves,
signalised crossings and left turns, cross-traffic loops) and a heading-up camera. **Gate:** the owner's look (tune a behaviour, watch it
take effect without a reset, Apply to the project and Undo) and native CI on Windows. A
development bed: it closes no M0/M3/M6 gate and validates nothing. Later slices: W74 and the
lane-change fields in the panel; lane changes on a circuit need a wrap rule of their own first.

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in under 8 minutes with zero validation errors, and runs.

---

### M4.1 — Detectors and controller integration

**Open.** Detectors/DCP authoring and events, signal groups/controllers beyond M2.7b's fixed-time ones (D48), actuated/adaptive logic, phase validation and external-control interfaces. External integrations need explicit protocols and deterministic recorded inputs; no wall-clock dependency in core. **Gate:** passage/occupancy/aggregation fixtures, controller state-transition tests, dangling- reference cleanup and the M4 real timing-sheet exercise.

---

### M4.2 — Amber stop-or-go

**Implemented 2026-10-09 (D147, [AMBER](reference/AMBER.md), [gate evidence](evidence/m4.2-amber.md)); native CI per PR.**
Analytic rows AM1–AM8 cover both outcomes, the 40-seed before/after is recorded, and same-build
replay is byte-identical; the owner closes the milestone. Carved out by D143 (R2): D36 ran amber
as red, which biased every signalised delay an M5 table reports. Contract first: a stop-or-go decision at amber onset (can
the vehicle stop at a stated deceleration before the stop line?), intergreen handling, and
failure-first rows. It moves prototype trajectories, so frozen baselines and BA18 change only by a
recorded decision. **Gate:** analytic amber fixtures, both outcomes; before/after over at least 40
seeds (D88); same-build replay. The cited `w74` preset stays out of the auto-loaded catalog and
gets no `amberDeceleration` until this milestone is revisited (D201, sitting B13).

---

## M5 — Evaluation and reporting

The reason the whole project exists (`PROBLEM.md` §4). Slices in [M5_PLAN](plans/M5_PLAN.md)
(D130). **Implemented:** M5.2 batches and 95 % CI (D131), M5.3–M5.6 (D132–D134, D141), M5.8
scenario comparison (CLI D148, editor D149), M5.9 cool-down (D146). **Open:** M5.1, M5.7 owner
rehearsal, and the M5 gate.

- Movement-level delay, LOS, queue length, travel time.
- Multi-seed batch runs with means and confidence intervals.
- Scenario comparison: base, with-project and mitigated on one seed list (M5.8, D143 R1).
- Report tables that go into an impact study without passing through a spreadsheet.
- LOS thresholds as swappable per-jurisdiction data, never compiled in.

**Done when:** one intersection, ten seeds, one command → a movement-level LOS table with confidence intervals, ready to paste into a report.

---

### M5.1 — Additional network objects and evaluated outputs

**Open.** Evaluation nodes/stop lines, movement measurements/overlays and reports. **Node contract written** with S0 of M8–M12 ([NODES](reference/NODES.md), D259); owner choices answered (D260); **S0.1 implemented** (D261: object at schema 48, Node tool and tab, rows N1–N5); §3 amended (D262); **S0.2 implemented** (D263: membership, rows N6–N11); **S0.3 implemented** (D264: per-turn, leg and Node delay, leg queue and letters in single-run and batch outputs, rows N12–N17); **S0.5 implemented** (D265: Suggest nodes, row N18). **S0.6 implemented** (D278: Results → Nodes, rows N19–N21). **S0.7 implemented** (D288: the Nodes tab lights the Node and its legs, row N22). **S0.8a implemented** (D289: Node rows in the scenario comparison, rows CMP8–CMP10). **S0.8b implemented** (D290: Results → Nodes for a comparison, row N21). S0 is complete but S0.4, which moved to S3 (D272). Transit stops, crosswalks and multimodal behaviour moved to *Later* by D143 (R8); parking is M8 since D145. **Gate:** event-accounting and known analytical scenarios, reference cleanup and multi-run output checks. Keep completed-trip delay distinct from HCM control delay/LOS.

---

## M6 — Calibration and validation · **GATE**

**Done when:** the engine reproduces published benchmark results — capacity and delay for a signalized approach, gap-acceptance capacity for an unsignalized minor movement — within a stated tolerance, and the tolerance is published in the docs and shown in the app.

**M6.0 — benchmark option sheet** (D143, R2): docs only, at least two published options per benchmark kind, no engine numbers and no tolerance, so the choice stays pre-registered. **Delivered 2026-10-10:** [m6-benchmark-options](evidence/m6-benchmark-options.md), kinds S1 (saturation flow and capacity), S2 (delay) and U (minor-movement capacity and gap acceptance). The owner chooses by letter ([OWNER_SITTING](plans/OWNER_SITTING.md) B9); the tolerance is set after that choice and before any comparable run. Calibrated gap acceptance (moved from M3.2 by R7) is validated here.

**This is a hard gate.** Numbers from an unvalidated engine must never reach a regulator. Until M6 passes, every results screen carries a permanent "not yet validated" marker.

---

## M7 — Desktop packaging

Offline install, native file dialogs, no server required.

**Done when:** a non-technical user installs from a single file on Windows and macOS, opens a project by double-clicking it, and works with no network connection.

> The Qt desktop now exists from M0.1 (D15), with an automated controls smoke test.
> M7 still owns installers, clean-machine deployment, platform integration and file
> associations on Windows/macOS. A developer executable does not close M7.

---

## M8–M12 — Multi-level modelling: Micro, Meso, Macro (D145)

**Booked by the owner on 2026-10-09; starts after the D143 queue** (M5.9 → M4.2 → M6.0 → M5.8).
Design, decisions and slices: [`plans/MULTI_LEVEL_MODELLING.md`](plans/MULTI_LEVEL_MODELLING.md).
One authoring network and one demand compile to three views; each engine imports nothing; every
result of every level carries the not-yet-validated marker until that level's M6 benchmark
passes. M8 is code complete (gate open); M9–M12 are not started; each slice begins with its
contract and acceptance rows. **Equal weight (D378):** open level work follows the study chain, M10.2–M10.4,
M9, M11.5 and the M11 gate, then M12 ([PROBLEM](PROBLEM.md) §3); hybrid meso–micro is not booked (B26).
**Amended 2026-10-15 ([D175](decisions/RECORD.md#d175), [workflow](plans/MULTI_LEVEL_WORKFLOW.md)):**
stages S0 Node (merging M5.1's evaluation nodes) and S1 StudyPeriod + PCU come before M10.1;
Street segments with cross-section templates replace centreline-only Links; M12's stored cut becomes a
run-time cut over a study area. M9–M11 keep their engine scopes and gates; no gate moves.
**S1 contract written** ([STUDY_PERIODS](reference/STUDY_PERIODS.md), D266): StudyPeriod, the run clock, Mode and PCU; owner choices answered (D267); **S1.1 implemented** (D268: study periods, `clockStart`, matrix `periodId`, rows SP1–SP8); **S1.2 implemented** (D269 modes in the Demand catalog, D270: the matrix `modeId`, vehicle-type `pcu`, derived mode PCU, the Modes tab, rows SP9–SP14). S1 is complete.
**S3 complete** ([STREET_SEGMENTS](reference/STREET_SEGMENTS.md), D303–D312, rows RD1–RD29); **S4 complete** ([READINESS](reference/READINESS.md), D313–D317, RS1–RS10); **S4.5 complete** (D323–D325, RS11–RS18);
**S5 complete** ([STUDY_AREA](reference/STUDY_AREA.md), D326–D332, SA1–SA16) with the single-type cut follow-up (D338, SA17–SA18). No milestone gate changes. Slice bodies: [archive](archive/ROADMAP-implemented-2026-10-27.md#m8m12-s3-s4-s45-and-s5-slices).

### M8 — Zones, OD matrices and parking lots

**Street segment demand follow-ups D340–D342 implemented** (RD30–RD34, MR1–MR7; Micro requires generated Link targets; [archive](archive/ROADMAP-implemented-2026-10-27.md#m8-road-demand-follow-ups-d340d342)). No milestone gate changes.

**Code complete; gate open.** Contract [ZONES_AND_PARKING](reference/ZONES_AND_PARKING.md) (D153);
M8.2 codec/commands (D154), M8.3 OD → routes (D155), M8.4a/b lot runtime and authored lots
(D156, D157), M8.5 lot report (D160), M8.6a/b zone tools, OD table and lot UI (D161, D162); owner
answers D158. The gate waits on a development-access study run by the owner and the owner's look
(sitting C8, C9).
**Done when:** a development-access study runs from a zone OD matrix through a capacity-limited
lot; a full lot produces a gate queue and never deletes a vehicle; older files load
byte-identical; editor tools and the OD table ship in the milestone.

### M9 — Dynamic assignment (Micro)

**Open, not started.** Iterative between-run path-cost loop over the existing routing decisions;
costs in a separate file with provenance; convergence reported. En-route rerouting is excluded.
**Done when:** an analytic two-route fixture converges to its equilibrium split within the stated
tolerance; same inputs give the same bytes; non-convergence is flagged.

### M10 — Macro four-step model

**Open, not started; M10.1 contract written** ([MACRO_NETWORK](reference/MACRO_NETWORK.md), D271; owner choices answered, D272: one class per mode, a textbook benchmark, 0.01 % gap; S2.1 link types and the Macro graph implemented, D273; S2.2 multi-class Frank–Wolfe implemented, D274; S2.3 run settings, link outputs and CLI `--macro` implemented, D275; S2.4 Run Macro in stages, Macro links table, link-type editor implemented, D277; MA15, the textbook benchmark on Sheffi's Table 5.4, implemented, D294, its capacities awaiting the owner's check against the print; named run definitions implemented at schema 56, D339; a procedure sequence still waits on M10.2–M10.4). Macro network compile with link types and volume-delay functions as data;
static user-equilibrium assignment; trip generation, gravity distribution with Furness
balancing, multinomial-logit mode choice; skims, select-link, Results and Export; centreline-only
Links in the editor. Owner's B7 answers 2026-10-14 (D158): BPR 0.15/4 default VDF (1964 manual)
with a local override, doubly-constrained gravity with exponential deterrence and HBW/HBO/NHB,
modes car/motorcycle/bus/truck, 0.1 % relative gap (tightened to 0.01 % by D272) and Furness defaults (cited in the contract). **Done when:** assignment reproduces a published test network's equilibrium
within a stated relative gap; each step passes a hand-computed fixture; parameters swap with no
code edit.

### M11 — Meso engine

**Open; M11.1 contract written** ([MESO_NETWORK](reference/MESO_NETWORK.md), D291; owner choices answered 1a–7a, D292); **M11.2 implemented** (D293: the Meso graph, `jamDensity` and `mesoSettings` at schema 52, rows ME1–ME2); **M11.3a implemented** (D295: authored routes, static decisions and OD routes as Meso paths, turn shares and demand per class, rows ME12–ME16); **M11.3b implemented** (D296: routeless inputs and placed decisions through Micro's own expansion, stubs handed to their full routes, rows ME17–ME19; the M2.6 template's demand loads whole). **M11.3 node model implemented** (D297: Tampère et al. 2011 §5.1, checked against the paper and its Table 8, rows ME20–ME23; conflicts inside a Node deferred with `MESO_PRIORITY_IGNORED`); **M11.3c implemented** (D298: Yperman 2007 read; the link model and `loadMeso`, spill-back and the boundary volumes, rows ME3–ME6, ME8, ME9, ME24–ME26; Example 4.5 exact, §7.1 within its figures; turn splits by Eq. 5.2); **M11.3d implemented** (D299: signals as averaged green, Tampère §5.2 internal supplies with Yperman's shared-lane split and HCM's g = G; choice 4a replaced by 4b; rows ME7, ME26, ME27). **M11.4 implemented** (D300: `runMeso` over the whole Micro run (owner), outputs by slice in PCU, `--meso`, Run Meso and the Meso links tab, rows ME10, ME11); **M11.4b implemented** (D301: a turn's free-flow time as a lag charged to the out-link's storage (owner), row ME28); **M11.6a written** (D302: the Meso benchmark option sheet, [meso-benchmark-options](evidence/meso-benchmark-options.md)); **B23 answered** (D315: Boyles et al. 2025 for M1 and M2, Daganzo 1994/1995 secondary, not-blind cases as verification only; waiting on the copy). M11.5 waits on M9; the M11 gate stays open (a Meso benchmark, rule 4). Link-transmission model (owner, D158), signals
and priority as capacity, spill-back, M9's assignment loop reused. **Done when:** analytic queue
and shockwave fixtures pass; spill-back blocks the upstream link at storage capacity; same seed
gives the same bytes.

### M12 — Multi-resolution subarea workflow

**Open, not started.** Subarea OD cut from an M10 assignment with gate zones and provenance,
run in M11 and Micro, with a cross-level consistency report. **Done when:** one cut runs at both
finer levels with recorded provenance and a reported volume comparison.

### M13 — Analytical junction capacity (HCM 7, D370)

**Open; contract written** ([NODE_ANALYSIS](reference/NODE_ANALYSIS.md)), owner choices answered (D371); **A1 implemented** (D372: HCM 7 Ch. 19 pretimed protected exclusive lane groups, schema 58, `--node-analysis`, NA1–NA8; Exhibit 19-41 carved out to A2); **A2 implemented** (D373 contract, D374: shared lane groups, critical v/c, back of queue; NODE_ANALYSIS §11, NA9–NA14); A3–A9 outlined (NODE_ANALYSIS §12); **A3 implemented** (D375 choices, D376 contract, D377: [NODE_ANALYSIS_A3](reference/NODE_ANALYSIS_A3.md), NA15–NA20; permitted l+sl, lr and one-lane layouts and permitted queues open); A4 next when the owner asks. A deterministic, flow-based method on the Node that serves Micro, Meso and Macro and is not a fourth level (D378): HCM 7 Ch. 19 lane-group saturation flow, capacity, control delay and LOS first (A1), then queues, permitted turns, stop control, timing proposals, roundabouts and actuated control (A2–A9). **Done when:** A1's rows NA1–NA8 pass, with every other level's bytes unchanged and the marker and method label on every result.

### M14 — Junction templates and UTDF export (D386, D387)

**Open; contracts written, owner choices answered (D387, D388)** ([JUNCTION_TEMPLATES](reference/JUNCTION_TEMPLATES.md), [UTDF_EXPORT](reference/UTDF_EXPORT.md)). The owner's HCM-first workflow (D387: no builder tool): Street segments crossing make Nodes, a junction template on the Node generates pockets, lane-assigned Connectors, right-of-way and NEMA-numbered signals; transition templates join Street segments of different cross-sections; posted speed on the Link, the Node analysis panel, and a UTDF export for Synchro 12 and Vistro 2026 (the competitor-format scope revision M1.23 requires; imports stay out). Road was renamed Street segment first (slice SR, done, D389). U0's probe is prepared and waits on the owner's imports ([UTDF_PROBE](plans/UTDF_PROBE.md)). J1 is done: the posted speed is on the Link (schema 60, D390). J2 is done: command-only kerb/far pockets with measured taper/storage, route/head preservation and derived compass movement names (D391, D392; JT3–JT4); J3a is done: structural catalog/loader, Node recipes and independent directions at schema 61 (D393–D396; JT5); its review is approved. J3b is done (D398–D402; JT6–JT8/JT13–JT15), awaiting owner review before J4: two-way-stop fill and safe replacement/refusals; B31 defers all-way-stop generation until A5 or U1 needs it, with no StopControl or schema change (D399). J6 also books one Command to change a cross-section through a junction before U1 (D402). Slices SR, U0, J1–J7, U1–U3. **Done when:** rows JT1–JT15 and UT0–UT9 pass, and the owner's UT10 round trip imports the fixture into both programs with geometry, lanes, volumes and timing matching the sheet.

---

## Later, with their own milestones — not to be started inline

Recorded so nobody starts them opportunistically:

- Pedestrians (a 2D walking model, not lanes), public transport, transit stops, crosswalks (D143, D381)
- Site circulation and stall-level parking: parallel/bay stalls, reversing in core (D381; M8 stays abstract)
- Side friction: bus/songthaew stops blocking a lane, on-street parking manoeuvres, incidents
  and temporary lane closures (D177, review §3 L), with the transit work above
- Activity-based demand and transit assignment beyond M10's four-step model
- Scenario management beyond M5.8's comparison (a scenario manager, variant trees)
- 3D presentation
- Collaboration

---

## Reviews

Dated reviews record evidence and proposals, never approvals; both are answered and archived:
[2026-10-09](archive/ROADMAP-review-2026-10-09.md) (R1–R8, answered by D143) and
[2026-10-03](archive/ROADMAP-review-2026-10-03.md) (O1–O10, sessions S0–S5).
