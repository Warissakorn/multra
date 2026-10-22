# Meso network and link-transmission loading (M11)

Written 2026-10-21 (D291) as the M11.1 contract, with seven owner choices (§11). The owner
answered the same day: 1a–7a, as recommended (D292). §3–§8 state the chosen options. M11.2 is
implemented (D293, §14), and so are M11.3a (D295, §15), M11.3b (D296, §16), the node model (D297,
§17) and the link model with the loading engine (D298, §18). It fixes:
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

## 3. The Meso graph (choice 1a)

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
- **Simple nodes** (as implemented, §14): the stations joined by Connectors outside every Node form
  one node. A Link that continues through a station gives a through turn with time 0. Several lane
  Connectors between the same two links give one turn naming them all, timed by the first Connector
  by id.

## 4. The link model (choices 2a and 7a; checked against Yperman's thesis on 2026-10-22, D298)

- **Fundamental diagram:** triangular, per lane. Free speed `v` and capacity `q_max` come from the
  link type. Jam density `k_jam` is new (choice 2a), in pcu/km/lane like the capacity's pcu/h. It
  must exceed the critical density `k_c = q_max / v` (v in km/h), and
  the backward wave speed is `w = q_max / (k_jam − k_c)`.
- **State:** the cumulative vehicle numbers `N_up(t)` and `N_down(t)` at the link's two ends, per
  Newell's simplified kinematic-wave theory (Newell 1993, Part I: the cumulative count at any point
  follows from the boundary counts, with a triangular relation).
- **Per step `[t, t + Δt]`** (Yperman 2007, §4.6; the thesis writes w signed negative, so its
  `t + Δt + L/w` is the `t + Δt − L/w` below):
  - **sending volume (Eq. 4.31):** `S = min(N_up(t + Δt − L/v) − N_down(t), q_max·lanes·Δt)`;
  - **receiving volume (Eq. 4.35):** `R = min(N_down(t + Δt − L/w) + k_jam·lanes·L − N_up(t), q_max·lanes·Δt)`;
  - **cumulative values between steps:** interpolated linearly (Eq. 3.2–3.3);
  - **the three steps (§3.2):** volumes, then the node model at every node, then the counts, all
    from the state at t, so the order of nodes changes nothing.
- **Origins and destinations:** an origin is a point queue, `G = min(N_r(t + Δt) − N_up(t), R)`
  (Eq. 5.4); a destination receives without limit, `G = S` (Eq. 5.5). An origin whose first link
  also receives turns joins that node as a demand-constrained row (ours; it reduces to Eq. 5.4 on an
  entry link).
- **Step:**
  - a fixed `Δt` for the whole run, reported in the output;
  - **the limit:** `Δt ≤ min(L/v, L/w)` over links. The thesis prints only `L/v` (Eq. 3.4); `L/w` is
    derived (D298), because the receiving volume reads the downstream count at `t + Δt − L/w`, which
    is known only when it is not later than t. It binds only when `k_jam < 2·k_c`;
  - a step above the limit is `MESO_STEP_TOO_LONG` at `definition.mesoSettings.step`;
    `mesoShortestLink` names the link (choice 7a).
- **Storage:** `k_jam · lanes · L`. A full link's receiving volume is 0, which blocks its upstream
  node: spill-back.
- **Units:** veh/h per class; capacity in PCU/h, shared by `Σ pcu · flow` as in Macro (D272).

## 5. The node model (choices 3a and 4a; checked against the paper on 2026-10-22, D297)

- **Per step, at each node:** the in-links' sending volumes split by turn (`S_ij`, from the turn
  shares of §6), the in-links' capacities `C_i`, and the out-links' receiving volumes `R_j` go in;
  each turn's transfer `q_ij` comes out. Each in-link is first in, first out, so a blocked turn
  holds the turns behind it: conservation of turning fractions (CTF).
