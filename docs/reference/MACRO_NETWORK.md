# Macro network and static assignment (S2, M10.1)

Written 2026-10-19 (D271) as a contract with eight owner choices (§12). The owner answered the
same day (D272: 1a, 2a, 3b, 4a, 5a, 6a, 7b, 8b); §4, §6, §7, §9–§11 and §14 carry the answers.
Slices S2.1 (D273, §16) and S2.2 (D274, §17) are implemented.
This is stage S2 of the multi-level workflow
([MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) §10, D175): "Macro compiles from a detailed
network (M10.1)". It fixes how one detailed project compiles into a Macro network, which values
that network needs and the document lacks, and how a static user-equilibrium (UE) assignment of
one study period runs on it.

Rule 4 applies: every Macro result carries the not-validated marker until M6's Macro benchmark
passes.

## 1. Scope

- **In M10.1:**
  - `compileMacroNetwork`: the drawn Links and Connectors as a directed graph;
  - link types and VDF (volume-delay function) parameters as data;
  - the Macro demand of one study period, in PCU;
  - static UE assignment by Frank–Wolfe, with the relative gap reported;
  - link outputs, the CLI and a first editor surface (choice 5).
- **Out:**
  - trip generation, distribution, mode choice (M10.2–M10.4);
  - skims, select-link and canvas volume bands (M10.5);
  - Roads, cross-section templates and centreline-only Links (S3);
  - Node delay in Macro (D175 answer 10);
  - Meso (M11).

### Already decided (cited, not asked again)

