# Meso network and link-transmission loading (M11)

Written 2026-10-21 (D291) as the M11.1 contract, with seven owner choices (§11). No code is
written against it until the owner answers. It fixes:
- how one detailed project compiles into a Meso network;
- which values that network needs and the document lacks;
- how a link-transmission model (LTM) loads one study period's demand onto it, with spill-back.

Rule 4 applies: every Meso result carries the not-validated marker until M6's Meso benchmark
passes ([MULTI_LEVEL_MODELLING](../plans/MULTI_LEVEL_MODELLING.md) §5).

## 1. Scope

- **In M11.2–M11.4:**
  - `compileMesoNetwork`: the drawn Links, Connectors and Nodes as a Meso graph;
  - a jam density per link type;
  - the LTM engine for one study period;
  - signals and priority as capacity at nodes;
  - spill-back;
  - link and node outputs by time slice;
  - the CLI and a first editor surface (choice 6).
- **Out:**
  - **M11.5:** dynamic assignment, which reuses M9's loop. M9 is not started;
  - **hybrid Meso–Micro:** WORKFLOW answer 14 keeps only a sending/receiving boundary interface (§13);
  - **Roads and cross-section templates:** S3;
  - **parking lots in Meso:** a later slice; Micro keeps lot dynamics until then;
  - **mid-link zone connectors:** a compile-time split, booked for M10/M11 by WORKFLOW §10, not in M11.2.

### Already decided (cited, not asked again)

| Item | Decision | Where |
|---|---|---|
| Formulation | Link-transmission model (kinematic-wave based), for spill-back | D158 (B7 answer 1) |
| Gate | Analytic queue and shockwave fixtures pass; spill-back blocks the upstream link at storage capacity; same seed gives the same bytes | ROADMAP M11 |
| Classes | One class per mode, sharing capacity in PCU | D272 (3b) |
| Node detail | Meso builds its node model from Node membership in M11.2 | D272 (1a) |
| Link values | No link-type value ships until a source names it | D272 (2a), D267 |
| Modules | `src/meso/` imports nothing; `src/project/` compiles and calls it | D145 design §4 |
| Hybrid | A sending/receiving boundary interface in M11.1, with the step limit; no commitment to build it | WORKFLOW §9 answer 14 |

## 2. Today (measured from the code on D290's commit)

- **The Macro graph** (`src/macro/graph.hpp`, `compileMacroNetwork` in
  `src/model/demand/macro_compile.*`):
  - Links are split at Connector stations into directed `link` arcs, each with length, lanes,
    capacity and free time from its link type;
  - Connectors are uncapacitated `connector` arcs;
  - each zone connector has its own centroid node.
- **Link types** (`src/model/demand/definition.hpp`): `{id, name, capacityPerLane, freeSpeed,
  vdfId, modes, source}`. There is no jam density or wave speed. None ships (D273).
- **Nodes** (`nodeMembership`, NODES §12): in-legs, out-legs, turns as object chains, conflicts by
  turn pair with priority, and control (`signalised` when a head is inside or on an in-leg).
- **Signals** (`SignalController{cycle, offset, groups}`, `SignalGroup{greenStart, greenEnd,
  amber}`): fixed-time programs, so a group's green windows are known for the whole run.
- **Demand:**
  - `macroPeriods` and `macroDemand` (`src/project/macro_input.hpp`) give one period's veh/h per
    class and OD pair;
  - Micro's routes and routing decisions give paths with weights (D119), and M8.3 expands an OD
    matrix into routes (D155).
- **Macro assignment** (`assignMacro`) is link-based Frank–Wolfe. It keeps link flows, not path
  flows.
- **Missing for Meso:**
  - jam density;
  - turn-level capacity at a Node;
  - path flows from an assignment;
  - a time-varying loading.

## 3. The Meso graph (choice 1)

- **Links:** the Macro graph's `link` arcs, one Meso link each, with its length, lanes, capacity
  and free speed.