- **The model (3a):** Tampère et al. (2011), §5.1, the unsignalised instance of their generic class
  (Eq. 15). It meets the paper's requirements (§3.1): general n × m, flow maximisation,
  non-negativity, conservation, demand and supply constraints, CTF, and the invariance principle.
  - **Supply interaction rule:** oriented capacity proportional. `C_ij = (S_ij / S_i) · C_i` (Eq. 19)
    is in-link i's claim on out-link j's supply.
  - **Solution:** `q_ij = min(S_ij, a_{j*(i)} · C_ij)` (Eq. 28), with `a_j` the level of reduction of
    out-link j (Eqs. 25–27), solved exactly by the paper's algorithm (§5.1.2, Eqs. 29–32) in at most
    n iterations. The solution is unique (App. A).
- **Conflicts and priority inside a Node: not modelled yet (D297, amends D292 3a in part).** The
  paper gives no instance of node supply constraints for conflict points. Its Eq. 17,
  `Σ q_ij t_ij ≤ 1` after Brilon and Wu (2001), is "provisionally suggested", and the paper leaves
  the subject to future research.
  - A valid Node with conflicts is advised `MESO_PRIORITY_IGNORED` at `network.nodes[i]`.
  - Adding them needs Brilon and Wu (2001) read, a cited `t_ij`, and a new decision.
- **Signals (4a): deferred to M11.3d (owner, D298).** Explicit green under CTF holds a whole in-link
  while any of its turns is red, and the thesis (§5.2.1) instead averages green as `g/C` capacities.
  Until the owner decides, a valid signalised Node is advised `MESO_SIGNAL_IGNORED` and the engine
  loads it as unsignalised. As planned before that choice: explicit green per step. The paper (§5.2): "if the green and red phases are
  explicitly simulated, the node model of Section 5.1 can be applied unaltered for every green
  phase separately". The engine (the rest of M11.3) therefore withholds a turn's demand while its
  group is red; the averaged-green constraints (Eqs. 33–36) are not used.
- **Evidence:** [tampere-node-model](../evidence/tampere-node-model.md).

## 6. Demand and paths (choice 5a)

- **Demand:**
  - one study period, per class in veh/h;
  - by the matrix's intervals where it has them, else constant over the period;
  - loading is deterministic (fluid). There is no seed, so the same input gives the same bytes. This
    meets the ROADMAP gate's "same seed" in its stronger form.
- **Paths until M9 (5a).** Every vehicle class needs turn shares at each node. They come from
  Micro's routes and routing-decision weights, which already compile, including OD → routes
  (D155). Meso and Micro then load the same paths, which is what M12's comparison needs. Each
  route's Links and Connectors map to Meso links and turns in M11.3a.
- **As built in M11.3a (D295):**
  - **Sources:** authored routes, static routing decisions (`withRoutingDecisions`, including
    interval and type weights) and OD matrices (`withOdDemand`).
  - **Classes:** one per composition (`composition:<id>`) or lone vehicle type (`type:<id>`).
  - **Placed decisions and routeless inputs (M11.3b, D296)** take Micro's own expansion
    (`expandRouteless` after `resolveCatalogs`). Its lane paths are mapped back to Links and
    Connectors. A stub's flow goes to the full routes its vehicles change lanes onto, because a stub
    does not end the trip; a free-walk lane end is an exit, as in Micro (D44).
  - **The engine splits by route composition, Yperman Eq. 5.2 (owner, D298; amends D295 in part).**
    `S_ij` is the routes of the vehicles at the link's exit, read from per-commodity cumulative counts
    at its entrance, so vehicles already on a link keep their routes when a share changes.
    `mesoTurnShares` (shares at a time t) remains a summary of the demand, not the engine's split.
  - **Exact FIFO at a diverge** (the thesis's sub-flow procedure, §5.1.4) is not built; the thesis
    allows skipping it. A row's transfers are split by its `S_ij` composition.
  - **The demand covers the whole run.** Choosing a study period (which matrices enter, and the
    window) belongs to `--meso PERIOD` in M11.4.
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
- **Editor (6a):** Run Meso, a Meso links table and the CLI, as S2.4.

## 8. Settings and data

- **`definition.mesoSettings{step, slice}`:** written only when set, at schema 52; absent means
  step 1 s and slice 300 s. `INVALID_MESO_SETTING` at `.step` outside (0, 60] s, and at `.slice`
  when it is not a whole number (≥ 1) of steps.
- **Link type field:** `jamDensity: {value, source}` (pcu/km/lane), written only when used, at
  schema 52. `INVALID_LINK_TYPE` at `.jamDensity.value` at or below the critical density, and at
  `.jamDensity.source` when blank. A Meso run with a link type lacking it gives `MESO_JAM_DENSITY_MISSING` at
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

## 10. Sources

- G. F. Newell (1993) (as reported; not opened from the session), "A simplified theory of kinematic waves in highway traffic, Part I: General
  theory", *Transportation Research Part B* 27(4), 281–287; Parts II (bottlenecks) and III
  (multi-destination flows). Bibliographic details from TRID and IDEAS; the content from search
  summaries.
- I. Yperman (2007), *The Link Transmission Model for Dynamic Network Loading*, KU Leuven doctoral
  thesis. **Read on 2026-10-22** from the owner's Markdown edition (D298,
  [evidence](../evidence/yperman-ltm.md)).