| Item | Decision | Where |
|---|---|---|
| VDF | BPR, α 0.15, β 4, with a local override | D158 (B7 answer 2) |
| Stopping | Relative gap 0.01 % (D272 choice 8b, replacing D158's 0.1 %) | D272 |
| Algorithm | Static UE, Frank–Wolfe first; fixed iteration order; ties broken by id; no seed; the same input gives the same bytes | D145 design §3.4 |
| Node delay | Not at first: link VDFs and fixed turn penalties | D175 answer 10 |
| Flow basis | `mean`; no `macroFlowBasis` field before a reader needs another value | D175 answer 13; STUDY_PERIODS choice 4a |
| PCU | Derived per mode (`modePcu`); a missing one is `MODE_PCU_MISSING` | STUDY_PERIODS §7 |
| Modules | `src/macro/` imports nothing; `src/project/` calls it directly | D145 design §3.4, §4 |

## 2. Today (measured from the code on D270's commit)

- **Links** (`src/model/network/network.hpp`) are one-directional:
  - `Link{id, geometry, lanes, level, displayType, laneOffset, name, boundaryMarkings,
    behaviourTypeId}`;
  - the lane count is `lanes.size()`;
  - the length is the reference polyline's;
  - no capacity, free speed or road class is stored. `behaviourTypeId` names a Micro driving
    behaviour, not a road class.
- **Connectors** join `from` and `to` lane references, each at a station on its Link (absent: the
  source end / target start), with their own polyline.
- **Nodes** (S0, [NODES](NODES.md)): `nodeMembership` gives legs, turns as object chains, and
  control. Nothing in Micro's compile reads a Node.
- **Demand** ([ZONES_AND_PARKING](ZONES_AND_PARKING.md) §3, [STUDY_PERIODS](STUDY_PERIODS.md)):
  - a zone's connectors sit at a Link's start (origin) or end (destination), each with a `share`;
  - `odMatrixWindow` gives a matrix's window;
  - `matrixModes` resolves a moded matrix's composition;
  - `modePcu` gives the share-weighted PCU, absent while any type lacks `pcu`.
- **No PCU, capacity or free-speed value ships** (D267).

## 3. The Macro graph (choice 1)

- **Macro nodes:**
  - the two ends of every Link;
  - every station on a Link where a Connector leaves or arrives.
- **Macro links (capacitated):** each piece of a Link between two consecutive Macro nodes.
  - Length: the piece of the reference polyline.
  - Lanes: the Link's lane count.
  - Capacity: lanes × the link type's `capacityPerLane`.
  - Free-flow time: length ÷ the type's `freeSpeed`.
- **Connector arcs (uncapacitated):** one per Connector, from its `from` station to its `to`
  station.
  - Free-flow time: the Connector's length ÷ the `from` Link's `freeSpeed`.
  - The time is fixed. With no turn penalties (choice 4), a turn costs only its travel.
- **Centroid arcs:** a zone's origin connector is an arc from the zone to its Link's start; a
  destination connector is an arc from its Link's end to the zone. They are uncapacitated, with
  zero time.
- **Levels:** a Link on another level joins the graph only through Connectors, as in Micro, so an
  overpass never forms a turn.
- **Nodes:** M10.1's compile does not read Nodes (choice 1a). A Node's interior Links stay links
  with their own type and capacity.
- **One name across levels (D272).** Every level compiles from the same document, never from
  another level's graph (D145). A Macro link is named by its Link id and station range, and Meso
  (M11.2) uses the same names. Meso's link-transmission model needs a node model with turn
  capacities and link storage; it builds that from Node membership. Static Macro needs only link
  VDFs.
- **Ordering:** Macro nodes and links are ordered by (Link id, station) and Connector id. Every
  later loop runs in that order, never in container order.
- **Refusals,** each at its path, before any assignment:
  - `MACRO_LINK_TYPE_MISSING` (§4);
  - `MACRO_LINK_TYPE_UNKNOWN`;
  - `MACRO_NO_PATH` at an OD cell whose connectors no path joins. This is Micro's `OD_NO_PATH`
    rule on the Macro graph.

## 4. Link types (choice 2)

```json
{ "id": "urban-arterial", "name": "Urban arterial", "capacityPerLane": 1440,
  "freeSpeed": 13.9, "vdfId": "bpr", "modes": ["car", "mixed"], "source": "…" }
```

- **Where they live.** `data/link-types/`, under the Demand catalog's presence rule (D269):
  - `definition.linkTypes` absent means the installed files;
  - present means the project's own;
  - a project-owned type is written at schema 50.
- **On the Link:** a new optional `linkType`, written at schema 50 only when set. Below 50 it is
  `EDIT_UNSUPPORTED_FIELD` at its path.
  - **Micro never reads it**, so every Micro output keeps its bytes (row MA1).
  - Absent means Macro refuses the Link (`MACRO_LINK_TYPE_MISSING`). There is no default type,
    because a default would be an uncited capacity (WORKFLOW §1.4).
- **Fields:**
  - `capacityPerLane`: pcu/h per lane, finite and > 0;
  - `freeSpeed`: m/s, finite and > 0 (the editor shows km/h);
  - `vdfId`: names a VDF (§5);
  - `modes` (optional): the modes allowed on the type; absent means every mode (D272 choice 3b).
    An unknown mode id is `INVALID_LINK_TYPE`;
  - `source`: a non-empty citation.

  A bad value is `INVALID_LINK_TYPE` at the field's path; a malformed file is
  `EDIT_CATALOG_READ`.
- **Which capacity.** The BPR parameters 0.15 and 4 were estimated against the 1964 manual's
  *practical* capacity, about the point where speeds fall roughly 15 % below free flow. A
  secondary source (Boyles, Lownes and Unnikrishnan, *Transportation Network Analysis*, vol. I)
  warns that pairing them with a contemporary capacity is a mistake. `capacityPerLane` is therefore
  **the capacity the VDF uses**, and its `source` must say which capacity it is.
  - The primary 1964 text has not been read. It is to be checked before any value enters `data/`.
- **Shipped:** no type (choice 2a), as with PCU (D267). The editor offers project-owned types.

## 5. VDF data

```json
{ "id": "bpr", "form": "bpr", "alpha": 0.15, "beta": 4, "source": "Bureau of Public Roads, Traffic Assignment Manual (1964)" }
```

- **Shipped:** `data/vdf/bpr.json`, D158's choice.
- **The form is code; the parameters are data.** `form: "bpr"` is
  t(v) = t₀ · (1 + α · (v / c)^β). Another `alpha` or `beta`, or a local file, swaps with no code
  edit (row MA12).
- **Refusals:** an unknown `form` or a non-finite or negative parameter is `INVALID_VDF`. A link
  type naming a missing VDF is `MACRO_VDF_UNKNOWN`.
- **The same primary-source check as §4 applies.**

## 6. Demand

- **One assignment per study period.**
  - The matrices assigned together are those naming the same `periodId`.
  - A matrix with its own times is a period of its own, `[startTime, endTime]` in run seconds.
  - Run Macro names the period it assigns.
- **Flow.** Each cell's trips follow Micro's split (ZONES_AND_PARKING §3): the origin and
  destination connector shares, giving one OD flow per connector pair. A class's flow is
  trips ÷ (period length / 3600), in veh/h, D175's `mean` basis; its load on a link is that times
  its PCU (§7).
- **One class per mode (D272 choice 3b).**
  - The matrices of one period that name a mode form that mode's class. A matrix naming only a
    composition forms a class of its own, keyed by the composition.
  - A class's demand is in veh/h. Its PCU is `modePcu` of the mode, or of the composition by the
    same formula.
  - `MODE_PCU_MISSING` at `odMatrices[i]` is refused while a class's PCU is absent.
  - A class uses only the links whose type allows its mode (§4); a composition class uses every
    link. A cell whose class has no allowed path is `MACRO_NO_PATH`.
- **Advisories, not refusals:**
  - `MACRO_INPUT_IGNORED`: vehicle inputs, routes and routing decisions are Micro demand, and
    Macro leaves them out;
  - `MACRO_LOT_UNCAPPED`: a parking lot is not capacity-limited in a static assignment. Its period
    arrivals are reported against capacity plus departures (WORKFLOW §8);
  - signals, conflict areas and give-way rules are Micro detail, and Macro ignores them with no
    row.

## 7. Assignment (`src/macro/`)

- **A pure engine.** It takes value types in and gives values out: no I/O, no clock, no unordered
  iteration. It gets its own `check_architecture` rule and a negative fixture, as M5.2 had.
- **All-or-nothing loading.** Shortest paths by Dijkstra over the ordered graph. A tie between
  two labels keeps the predecessor with the lower link order (row MA7).
- **Classes share one cost.** A link's load is Σₘ pcuₘ · vₘ, and every class sees the same time
  t(load). The costs are therefore symmetric, and Frank–Wolfe still minimises one Beckmann
  objective. Each class loads all-or-nothing on its own allowed subgraph, and one step size moves
  every class.
- **Frank–Wolfe** (LeBlanc, Morlok and Pierskalla, *Transportation Research* 9(5), 1975,
  pp. 309–318, the method's first traffic application, tested on the Sioux Falls network):
  - start from all-or-nothing loading at free-flow times;
  - each iteration loads all-or-nothing at current times and finds the step by bisection of the
    Beckmann objective's directional derivative;
  - the bisection is a fixed 60 halvings of [0, 1] (D274); a direction still descending at 1
    takes the full step.
- **Relative gap:** (Σₘ Σₐ vₘₐ tₐ − Σₘ Σ_rs dₘ_rs κₘ_rs) / Σₘ Σₐ vₘₐ tₐ, where tₐ is the time at
  the current load and κₘ is class m's shortest-path time on its allowed subgraph.
  - Definitions differ between sources (Boyce, Ralevic-Dekic and Bar-Gera, *J. Transp. Eng.*
    130(1), 2004, pp. 49–55), so the output names this formula.
- **Stopping:** at a gap of 0.01 % (D272 choice 8b) or at an iteration cap, a run setting.
  - **Run settings** (D275): `definition.macroSettings{targetGap, maxIterations}`, written at
    schema 51 only when present. Absent means a target of 10⁻⁴ and a cap of 1000 iterations.
    - The cap is a numerical limit, not a model value: reaching it is flagged, never hidden.
    - A target outside (0, 1), or a cap that is not a whole number from 1 to 100000, is
      `INVALID_MACRO_SETTING` at the field's path. Below 51 the key is `EDIT_UNSUPPORTED_FIELD`.
  - Stopping at the cap is `MACRO_NOT_CONVERGED`. The result is still written, flagged with the
    gap it reached. It is never hidden.
- **Determinism:** the same document, data and settings give the same bytes. Permuting the
  document's Links or Connectors changes no output byte (row MA11).
- **Cost:** wall time and Ir are measured on the benchmark network and booked in
  ENGINEERING_NOTES. No budget is set before the first measurement.

## 8. Turn penalties (choice 4)

- **Recommended (4a): none in M10.1.** A turn's cost is its Connector arc's travel time.
- D175 answer 10 named "fixed turn penalties" as Macro's eventual junction cost. No cited value
  exists. Published test networks (§11) carry none, so a penalty now would also break their
  comparison.
- The field (per Node control type, or per turn) is fixed with its first cited value, in a later
  slice.

## 9. Outputs

- **Per link** (Macro links and Connector arcs, keyed by their source object and station range):
  - flow in veh/h per class, and the total in pcu/h;
  - V/C on the pcu total (capacitated links only);
  - free-flow and congested time;
  - the link type.
- **Per run:**
  - the period;
  - the document revision and the data files read, through the input manifest;
  - the gap formula and the gap reached;
  - iterations, convergence and the advisories.
- **Periods are named** as the study period's id, or `matrix:<id>` for a matrix with its own
  times. Any other name is `MACRO_PERIOD_UNKNOWN`.
- **Format.** JSON and CSV through the shared formatters (`csv_format.hpp`), each carrying the
  not-validated marker line. A CLI flag, `--macro <period>`, writes them.
- **Editor** (choice 5a):
  - a *Run Macro* action that asks for a period and runs on the worker thread (D141), with progress
    and cancel;
  - a Results → *Macro links* table with the marker;
  - a result is marked stale when the document revision changes (WORKFLOW §5 step 4).
- **No output exists for a project that is never run in Macro.** Every Micro output keeps its
  bytes.

## 10. Node coarse values (S0.4, choice 6)

- NODES §5 says S0.4's coarse values "land with S2", and their fields are fixed by S2's contract
  against M10.1's first reader.
- **What M10.1 actually reads.** It compiles a Node's interior from its Micro detail (§3). A Node
  without detail (no internal Link or Connector) joins nothing in this graph, because Links join
  only through Connectors. Nodes without detail first arise from Roads (S3).
- **Answered (D272, 6a):** S0.4 moves to S3, so no field is stored before its reader. This amends
  D260's answer 5a ("with S2").

## 11. Benchmark (choice 7)

- **M10.1's rows are hand-computed** (§14): two parallel BPR links with a closed-form equilibrium,
  and a three-route network with one route unused.
- **The M10 gate** asks for "a published test network's equilibrium within a stated relative gap".
  - **Candidate:** Sioux Falls (LeBlanc et al. 1975; 24 zones, 24 nodes, 76 links), from the
    Transportation Networks for Research repository (Bar-Gera, Stabler). It publishes best-known
    flows with an average excess cost of 3.9 × 10⁻¹⁵ and an objective of 42.31335287107440.
  - **Licence:** the repository's page says its data sets are "for academic research purposes
    only". That is a use restriction, not an open licence. The repository's own LICENSE file has
    not been read.
- **Answered (D272, 7b): a textbook network** with a published equilibrium, so its data can live
  in the repository with a citation. The first candidate is Sheffi, *Urban Transportation
  Networks* (1985). Its exact example, values and published solution are named and checked
  against the book before row MA15 is written as code. Sioux Falls is not used.
- **Checked (2026-10-22, D294): Sheffi §5.2, Table 5.4** (book p. 120), Frank–Wolfe on
  **Figure 5.1** (book p. 114): three parallel links, one O–D pair, q = 10, and
  tₐ = t₀ₐ (1 + 0.15 (xₐ / cₐ)⁴) with t₀ = (10, 20, 25). The units are the book's (time and flow
  units), so the row runs on a hand graph.
  - **Capacities c = (2, 4, 3), derived.** The copy read has no image of book p. 114, and its OCR
    lost the capacities. Table 5.1 prints t₁(10) = 947 and t₂(10) = 137, giving c₁ = 2 and c₂ = 4.
    With c₃ = 3 a replay matches every number of Table 5.4 to its print precision; 2.8 and 3.2 fit
    worse. The owner confirms c against the print (OWNER_SITTING).
  - **Two misprints:** iteration 2's direction (printed link 1; the arithmetic needs link 3) and
    iteration 3's t₃ (printed 35.3; 25.3 by its own flows).
  - **Equilibrium:** equal times u = 25.456 at x = (3.583, 4.645, 1.772), z = 189.332. The book's
    last row prints z = 189.33 and t = (25.6, 25.7, 25.4).
  - Evidence: [macro-sheffi-benchmark](../evidence/macro-sheffi-benchmark.md).

## 12. Owner choices (answered 2026-10-19, D272: 1a, 2a, 3b, 4a, 5a, 6a, 7b, 8b)

1. **The graph.**
   - **(a) Links split at Connector stations; Connectors as uncapacitated arcs; Nodes not read in
     M10.1 (recommended).** Everything comes from objects already drawn. A Node's interior keeps
     its Micro capacity, and nothing is collapsed that S3 would have to undo.
   - (b) Collapse each Node's interior into one uncapacitated arc per membership turn. A turn could
     carry a penalty later, but the interior Links lose their capacity.
2. **Link-type values.**
   - **(a) Ship no type until a source names its values (recommended),** as PCU (D267). Studies
     enter project-owned types with their source.
   - (b) Ship types from a source the owner names (title, edition, table), saying which capacity
     the source gives (§4).
3. **Demand classes.**
   - **(a) One class in PCU (recommended).** It is enough for the published networks and needs no
     mode bans.
   - (b) One class per mode now, with mode bans per link type.
4. **Turn penalties.**
   - **(a) None in M10.1 (recommended),** §8.
   - (b) A per-control penalty table in data now. That needs cited values.
5. **Editor surface.**
   - **(a) Run Macro, a Macro links table and the CLI (recommended);** canvas bands in M10.5.
   - (b) CLI only in M10.1.
6. **S0.4 coarse values.**
   - **(a) Move to S3 (recommended),** §10. This amends D260's answer 5a.
   - (b) Store the fields in S2 anyway.
7. **Benchmark data.**
   - **(a) Sioux Falls measured locally and uncommitted until its licence is confirmed
     (recommended),** §11.
   - (b) A textbook network with a published solution (for example Sheffi, *Urban Transportation
     Networks*, 1985), once its exact example is named and checked.
8. **The target gap.**
   - (a) Keep D158's 0.1 %, always reporting the gap reached.
   - **(b) 0.01 % (recommended).** Boyce et al. (2004) found that a relative gap of 10⁻⁴ is needed
     before link flows stabilise between build and no-build runs. Comparing scenarios is what a
     study does with these flows. Their network was large, and their gap definition is checked
     against §7's before the number is used.

## 13. Slices

| Slice | Content | Rows |
|---|---|---|
| S2.1 | Link types and VDF data, `Link::linkType` at schema 50, `compileMacroNetwork` (pure, `src/model/`), Macro demand. **Implemented (D273)** | MA1–MA6 |
| S2.2 | `src/macro/`: all-or-nothing loading, multi-class Frank–Wolfe, gap, convergence. **Implemented (D274)** | MA7–MA11, MA16, MA17 |
| S2.3 | Run settings, `MACRO_NOT_CONVERGED`, the link outputs in JSON and CSV, CLI `--macro`. **Implemented (D275)** | MA12, MA13 |
| S2.4 | Run Macro on the worker with stages (§19 a), the convergence record, the Macro links table and stale marking; the link-type editor (a types table, the Link's type field). **Implemented (D277)** | MA14, MA18 |
| Gate | Benchmark (choice 7): Sheffi Table 5.4 (§11). **Row implemented (D294); the gate is the owner's** | MA15 |

## 14. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| MA1 | Shipped projects; a Link given a `linkType` | Saved bytes and schema unchanged without one; Micro single-run and batch outputs byte-identical with one |
| MA2 | `linkType` and owned types | Round-trip at 50; 49 refused at the path; presence rule as D269; `INVALID_LINK_TYPE` at each bad field |
| MA3 | A Link with a Connector leaving mid-Link | Two Macro links split at the station, lengths and times as hand-measured; the Connector arc's time from its `from` Link's speed |
| MA4 | The shipped four-leg with types set | Every Link piece and Connector present; an overpass on another level joins nothing |
| MA5 | Zones | Centroid arcs at Link start and end; `MACRO_NO_PATH` where Micro has `OD_NO_PATH` |
| MA6 | Demand | veh/h per class = trips ÷ hours, by connector share; classes by mode (or composition) and period; `MODE_PCU_MISSING` at the matrix |
| MA7 | All-or-nothing loading | Hand-checked paths; a tie resolved by link order |
| MA8 | Two parallel BPR links | Flows equal the closed-form equilibrium (equal times) within the target gap |
| MA9 | Three routes, one never used | The unused route carries no flow at equilibrium; the used two have equal times |
| MA10 | Gap and cap | The gap equals §7's formula on hand values; a cap of 1 flags `MACRO_NOT_CONVERGED` and still writes the result |
| MA11 | Determinism | Same bytes twice; permuted Links and Connectors give the same bytes |
| MA12 | VDF data | Another α and β change the flows with no code edit |
| MA13 | Outputs | Every field of §9; the marker line in JSON and CSV |
| MA14 | Editor | Run Macro is one action and no Undo step; a table row per link; an edit marks the result stale |
| MA15 | Benchmark: Sheffi Fig. 5.1 (§11) | (a) Capped at k = 1–5 iterations, the flows, times, objective and step match Table 5.4's printed rows (±0.01, ±0.2, ±0.5, ±0.001; the misprinted t₃ excepted); (b) at the 10⁻⁴ target the run converges on the equal-time equilibrium within 10⁻³ and z within 0.005 of the printed 189.33 |
| MA16 | A mode banned on a link type | That class never uses the link; its flow takes the next allowed path; other classes still use it |
| MA17 | Two classes, different PCU, one shared link | The link's load is Σ pcu · v, and both classes see the same time |
| MA18 | Convergence record and cancel (§19 a) | One entry per gap computed (iterations + 1), the last equal to the gap; an observer's stop cancels with the flows reached, never converged |

## 15. Seams

- **M10.2–M10.5** read the same graph and demand: generation and distribution fill matrices,
  mode choice splits them, skims come from §7's shortest paths.
- **S3:**
  - Roads give Nodes without detail, and S0.4 lands with them (choice 6a);
  - the `linkType` hierarchy gives a coarse Node's priority (NODES §5).
- **S5:** the run-time cut reads §9's link flows.
- **M6:** gains the Macro benchmark kind (D145 design §3.7).

## 16–18, 20. As implemented

The as-implemented records of S2.1 (§16), S2.2 (§17), S2.3 (§18) and S2.4 (§20) are in
[MACRO_NETWORK_IMPLEMENTED](MACRO_NETWORK_IMPLEMENTED.md), with their section numbers kept.

## 19. Run procedure (D276; answered (a), D277)

Studied in [evidence/macro-run-procedures.md](../evidence/macro-run-procedures.md): Visum's
procedure sequence and Aimsun Next's scenario and experiment. Both keep the run's definition and
limits apart from the network, both stop at a gap or a cap, Aimsun keeps convergence data per run,
and a 2022 Visum fault let a step with errors count as done.

**The stages** (`runMacro` today, named for the editor):
1. **Inputs:** read and hash the project and catalog files.
2. **Period:** the named study period, or `MACRO_PERIOD_UNKNOWN`.
3. **Graph:** `macroNetwork`; its issues refuse the run.
4. **Demand:** `macroDemand`; its issues refuse the run, and its advisories are kept.
5. **Assignment:** Frank–Wolfe to the gap or the cap (`MACRO_NOT_CONVERGED`).
6. **Outputs:** JSON, CSV, or the Results table.

A stage that refuses stops every later stage. A stage is never counted as done with an issue.

**Owner choice for S2.4 (answered 2026-10-19, D277: (a), with more capability wanted later).**
(b) and (c) stay booked as seams. `runMacro(period, settings)` is the unit that a later run list
(b) or procedure sequence (c) repeats, so neither needs a new engine.
- **(a) Recommended, chosen.** One run setting per project, as today.
  - Add a per-iteration convergence record (iteration, gap, step λ) to `MacroAssignment` and to
    the outputs, as Aimsun keeps convergence data. It is a measured value, not a setting, so no
    schema changes.
  - The Run Macro dialog shows the six stages with their status and messages, and Cancel stops
    between iterations.
- (b) As (a), plus several named Macro run definitions (period plus settings) in the document,
  like Aimsun's experiments. This needs schema 52 and an editor list.
- (c) A general procedure sequence like Visum's (generation → distribution → mode choice → skim →
  assignment). It is premature while M10.2–M10.4 do not exist, and is booked as an M10 seam.
