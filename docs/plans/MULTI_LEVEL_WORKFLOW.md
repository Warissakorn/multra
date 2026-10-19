# Multi-level workflow — one document, any starting level (design, 2026-10-15)

The owner accepted every recommendation of the 2026-10-14/15 design review in chat
([D175](../decisions/RECORD.md#d175)). This document amends the [D145 design](MULTI_LEVEL_MODELLING.md):
where they disagree, this one wins. It is a plan, not evidence: **nothing here is implemented**.
[ROADMAP](../ROADMAP.md) owns scopes and gates; [NEXT](../NEXT.md) owns the queue. Every slice
still begins with its contract and acceptance rows; numbers and citations named below are
candidates that the contract checks against the source before any value enters `data/`.

## 1. Principles

1. **One document.** Each level compiles the same `*.traffic.json`; nothing is converted or cut
   into a second file (hard rule 3).
2. **Detail per object.** Every object has coarse defaults and can be refined one at a time; a
   coarse-only study never sees fine settings.
3. **Derived values are not stored.** A default is an absent field; an authored or overridden
   value is a present field; an object the program created carries `"generated": true` until a
   person edits it. No per-field status flag.
4. **Old files keep their bytes; no uncited default; every result of every level carries the
   not-yet-validated marker.**
5. **Offline first.** Every step works from local files; the network and OSM are conveniences.
   This matches MAP_BACKGROUND ("network failure never prevents network editing or simulation").

## 2. New and changed objects

| Object | Stored | Derived |
|---|---|---|
| **StudyPeriod** (new) | `id`, start/end (study clock), `profileId`, `macroFlowBasis` (default `mean`) | — |
| **OdMatrix** (changed) | `periodId`; exactly one of `compositionId` / `modeId` | A schema-31 matrix loads as an implicit period and **saves as schema 31 while no new feature is used** |
| **Mode** (new, `data/modes/`) | the composition it draws from | time-weighted PCU over the matrix period |
| **Vehicle type** (changed) | `pcu` + source; **no default** (Macro refuses `MODE_PCU_MISSING`) | — |
| **Zone** (changed) | kind `internal` / `gate` | — |
| **Cross-section template** (new, `data/cross-sections/`) | directions, lanes per direction, widths, median, shoulders, `linkType`, source | — |
| **Road** (new) | centreline + `crossSectionId` + overrides | Macro/Meso: one directed link per direction. Micro: lane Links generated from the template and stored as today |
| **Node** (new; merges M5.1 evaluation nodes) | polygon (absent = derived disc); **only without detail:** turns differing from default, control, major/minor, stage signal | **with detail:** legs, turns, lanes, g/C, conflicts from the Links, Connectors, conflict areas and heads inside the polygon |
| **Study area** (new) | polygon where Micro/Meso results are wanted | run-time subarea cut |
| **CountSet** (new) | one count store, **merged with D142 decision counts** | — |
| **Control points** (new) | ≥ 2 known coordinates in the project's EPSG | placement of images and imports |
| **Background image** (changed) | world file (`.pgw`/`.jgw`) support; several images later | automatic placement |

## 3. Node

**From Micro detail.** Objects inside the polygon belong to the Node; *Suggest nodes* draws
polygons around conflict clusters (`generated`). Links crossing the boundary inward/outward are
legs; a lane chain through the Connectors inside (`routeLaneChains`) makes a turn; conflict areas
between Connectors of different turns give the turn-conflict table and priority; heads and their
program give each turn's g/C; Links wholly inside collapse into turns. A Link crossing the
boundary more than once is refused; roads on different levels never form a turn.

**Coarse values** (Node without detail): every in→out pair except U-turns is allowed; control is
priority by `linkType` hierarchy; a signal needs cycle and stages entered by the user.

**Refining** opens the existing canvas zoomed and filtered to the polygon (no second editor):
turns propose Connectors, major/minor gives conflict areas with Yield/Stop, stages give a program
and heads. Coarse values are deleted in the same Command; **deleting the detail first derives
coarse values from it** in that Command, so nothing is lost and there is one owner.

**Lifecycle.** A dead-end Node stays (zones and gates use it). A two-leg Node with only default
values and identical cross-sections is merged away, joining the Roads, in the Command that made
it so. A Node with detail or authored values is never removed automatically; it is reported.

## 4. Road and the cross-section

- The template is a **creation tool** at first: *Apply template* rewrites both directions in one
  Undo step; a hand-edited Link is detached from its template and shown as such.
- **Turn pockets and approach widening belong to the Node**, not the cross-section. A
  cross-section change mid-road is two Roads joined by an uncontrolled Node.
- *Join opposite Links into a Road* serves Micro-first studies; *Assign template* sets many Roads
  at once (required when no OSM tags exist).
- **Nodes come from Road crossings on the same level**, each confirmed *junction / overpass*;
  endpoints snap within a screen tolerance while drawing and a metric tolerance on import;
  splitting at a crossing updates Routes, ZoneConnectors and OD in one Undo; *Merge nodes* serves
  roundabouts. OSM data (online or file) uses OSM topology, never geometric crossings.

## 5. The workflow — one loop for every level

```text
0 Study setup → 1 Network → 2 Demand → 3 Readiness → 4 Run → 5 Check
                                             ↑                     │
                                             └── 6 Refine ◄────────┘
                               7 Scenario (with-project) → 8 Compare and report
```

| Step | Content |
|---|---|
| 0 | StudyPeriods, modes/PCU, counts (CSV), study area, control points and EPSG |
| 1 | Reference image (OSM tiles, or image + world file) → draw Roads or import CSV / GeoJSON / OSM (online or file) / DXF later → Nodes from crossings → assign templates → refine only the Nodes that need it |
| 2 | OD per StudyPeriod: typed or imported, trip generation, or **derived from existing inputs and routes**. Expanding the area warns about `gate` zones now inside the network |
| 3 | Readiness per level, *Fill defaults*, seam checks; `generated` objects inside the study area count as missing for Micro |
| 4 | Run any level. Boundary flows are **cut at run time** from the latest coarser result, or typed by the user; the result records its document revision and every input result, and becomes *stale* when they change |
| 5 | Calibration against counts; cross-level report separating data conflicts from model differences |
| 6 | The program proposes Nodes to refine (high V/C, spill-back in Meso, failed calibration) |
| 7–8 | With-project = copy the file and compare with M5.8, extended to every level. Export is marked while `generated` objects or stale results remain |

Steps 0 and 2–8 never need the network.

## 6. Seam rules

- **Ownership with detail:** turns belong to Connectors, control to the detail, mid-block lanes
  to the cross-section.
- **Conflicts:** lane counts offer *use Link / use Node*; speed jumps warn only; lane positions
  use `reanchorConnectors`.
- **Severity depends on the level being run;** automatic adjustment happens only to defaults and
  only inside the Command that caused it — never silently.

## 7. Calibration, values and coordinates

| Item | Decision | Source to check in the contract |
|---|---|---|
| Calibration report | GEH < 5 for ≥ 85 % of counts **or** the flow test (±100 veh/h below 700; ±15 % for 700–2,700; ±400 veh/h above 2,700); criteria in `data/` | UK DMRB Vol. 12a as reported by secondary sources; the original wording and its current successor guidance must be read |
| Saturation flow | Local measurement first — the existing queue-discharge tool (M3.3.1) can measure it and feed a Node override; HCM base 1,900 pc/h/ln as the cited fallback | HCM; local studies range ~1,550–2,000 |
| Cross-sections | Thai design standards (Department of Highways, Department of Rural Roads) | edition to be named by the owner or an engineer |
| PCU | One value per vehicle type, cited, **flagged uncertain for motorcycles** (values depend on position in the green; Bangkok data in Powell 2000 report 0 in the first 6 s and ~0.53–0.65 later); tied to R3 | Thai source to be chosen by an expert |
| Coordinates | WGS 84 / UTM 47N (EPSG:32647) by default, 48N (EPSG:32648) east of 102°E; EPSG stored per project; **every import names its EPSG**, so legacy Indian 1975 data (EPSG:24047/24048) converts instead of landing hundreds of metres off | EPSG registry |
| Snap tolerance | ~8 px while drawing, 1–2 m on import, both adjustable | to be tested with users |

## 8. Answers to the remaining gaps

| Gap | Decision |
|---|---|
| Canvas at city scale; large images | **Measure first** (`tools/*_benchmark.cpp`, D28/D31), then wireframe when zoomed out (Ctrl+A exists), no lane detail outside the study area, image tiles only if measured slow |
| Parking lots in Macro | No capacity in static assignment; warn when period arrivals exceed capacity plus departures; lot dynamics stay in Micro/Meso |
| Four-step feedback | In M10 after the four steps pass: MSA-averaged congested skims, iteration cap, convergence always reported |
| Fine → coarse feedback | Never automatic: a report of turn capacity measured in Micro against the Node's, and an explicit, undoable *Use as override* with provenance |
| Run-time cut speed | Cut every run first; coarser results are saved to file with provenance; a cache keyed by (document revision, input result id) only if measured slow |

## 9. Owner answers (2026-10-15)

| # | Question | Answer |
|---|---|---|
| 1 | Change D145's direction | Yes; recorded as D175, M8 not reopened |
| 2 | Offline first | Yes |
| 3 | Node polygon first, merged with M5.1 | Yes |
| 4 | Road + cross-section templates | Yes; creation tool first |
| 5 | Sources | As §7; the Thai editions remain open (OWNER_SITTING B12) |
| 6 | Coordinates | As §7 |
| 7 | Import formats | CSV, then GeoJSON and OSM files; DXF on demand |
| 8 | Snap tolerance | As §7, to be tested |
| 9 | Node coarse values after refining | Delete; derive them back when detail is deleted (§3) |
| 10 | Node delay in Macro | Not at first: link VDFs and fixed turn penalties, so published test networks stay comparable |
| 11 | Boundary flow source | Both; the result records which |
| 12 | StudyPeriod changes M8's schema | Yes, with byte-identical schema-31 saves |
| 13 | `macroFlowBasis` / calibration / ODME | `mean` / §7 / decided in the ODME milestone's contract (candidate: gradient method keeping the prior matrix close) |
| 14 | Hybrid Meso-Micro | Keep a sending/receiving boundary interface in M11.1 (with the LTM step's CFL limit); no commitment to build it |

## 10. Order

| Stage | Work |
|---|---|
| S0 | **Node** contract and object (polygon, Micro mapping, coarse values, lifecycle) — merges M5.1's evaluation nodes. Contract written: [NODES](../reference/NODES.md) (D259); S0.1–S0.3 and S0.5 implemented (D261, D263–D265); S0.4 lands with S2 |
| S1 | StudyPeriod + mode/PCU (schema change with byte-identical schema-31 saves). Contract written: [STUDY_PERIODS](../reference/STUDY_PERIODS.md) (D266); owner choices answered (D267); S1.1 implemented (D268) |
| S2 | Macro compiles from a detailed network (M10.1) |
| S3 | Road + templates + Nodes from crossings + seam checks + *Assign template* |
| S4 | Readiness panel + *Fill defaults* + Node proposals |
| S5 | Micro on part of the network + run-time cut + stale marking (replaces M12's stored cut) |

Offline imports: the principle now; world file, control points and CSV import in M10; GeoJSON
and OSM files next (one importer with the online path); DXF, several images and user MBTiles on
demand. Bulk OSM tile download stays excluded (tile policy, MAP_BACKGROUND). Elsewhere: level
views on the canvas (M10.1 contract), metric names that never share a name across definitions
(before M10.5), matrix/zone CSV import and CountSet (M10), mid-link ZoneConnectors as a
compile-time split in Macro/Meso only — Micro draws driveways as their own Links (M10/M11),
ODME as its own milestone after M10.

## 11. Superseded parts of the D145 design

| D145 text | Replaced by |
|---|---|
| Centreline-only Links, refused by Micro (§3.1) | Roads; Micro leaves them out of its run and reports the seams |
| Node reused from M5.1 as if it existed (§3.1) | S0 builds it |
| Subarea cut stored with provenance (§3.6, M12) | Study area + run-time cut + stale marking |
| Matrix period bound to the run's duration (ZONES_AND_PARKING §2.3) | StudyPeriod; each level checks its own run tier |
| Order M9 → M10 → M11 → M12 (§7) | S0–S5 above; M9–M11 keep their engine scopes |