- C. M. J. Tampère, R. Corthout, D. Cattrysse, L. H. Immers (2011), "A generic class of first order
  node models for dynamic macroscopic simulation of traffic flows", *Transportation Research
  Part B* 45(1), 289–309, doi:10.1016/j.trb.2010.06.004. **Read in full on 2026-10-22** from the
  owner's PDF (D297, [evidence](../evidence/tampere-node-model.md)).
- C. F. Daganzo (1994, 1995), the cell transmission model: the alternative D158 did not choose.

## 11. Owner choices (answered 2026-10-21, D292: 1a–7a)

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
| M11.2 | `compileMesoNetwork`, `jamDensity`, settings, refusals. **Implemented (D293)** | ME1, ME2 |
| M11.3a | Authored routes (static routes, static routing decisions, OD-expanded routes) → Meso paths and turn shares; demand per class (5a). **Implemented (D295)** | ME12–ME16 |
| M11.3b | Routeless inputs and placed routing decisions (the lane-level walk) → Meso paths. **Implemented (D296)** | ME17–ME19 |
| M11.3 | The node model. **Implemented (D297)** | ME20–ME23 |
| M11.3c | The link model and the loading engine (unsignalised), spill-back, the boundary volumes. **Implemented (D298)** | ME3–ME6, ME8, ME9, ME24–ME26 |
| M11.3d | Signals, after the owner's choice (D298) | ME7 |
| M11.4 | Outputs, CLI `--meso`, editor surface | ME10, ME11 |
| M11.5 | M9's loop reused (after M9) | written with M9 |

## 13. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS). The analytic rows use hand-built Meso
networks, not drawn projects, so their numbers follow from the formulas alone.

