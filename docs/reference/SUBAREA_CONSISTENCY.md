# The subarea workflow and the cross-level consistency report (M12)

Written 2026-11-14 (D553) as the M12 contract, before any code. The owner picked M12 ("ทำต่อตามแนะนำ")
and answered its four choices with the recommended option (§8). It builds on S5's study area, which
replaced M12's stored cut with a run-time cut (D175, [STUDY_AREA](STUDY_AREA.md)). It fixes:
- how Meso runs on a study area's cut, the same cut Micro runs on;
- what each level's volume at a gate and on a kept Link is;
- how the levels are compared, and how the report is written.

Rule 4 applies: every number carries the not-validated marker. The report says how far the levels
are apart; it does not say which one is right, and it sets no threshold ("reported, not forced",
[MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §3.6).

## 1. Scope

- **In (R1):**
  - a Meso run over the study area's run-time copy (§3);
  - the report, per study period, at every gate and every kept Link: Macro, Meso and Micro
    volumes and the three pairwise comparisons (§4–§5);
  - `MultraConsistencyReport` JSON and CSV with provenance and marking (§6);
  - the CLI (§7).
- **In (R2), with its own amendment:** a Results page for the report.
- **Out:**
  - calibration against counts and ODME;
  - any threshold, pass/fail or colour on a comparison;
  - hybrid Meso–Micro (B26);
  - changing the editor's *Run Meso* or `--meso`: both keep running the whole network, since
    their result is the Meso cut's source and the side file's content (D331).
- **Already decided, not asked again:** the kept set, gates, the Macro cut, side files, stale
  rules and the marking lines are S5's (STUDY_AREA §4–§11), unchanged.

## 2. Today (read from the code on D552's commit)

| Item | Today | Where |
|---|---|---|
| Meso extent | `runMeso` loads the whole document; a study area only adds watched Links when the area cuts Meso | `src/project/meso_run.cpp`, `src/shell/meso_panel.cpp:55` |
| The cut copy | `studyAreaDocument` builds the kept network, gate zones and gate matrices; `compileDocument` uses it for Micro only | `src/project/study_area_run.*`, `src/project/run.cpp:75` |
| Macro volumes | `MacroAssignment::classFlows`, veh/h per class and Macro link; each Macro link names its `source` Link and station | `src/macro/assign.hpp`, `src/macro/graph.hpp` |
| Meso volumes | Per-link `up`/`down` curves in PCU; per (class, path) watched entrance counts (S5.4); per commodity entering and arriving curves (M11.5) | `src/meso/loading.hpp` |
| Micro volumes | No per-Link count output. `SegmentEnteredEvent` (time, vehicle, section) when a vehicle passes into a later section of its route; `DepartedEvent` at the first; `ArrivedEvent` with the route | `src/core/event_types.hpp`, `src/core/publish.cpp:44` |
| Seeds | `runSeedsParallel` and `Estimate` (n, mean, sd, 95 % half-width, t-based) | `src/runner/batch.hpp`, `src/project/parallel_seeds.hpp` |

## 3. The Meso area run (owner 1: the same copy as Micro)

- `runMeso` gains an opt-in `area` argument (default off). With it, and a study area that names a
  Node, it runs on `studyAreaDocument(doc, &advisories, data, sources, &provenance)`: the same
  kept set, gate zones and gate matrices that Micro compiles. The copy's advisories
  (`MICRO_AREA_INPUT_IGNORED`, the Macro cut's) are the run's.
- **Refusals,** before the graph:
  - the area refusals (`MICRO_AREA_NODE_COARSE`, `_NODE_INVALID`, `_EMPTY`,
    `STUDY_AREA_GATE_UNKNOWN`, `MICRO_AREA_PERIOD_NOT_RUN`) apply unchanged;
  - `MESO_AREA_SOURCE_SELF` at `studyArea.flowSource` when the source is `"meso"`: Meso would cut
    its own whole-network run to feed itself.
- **Bytes:** without `area`, or without a study area, `runMeso` is unchanged (SC1). The Meso side
  file is never written from an area run.
- **Units:** the copy's gate matrices are vehicles per class; Meso loads them in PCU as any OD
  demand (D300).
- **Provenance:** the run records `StudyAreaProvenance` as Micro's snapshot does (S5.5a).

## 4. What each level's volume is

For each study period P = [s, e) that lies wholly inside the run (S1's run clock) and each object
below, every level gives veh/h. A period only partly inside gets the advisory
`CONSISTENCY_PERIOD_OUTSIDE` and no rows. The objects are, in this order:
1. each entry gate, in cut order;
2. each exit gate, in cut order;
3. each kept Link, in network order.

- **Macro** (the source the cut was fed from; `flowSource` `"macro"` only):
  - a kept Link: Σ over classes of `classFlows` on the Link's Macro link with the smallest
    `fromStation` (the flow entering the Link), from P's Macro run (side file or run at compile,
    exactly as the cut took it);
  - an entry gate: the gate matrix's row sum for P, veh/h; an exit gate: its column sum.
  - Under `"typed"` the gate values are the typed flows and the Link values are empty: no
    assignment says what a Link carries.
- **Meso** (§3's area run, with every kept Link watched):
  - a kept Link: the vehicles entering it in P, Σ over (class, path) of the watched entrance
    count's increase over [s, e) ÷ the class PCU (STUDY_AREA §7's rule), × 3600 / (e − s);
  - an entry gate: the same on the gate's Link;
  - an exit gate: Σ over commodities whose path ends on the gate's Link of the arriving curve's
    increase over [s, e) ÷ class PCU, × 3600 / (e − s).
  - Curves are sampled at k · step; a period bound between samples is linearly interpolated.
- **Micro** (each seed, then `Estimate` over the seeds):
  - a kept Link: vehicles that enter it in [s, e). A vehicle enters a Link at its `DepartedEvent`
    when its departure route's first section lies on that Link, or at a `SegmentEnteredEvent`
    whose section lies on a Link other than the one of its previous section. Connector sections
    belong to no Link. A lane change (`LaneChangeEvent`) keeps the Link and counts nothing;
  - an entry gate: the same on the gate's Link;
  - an exit gate: `ArrivedEvent`s in [s, e) whose arrival route's last section lies on the gate's
    Link;
  - × 3600 / (e − s). The `Estimate` is the batch's: n, mean, sd, 95 % half-width.
- **Warm-up is not removed.** All levels start empty at the run's start; a period starting there
  under-counts at the finer levels. The report states the run start and each period, and says so
  in its notes; it does not correct it.

## 5. Comparisons

For each object and the pairs (Macro, Meso), (Macro, Micro), (Meso, Micro), with a the coarser
value and b the finer (Micro's is its mean):
- **Δ** = b − a, veh/h;
- **%** = 100 · (b − a) / a, empty when a = 0;
- **GEH** = √(2 (b − a)² / (a + b)), empty when a + b = 0.
  - Source: the GEH statistic of the UK DfT's TAG unit M3.1 *Highway assignment modelling*, used
    there to compare a modelled hourly flow with a count. Paraphrased here, not quoted. **The
    paragraph and edition are to be confirmed** (open item O1): this session's network did not
    reach the publisher's host.
  - MULTRA writes the number only. TAG's acceptability criteria are not applied (owner 2).
- A pair with an empty side gives empty Δ, % and GEH.

## 6. Output

- **JSON** `{"format": "MultraConsistencyReport", "version": 1, "notValidated": true, …}`:
  - `provenance`: project file name only, `projectSha256`, `flowSource`, the source side file's
    SHA-256 or `"run at compile"` or `"typed"`, the Macro periods, the seeds, Meso's step, slice
    and duration, and the tool;
  - `marking`: S5.5b's object, when it applies;
  - `periods`: each with `id`, `start`, `end` and `rows`; a row is `{kind: "entryGate" |
    "exitGate" | "link", id, name, macro, meso, micro: {n, mean, sd, halfWidth95}, comparisons}`;
  - `advisories` (the copy's, Meso's and §4's), `notes`.
- **CSV**: the not-validated marker line, S5.5b's marking lines, then one row per (period,
  object) with every column of the JSON row flattened. Numbers use the shared formatter
  (`csv_format.hpp`).
- Same project, side files and seeds give the same bytes.

## 7. CLI

`run_simulation --project P --consistency-report OUT [--seeds LIST] [--csv FILE]`:
- `--seeds` takes the batch's list syntax; the default is 42–46 (five seeds, owner 3);
- refuses `CONSISTENCY_NO_STUDY_AREA` for a project without a study area, and an existing `OUT`
  or `--csv` file before any run, as the batch does;
- reads the side files beside the project, as every other run does (S5.5b).

## 8. Owner choices (answered 2026-11-14, D553)

1. **How Meso runs on the cut.** **(a) The same run-time copy as Micro: chosen.** (b) Meso on the
   whole network, compared on the area's Links only.
2. **What the report compares.** **(a) Every gate and every kept Link, with Δ, % and GEH, no
   pass/fail: chosen.** (b) Gates only. (c) Every Link without GEH.
3. **Micro seeds.** **(a) Several seeds through the batch (mean and 95 % interval), default 5:
   chosen.** (b) One seed.
4. **Slices.** **(a) R1 CLI first, R2 the Results page after: chosen.** (b) Both in one slice.

## 9. Open items

- **O1 GEH source:** confirm TAG unit M3.1's paragraph, table and edition for §5. Needs the owner's
  copy, or the publisher's host allowed for the session. The formula carries no tuned number; R1
  may implement it, and the citation stays marked unconfirmed until then.
- **Warm-up:** a rule to drop the first minutes needs a source; none is invented (§4).
- **A gate on a Link that is also a kept Link** appears both as a gate row and as a Link row on
  purpose: the gate row compares what was fed, the Link row what passed.

## 10. Slices

| Slice | Content | Rows |
|---|---|---|
| R0 | This contract (docs only) | — |
| R1 | `runMeso`'s area run, `subarea_consistency.*`, JSON/CSV, `--consistency-report` | SC1–SC7 |
| R2 | The Results page (amendment first) | — |

## 11. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| SC1 | Byte identity | Meso, Micro and Macro outputs of shipped projects and of the S5 fixtures without `area` are byte-identical to D552's |
| SC2 | Meso area run | On SA4's two-Node network cut from Macro: Meso loads only kept Links and gate zones; under flow well below capacity each entry gate's entering vehicles over the run equal the gate matrix's trips to 1e-9; `"meso"` gives `MESO_AREA_SOURCE_SELF` at `studyArea.flowSource` |
| SC3 | Macro column | On SA8's corridor, each kept Link's Macro value equals `classFlows` read by hand from the assignment, and each gate's equals its matrix row or column sum |
| SC4 | Meso column | Recomputed independently from the curves; with flow well below capacity and a period much longer than the free-flow time, each Link's Meso value is within the hand bound q · T_ff / (e − s) of Macro's |
| SC5 | Micro column | Recomputed independently from the raw events of each seed; mean, sd and half-width equal the batch `Estimate` to 1e-9; the report twice gives the same bytes |
| SC6 | Arithmetic | Δ, % and GEH on fixed numbers by hand (e.g. a = 400, b = 360: Δ −40, % −10, GEH 2.0520); a = 0 gives empty %, a = b = 0 empty GEH |
| SC7 | CLI | A separate process writes JSON and CSV with the marker, exact hashes and the seeds; a project without a study area is refused with `CONSISTENCY_NO_STUDY_AREA`; a stale Macro side file shows in the marking |

## 12. R1 as implemented (D553)

- **Code:**
  - `src/project/subarea_consistency.*`: `runMesoArea`, `compareVolumes`, `microVolumes`,
    `consistencyReport`, `consistencyJson`, `consistencyCsv`;
  - `tools/consistency_cli.*`: `--consistency-report`, handled before `run_simulation`'s own parsing;
  - `estimate` (batch.cpp) is now public, so the report's `Estimate` is the batch's.
- **`runMesoArea`** is a new function rather than an `area` argument on `runMeso`. `runMeso` is
  untouched, so SC1 holds by construction and the test still checks it.
- **Readings taken:**
  - A Link's Macro value comes from `runMacro` on the original document for each period, not from
    the side file. A fresh side file gives the same flows (SA13). A stale one shows in the marking.
  - Rows come in cut order: entry gates, exit gates, kept Links. A zone inside the area is no
    gate. The flow it originates shows on the Link rows, and its fed value is left out of the
    gate rows.
  - A `"meso"` source refuses the whole report with `MESO_AREA_SOURCE_SELF`.
- **Observed on SC2–SC5's corridor** (area {A}, 600 veh/h west to east in 0–600 s, seeds 42–43):
  - Macro is 600 on A's outbound leg.
  - Meso is 592.7, inside the free-flow bound of 11.99 veh/h. All 100 trips entered.
  - Micro averages 240.
  - Micro is lower because A's major road runs north–south, so this movement gives way. Micro
    discharged about 306 veh/h onto the west leg in the period, and the rest queued at the source.
    Meso's Node ignores priority (`MESO_PRIORITY_IGNORED`).
  - The report shows the difference as designed. Nothing was tuned.
- **Rows:** all passed on their first run.
  - SC1–SC6 are in group `consistency` (`tests/subarea_consistency_tests.cpp`).
  - SC7 is `cli-consistency` (`tests/consistency_cli.cmake`, with its fixture and registration).