- **Nodes:**
  - **A Node** (S0) becomes one Meso node. Its in-links are the arcs ending at its in-legs'
    crossings, its out-links the arcs starting at its out-legs', and its turns are
    `nodeMembership`'s turns. The Links and Connectors inside the polygon are not Meso links. Their
    length enters the turn as a fixed free-flow time (the Micro turn path's length over the in-leg's
    free speed).
  - **Any other junction** of Connectors (a merge, a diverge, a lane drop) becomes a simple node
    from the Macro graph's topology.
- **Zones:** centroids as in Macro. A source link's demand enters at its upstream end, and a sink
  link takes everything its downstream end sends.
- **Refused:**
  - a Node whose membership has an issue (NODE_LINK_REENTERS, NODE_CONNECTOR_CROSSES,
    NODE_NO_TURN) gives `MESO_NODE_REFUSED` at the Node;
  - a link shorter than one step's free-flow travel needs the step limit of §4.

## 4. The link model (choice 2, choice 7)

- **Fundamental diagram:** triangular, per lane. Free speed `v` and capacity `q_max` come from the
  link type. Jam density `k_jam` is new (choice 2). The critical density is `k_c = q_max / v`, and
  the backward wave speed is `w = q_max / (k_jam − k_c)`.
- **State:** the cumulative vehicle numbers `N_up(t)` and `N_down(t)` at the link's two ends, per
  Newell's simplified kinematic-wave theory (Newell 1993, Part I: the cumulative count at any point
  follows from the boundary counts, with a triangular relation).
- **Per step `[t, t + Δt]`** (Yperman's LTM, as the search summaries report it; to be checked
  against the thesis before M11.3):
  - **sending volume:** `S = min(N_up(t + Δt − L/v) − N_down(t), q_max·lanes·Δt)`;
  - **receiving volume:** `R = min(N_down(t + Δt − L/w) + k_jam·lanes·L − N_up(t), q_max·lanes·Δt)`;
  - **cumulative values between steps:** interpolated linearly.
- **Step:**
  - a fixed `Δt` for the whole run, reported in the output;
  - **the limit:** `Δt ≤ min(L/v)` over links. It is proposed here and checked against Yperman
    before M11.3, together with whether `L/w` also bounds it;
  - a step above the limit is `MESO_STEP_TOO_LONG`, naming the shortest link (choice 7).
- **Storage:** `k_jam · lanes · L`. A full link's receiving volume is 0, which blocks its upstream
  node: spill-back.
- **Units:** veh/h per class; capacity in PCU/h, shared by `Σ pcu · flow` as in Macro (D272).

## 5. The node model (choices 3 and 4)

- **Per step, at each node:**
  - the in-links' sending volumes, split by turn;
  - the out-links' receiving volumes;
  - the node model gives each turn's transfer. Every vehicle on an in-link waits in its order
    (first in, first out per in-link), so a blocked turn holds the turns behind it.
- **Choice 3: the model.**
  - (a) An instance of the generic first-order node model of Tampère et al. (2011): the out-link
    supply is distributed in proportion to capacity, with node supply constraints for conflicts
    inside a Node. The paper gives requirements and an exact solution algorithm.
  - (b) Merge and diverge rules only (capacity-proportional merge, FIFO diverge), with no
    constraint for conflicts inside a Node.
- **Choice 4: signals.** A turn's capacity is the in-leg lanes' saturation flow (the link type's
  capacity per lane) times its group's green.
  - (a) Green as a time-varying window per step, from the controller's cycle, offset and green
    start and end, with amber counted as in Micro.
  - (b) A constant `g/C` per turn.
- **Priority (unsignalised).** The Node's conflicts by turn pair, with the minor turn yielding,
  enter the node model as node supply constraints. Under 3b they are ignored and reported as
  `MESO_PRIORITY_IGNORED`.