| Row | Case | Asserts |
|---|---|---|
| ME1 | Compile the four-leg and the T-junction with a Node | One Meso node per Node with membership's turns; links as the Macro graph's pieces; a refused Node gives `MESO_NODE_REFUSED` |
| ME2 | A link type without `jamDensity`; a step above the limit | `MESO_JAM_DENSITY_MISSING` at the type; `MESO_STEP_TOO_LONG` naming the shortest link; a project without Meso fields keeps its bytes |
| ME3 | One link, inflow below capacity | Travel time = L/v within one step; outflow equals inflow after L/v; the link stays below critical density |
| ME4 | A bottleneck (downstream capacity below inflow) | The queue grows at inflow − capacity and clears at capacity − inflow after the demand drops; conservation: `N_up − N_down` = stored, every step |
| ME5 | A queue's back moving upstream | On a chain of 500 m links behind a bottleneck, the back of the queue reaches each link's entrance at the shockwave speed `(q₂ − q₁)/(k₂ − k₁)` of the triangular diagram, within one step per link |
| ME6 | Spill-back | Yperman's Example 4.5 (10 km, 3600 veh/h, 120 km/h, 225 veh/km, Δt = 5 min, blocked downstream): the entrance count is 300 veh per step until 35 min (the restriction 2.5 min early, as the thesis states), reaches `k_jam·L` = 2250 at 40 min, and R = 0 after; a chain A → B → C with C narrower: A's transfer equals `R_B` while B is full, and the flow resumes when B drains (the ROADMAP gate) |
| ME7 | A signalised turn (M11.3d, waits on a decision, D298) | Throughput over whole cycles equals `s·g/C` under saturation; the queue cycles with red and green (choice 4a) |
| ME8 | A merge and a diverge | Yperman §7.1's diverge network (Table 7.1, links split at 500 m, Δt = 15 s, 90 min): the congested density on nm ≈ 167 veh/km (Fig. 7.5) and 2000 veh/h into m from each in-link; r2m clears by about 46 min and the queue on r1n lasts from about 28 to 82 min, each within Fig. 7.4's 6-minute spacing; the node model alone is ME21 |
| ME9 | Determinism | The same input gives the same numbers; a permuted link order gives the same per-link numbers |
| ME10 | Outputs | Slice rows sum to the totals; the marker line first; spill-back flagged in the slice it happened |
| ME11 | Editor (choice 6a) | Run Meso on its own worker with Cancel; the Meso links table with the marker; stale by revision |
| ME12 | Route mapping (M11.3a) | A route over Links and an outside Connector gives its Link pieces in order and the simple node's turn; through a Node, the Node's membership turn and no interior link; a Link split at a station, its through turn. Every path's turn k joins its links k and k + 1 |
| ME13 | Refusals (M11.3a) | A route whose Connector does not join its Links gives `MESO_PATH_UNMAPPED` at `definition.routes[j]`, and the other routes still map; the walk's blocking issues (a placed decision's destination no chain reaches) at `definition.inputs[i].…`; a class without PCU gives `MODE_PCU_MISSING` at its input, or at `odMatrices[k]` for a matrix's |
| ME14 | Turn shares (M11.3a) | A static decision with two routes 3:1, then 1:1 in its second interval, gives turn shares 0.75/0.25, then 0.5/0.5 at the diverge; two compositions keep their own shares |
| ME15 | Demand per class (M11.3a) | A class's entry veh/h at each time equals its inputs' (by `inputPeriods`); an OD matrix gives a `composition:` class at trips ÷ hours by connector share; PCU from `modePcu` |
| ME16 | Determinism (M11.3a) | The same demand twice; reversed Links, Connectors and inputs give the same demand |
| ME17 | Free walk (M11.3b) | A routeless 900 veh/h input on a 2-lane Link whose lane 1 leads to b and lane 2 to c: paths to b and c at 450 each, shares 0.5/0.5, entry 900, no refusal |
| ME18 | Placed decision with stubs (M11.3b) | The same Link with a placed decision to b and c at 3:1, then 1:1 by interval: object-level turn shares 0.75/0.25, then 0.5/0.5; the stub lanes' flow joins the full paths (forcing: Micro's expansion has dead ends), and none ends on the decision's Link |
| ME19 | The M2.6 template (M11.3b) | With owned types and PCUs, no issue; at several times Σ entry flows over classes equals Σ the authored inputs' volumes; every path's turns join its links |
| ME20 | Tampère Table 8 (M11.3 node model) | The paper's 4×4 example (its Table 1, `C_i` of Table 5) gives Table 8's `q_ij` within 0.5 veh/h (the paper prints a rounded `a_7`), and `a_7` = 0.685 ± 0.001 |
| ME21 | Merge and diverge (§5.1.3 of the paper) | A merge with both in-links supply-constrained gives `q_i = C_i/ΣC·R`; with one demand-constrained, the other gets `R − S_i`. A diverge with a blocked branch holds the in-link: `q = min(S, R_j/f_j)` (FIFO) |
| ME22 | Requirements (§3.1 of the paper) | On generated cases: `q_ij ≥ 0`, `q_i ≤ S_i`, `q_j ≤ R_j`, CTF `q_ij = (S_ij/S_i)·q_i`, and every flow actively constrained (its demand met, or its `j*` supply used up); invariance: `S_i` replaced by `C_i` for a supply-constrained i leaves `q_i` unchanged |
| ME23 | Priority advisory | A valid Node with conflicts gives `MESO_PRIORITY_IGNORED` at `network.nodes[i]` among the compile's advisories; a project without Nodes gives none |
| ME24 | Step limit and destinations (M11.3c) | A link whose `L/w` is below its `L/v` sets the limit: `MESO_STEP_TOO_LONG` above it, and `mesoShortestLink` names it; an out-link with infinite receiving volume (a destination, Yperman Eq. 5.5) takes the whole sending volume |
| ME25 | Route composition by Eq. 5.2 (M11.3c) | A diverge whose routes switch from 1:0 to 0:1 at T changes its turn flows only one free-flow link time after T, because vehicles already on the link keep their routes (time-t shares would switch at T); each commodity's entered and arrived vehicles sum to its demand once the network empties |
| ME26 | Signal advisory (M11.3c) | A valid signalised Node gives `MESO_SIGNAL_IGNORED` at `network.nodes[i]`; an unsignalised Node gives none |

## 14. M11.2 as implemented (D293)

- **The graph** (`src/meso/graph.hpp`, imports nothing):
  - `MesoLink`, `MesoTurn`, `MesoConflict`, `MesoNode` and `MesoGraph` (links, turns grouped by
    node, entries, exits);
  - `MesoSettings` with `kMesoDefaults{1, 300}`;
  - `mesoStepLimit` and `mesoShortestLink`.
- **The compile** (`compileMesoNetwork`, `src/model/demand/meso_compile.*`):
  - stations as Macro's, plus every valid Node's crossings;
  - pieces whose midpoint lies inside a valid Node are left out;
  - station groups by union-find over outside Connectors and each Node's crossings;
  - one node per group a link touches, ordered by the group's least (Link id, station).
  - `mesoNetwork(d, data)` (`src/project/meso_input.*`) resolves the link types and adds
    `linkTypeIssues`.
- **The file** (`src/project/meso_settings.*`, `link_type_codec.*`): `mesoSettings` and
  `jamDensity` at schema 52, `EDIT_UNSUPPORTED_FIELD` below it. The future-schema rows moved to 53.
- **Rows:** ME1 and ME2 in `tests/meso_network_tests.cpp` (ctest `mesonetwork`).
  - ME1 covers the T-junction and four-leg Nodes (4 and 12 turns), a junction without a Node, two
    lane Connectors merged into one turn, and a refused Node compiled as if it were not drawn.
  - ME2 covers the jam density and the settings, the step limit, and the shipped projects'
    schemas.
- **Mutations caught:**
  - interior pieces kept;
  - the through turn dropped;
  - lane Connectors not merged;
  - the schema gate removed;
  - the critical-density check removed;
  - a refused Node treated as valid.

  Two of them survived a first version of the rows. Lane Connectors not merged needed a fixture
  with two lane Connectors. A refused Node treated as valid needed the Node over the junction
  rather than in empty space.

## 15. M11.3a as implemented (D295)

- **The values** (`src/meso/demand.hpp`, imports nothing):
  - `MesoInterval`, `MesoPath` (its route, links, the turns between them, and veh/h intervals) and
    `MesoClass` (id, PCU, paths);
  - `mesoPathFlow`, `mesoTurnShares` (per turn, the class's share of its in-link's flow at t) and
    `mesoEntryFlows` (per link, veh/h entering at its upstream end).
- **The mapping** (`mesoPath`, `src/model/demand/meso_paths.*`):
  - Each Link of the route is driven over the range its Connectors give, and its Meso links in that
    range are taken in order, so a Node's interior drops out.
  - The turn joining consecutive links comes from the graph, which keeps one per (in, out) pair.
- **The demand** (`mesoDemand`, `src/project/meso_input.*`):
  - It expands the document as the Micro compile does (OD, then decisions).
  - Inputs on one route and class share a path. Classes and paths are ordered by id.
- **Rows:** ME12–ME16 in `tests/meso_demand_tests.cpp` (ctest `mesodemand`).
  - ME12 runs the four-leg with its suggested Node (12 routes, one Node turn each), the same project
    without it, and the T-junction's through turns.
- **Mutations caught:**
  - an inclusive interval end;
  - classes merged by vehicle type;
  - the routeless refusal removed;
  - the Connector join check removed;
  - shares not normalised.

## 16. M11.3b as implemented (D296)