## 6. Demand and paths (choice 5)

- **Demand:**
  - one study period, per class in veh/h;
  - by the matrix's intervals where it has them, else constant over the period;
  - loading is deterministic (fluid). There is no seed, so the same input gives the same bytes. This
    meets the ROADMAP gate's "same seed" in its stronger form.
- **Paths until M9.** Every vehicle class needs turn shares at each node, from a set of paths.
  - (a) Micro's routes and routing-decision weights, which already compile, including OD → routes
    (D155). Meso and Micro then load the same paths, which is what M12's comparison needs.
  - (b) Path flows recovered from the Macro UE of the same period. This needs `assignMacro` to keep
    each iteration's trees and step weights, which changes S2's code.
  - (c) Free-flow shortest paths (all-or-nothing). Simple, but blind to congestion.
- **M11.5** later replaces the fixed paths with M9's loop.

## 7. Outputs

- **Link rows by time slice:**
  - inflow, outflow, mean density;
  - mean travel time (from the cumulative curves' horizontal distance);
  - queue (vehicles stored above the free-flow number);
  - a spill-back flag when the receiving volume was 0 in any step of the slice.
- **Node and turn rows by time slice:** transferred vehicles, and delay against the free-flow turn
  time.
- **Network totals:** VHT, VKT and total delay.
- **JSON and CSV** with the marker line first and the run's step, slice and input manifest.
- **CLI:** `--meso PERIOD`, as `--macro PERIOD`, with study-period ids or `matrix:<id>`.
- **The slice length** is a setting (default 300 s). A reporting interval is a choice of output,
  not a model value, so it needs no citation.
- **Editor (choice 6):** Run Meso and a Meso links table, or the CLI only.

## 8. Settings and data

- **`definition.mesoSettings{step, slice}`:** written only when set.
- **Link type field:** `jamDensity: {value, source}` (veh/km/lane), written only when used, at a
  new schema version. A Meso run with a link type lacking it gives `MESO_JAM_DENSITY_MISSING` at
  that type.
- **No value ships.** A study enters its own measured or cited value, as for capacity (D272 2a).
  The contract names no default, because a typical figure in a text is not a source for a Thai
  road.

## 9. Seams

- **Micro:** unchanged. Meso reads the same Links, Connectors, Nodes, signals and routes and writes
  nothing to them.
- **Macro:** shares the graph compile and link types. Meso adds `jamDensity`, which Macro never
  reads.
- **The boundary interface (WORKFLOW 14):** `meso::Boundary{sending(t), receiving(t)}` per
  boundary link, so a later hybrid can couple a Micro area. It is declared in M11.3 and nothing
  else is built.

## 10. Sources (as reported; the full texts were not opened from the session)

- G. F. Newell (1993), "A simplified theory of kinematic waves in highway traffic, Part I: General
  theory", *Transportation Research Part B* 27(4), 281–287; Parts II (bottlenecks) and III
  (multi-destination flows). Bibliographic details from TRID and IDEAS; the content from search
  summaries.
- I. Yperman (2007), the link transmission model (KU Leuven doctoral thesis); Yperman et al.
  (2005/2006) are the usual paper citations. The thesis was not found from the session, and its
  formulas and step limit are checked against it before M11.3.
- C. M. J. Tampère, R. Corthout, D. Cattrysse, L. H. Immers (2011), "A generic class of first order
  node models for dynamic macroscopic simulation of traffic flows", *Transportation Research
  Part B* 45(1), 289–309. Bibliographic details from IDEAS. The list of requirements is not yet
  read.
- C. F. Daganzo (1994, 1995), the cell transmission model: the alternative D158 did not choose.

## 11. Owner choices (recommendations first)

1. **The graph.**
   - **(a) A Node as one Meso node with its membership turns; other Connector junctions as simple
     nodes (recommended).** It uses the objects already drawn, and D272 booked it.
   - (b) Each Connector as its own Meso link, Nodes ignored. Simpler, but it loses turn capacity
     and conflicts.
2. **Jam density.**
   - **(a) A `jamDensity {value, source}` per link type, none shipped (recommended),** as capacity
     and PCU.
   - (b) Derived from the composition's vehicle lengths and Micro's standstill gap. One source, but
     Micro's values are not validated (rule 4), so the derived number would carry that doubt into
     Meso.
3. **The node model.**
   - **(a) Tampère et al. (2011), capacity-proportional, with conflicts as node supply constraints
     (recommended).** It is the cited general form and handles unsignalised priority.
   - (b) Merge and diverge rules only. It is faster to build and reports priority as ignored.
4. **Signals.**
   - **(a) Time-varying green per step from the program (recommended).** Queues grow on red and
     discharge on green, which ME7 checks.
   - (b) A constant `g/C`. Smoother, but there is no queue cycle.
5. **Paths until M9.**
   - **(a) Micro's routes and routing-decision weights (recommended).** They already compile, and
     Meso and Micro load the same paths.
   - (b) Macro UE path flows, which needs a change to `assignMacro`.
   - (c) Free-flow shortest paths.
6. **Editor surface.**
   - **(a) Run Meso, a Meso links table and the CLI (recommended),** as S2.4.
   - (b) The CLI only in M11.4.
7. **The step.**
   - **(a) A setting `step` (default 1 s), refused above the limit with `MESO_STEP_TOO_LONG`
     (recommended).** The step is visible and the same for every run.
   - (b) The largest step that meets the limit, chosen per network. It needs no setting, but it
     changes when a short link is drawn.

## 12. Slices

| Slice | Content | Rows |
|---|---|---|
| M11.2 | `compileMesoNetwork`, `jamDensity`, settings, refusals | ME1, ME2 |
| M11.3 | `src/meso/` LTM links and nodes, signals, spill-back, boundary interface declared | ME3–ME9 |
| M11.4 | Outputs, CLI `--meso`, editor surface | ME10, ME11 |
| M11.5 | M9's loop reused (after M9) | written with M9 |

## 13. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS). The analytic rows use hand-built Meso
networks, not drawn projects, so their numbers follow from the formulas alone.

| Row | Case | Asserts |
|---|---|---|
| ME1 | Compile the four-leg and the T-junction with a Node | One Meso node per Node with membership's turns; links as the Macro graph's pieces; a refused Node gives `MESO_NODE_REFUSED` |
| ME2 | A link type without `jamDensity`; a step above the limit | `MESO_JAM_DENSITY_MISSING` at the type; `MESO_STEP_TOO_LONG` naming the shortest link; a project without Meso fields keeps its bytes |
| ME3 | One link, inflow below capacity | Travel time = L/v within one step; outflow equals inflow after L/v |
| ME4 | A bottleneck (downstream capacity below inflow) | The queue grows at inflow − capacity and clears at capacity − inflow after the demand drops; conservation: `N_up − N_down` = stored, every step |
| ME5 | A queue's back moving upstream | The shockwave speed equals `(q₂ − q₁)/(k₂ − k₁)` on the triangular diagram, within one step's resolution |
| ME6 | Spill-back | A full link's receiving volume is 0 and the upstream link stops discharging; it resumes when space returns (the ROADMAP gate) |
| ME7 | A signalised turn | Throughput over whole cycles equals `s·g/C` under saturation; the queue cycles with red and green (choice 4a) |
| ME8 | A merge and a diverge | Capacity-proportional merge; a blocked diverge branch holds the in-link (FIFO) |
| ME9 | Determinism | The same input gives the same bytes; the order of links in the file does not change a number |
| ME10 | Outputs | Slice rows sum to the totals; the marker line first; spill-back flagged in the slice it happened |
| ME11 | Editor (choice 6a) | Run Meso on its own worker with Cancel; the Meso links table with the marker; stale by revision |