- **The expansion is Micro's.**
  - `mesoDemand` runs `routelessIssues` and `expandRouteless` over the routeless inputs, after
    `resolveCatalogs`, as `compileDocument` does. Lane splits, slice shares and typed routing are
    therefore Micro's, with no second copy.
  - Each part returns to its authored input by id (`<id>/type-…` included), so a composition's
    type parts sum back into its class.
- **Lanes to objects:** `mesoObjectChain` (`src/model/demand/meso_paths.*`) maps lane sections to
  their Link and connector paths to their Connector, then `mesoPath` maps the chain as in M11.3a.
- **Stubs:** a route in `routeDeadEnds` hands its flow to the full routes its `laneChanges` lead to,
  stub after stub. The flow is split equally in id order.
- **Rows:** ME17–ME19 in `tests/meso_demand_tests.cpp` (ctest `mesodemand`). ME19 loads the M2.6
  template's whole demand (its four placed decisions), conserved at four times.
- **Mutations caught:**
  - the stub hand-off dropped;
  - a connector path not mapped to its Connector;
  - type parts not summed back;
  - the walk's issues not reported.

## 17. M11.3's node model as implemented (D297)

- **`mesoNodeFlows`** (`src/meso/node_model.*`, imports nothing) is the paper's §5.1.2 algorithm,
  step for step:
  - the sets `U_j` and `J`;
  - the reduced supplies `R̃_j`;
  - `a_j` by Eq. 30 and the most restrictive `ĵ`;
  - 4(a), demand-constrained in-links, before 4(b), the capacity-proportional share.

  An in-link without capacity sends nothing.
- **The advisory:** `MesoCompile::advisories` carries `MESO_PRIORITY_IGNORED` for a valid Node with
  conflicts.
- **Rows:** ME20–ME23 in `tests/meso_node_model_tests.cpp` (ctest `mesonode`).
  - ME20 reproduces the paper's Table 8.
  - ME22 checks the requirements on 300 generated nodes, over 50 of them congested.
- **Mutations caught:**
  - step 4(a) skipped;
  - a demand-proportional rule in place of oriented capacities;
  - the advisory removed.
- **Mutations that changed nothing:**
  - a per-turn `min` in 4(b) is a no-op there, since every in-link in 4(b) has `S_i > a C_i`;
  - the tie order among equally restrictive out-links, since the solution is unique.

## 18. M11.3c as implemented (D298)

- **The link model** (`src/meso/link_model.*`, imports nothing):
  - `MesoCurve`: counts at each step, interpolated linearly, with their inverse;
  - `mesoSending` and `mesoReceiving` (Eqs. 4.31 and 4.35), `mesoStepCapacity` and `mesoStorage`;
  - `mesoWaveSpeed` and `mesoLinkTimeLimit` sit in `graph.*`, which `mesoStepLimit` uses.
- **The engine** (`loadMeso`, `src/meso/loading.*`): a commodity per class and path.
  - Per link: entrance and exit counts, and the sending and receiving volume per step, which is §9's
    boundary interface.
  - Per turn: the cumulative transfer.
  - Per commodity: demand, entered and arrived vehicles.
  - A path with no PCU, a repeated link or turns that do not join is refused, not loaded.
- **The node model** accepts an infinite receiving volume, which is the destination column.
- **The advisory:** `MESO_SIGNAL_IGNORED` for a valid signalised Node.
- **Rows:** ME3–ME6, ME8, ME9 and ME24–ME26 in `tests/meso_loading_tests.cpp` (ctest
  `mesoloading`).
  - ME6 reproduces Example 4.5 exactly.
  - ME8 reproduces §7.1: `k_B` 166.67 veh/km; r2m clears at 44 min against the thesis's 46; the r1n
    queue lasts 26.25–82.75 min against 28–82.
  - ME5's shock crosses each piece in 275 s, exactly the theory's figure.
- **Mutations caught:**
  - the receiving lag dropped;
  - the sending lag dropped;
  - Eq. 5.2 replaced by the entrance composition at t (ME25);
  - the step limit back to L/v only (ME24);
  - a finite destination (ME8);
  - the infinite-receiving guard removed (the node model then reads out of range).
