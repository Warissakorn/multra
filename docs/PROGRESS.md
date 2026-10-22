# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-10-22 — M11.3c: Yperman's link model and the loading engine (D298)

The owner supplied Yperman's thesis as a Markdown repository, and its chapters 3–5 and §7.1 were read.
- **Confirmed:** §4's sending and receiving volumes, the storage, the origin queue and the
  destination. The thesis prints only `L/v` as the step limit; `L/w` is added, derived.
- **Owner choices:**
  - turn splits follow the vehicles' routes (Eq. 5.2), replacing shares at time t;
  - signals move to M11.3d, because explicit green under FIFO holds the whole in-link. Signalised
    Nodes are advised `MESO_SIGNAL_IGNORED`.
- **`loadMeso`** joins the link model and `mesoNodeFlows` per step, with spill-back and the
  boundary volumes.
- **Against the thesis:** Example 4.5 is exact. §7.1's diverge network gives `k_B` = 166.67 veh/km,
  r2m clear at 44 min (thesis 46) and r1n queued 26.25–82.75 min (thesis 28–82).
- **Rows:** ME3–ME6, ME8, ME9 and ME24–ME26 in `mesoloading`. Six mutations are caught.

## 2026-10-22 — M11.3's node model from Tampère et al. (2011) (D297)

The owner uploaded the paper, and it was read in full.
- **`mesoNodeFlows`** is its §5.1 instance and algorithm: oriented capacity proportional, exact,
  and unique. It reproduces the paper's Table 8.
- **Changed from the contract:** the paper has no instance of conflict constraints (its Eq. 17 is
  provisional). By the owner's choice, priority inside a Node is advised `MESO_PRIORITY_IGNORED`
  instead of guessed.
- **Signals** are explicit green, as the paper's §5.2 allows.
- **Rows:** ME20–ME23 in `mesonode`. Three mutations are caught; two had no effect, and both are
  explained.
- **Still blocked:** the link model needs Yperman (B22).

## 2026-10-22 — M11.3 blocked on its sources; nothing built

The owner asked for M11.3. The contract makes checking §4 against Yperman (2007) and §5 against
Tampère et al. (2011) the first step, but the environment refused every host that holds them
(proxy 403: arxiv.org, lirias.kuleuven.be, ideas.repec.org). The owner chose to wait for the sources,
so no engine code was written. OWNER_SITTING B22 asks for the hosts or the PDFs.

## 2026-10-22 — M11.3b: routeless inputs and placed decisions as Meso paths (D296)

The owner asked for M11.3b. Meso now takes Micro's own expansion of routeless inputs and placed
decisions: `expandRouteless` after `resolveCatalogs`, so lane splits, slice shares and typed routing
are not copied.
- **Lanes to objects:** `mesoObjectChain` maps lane sections and connector paths back to Links and
  Connectors, then `mesoPath` as in M11.3a.
- **Stubs:** a stub's flow goes to the full routes its vehicles change lanes onto, because the trip
  does not end there.
- **Type parts:** a composition's type parts sum back into its class.
- **The M2.6 template** (four placed decisions) now loads its whole demand, conserved over time.
- **Rows:** ME17–ME19 in `mesodemand`. ME13 now checks the walk's own issues instead of the old
  refusal. Four mutations are caught.

## 2026-10-22 — M11.3a: Micro's routes as Meso paths and demand (D295)

The owner asked for M11.3a after MA15.
- **`src/meso/demand.*`:** paths, classes, turn shares and entry flows, as plain values.
- **`mesoPath`:** a route's Links and Connectors become Meso links and turns. Pieces inside a Node
  collapse to the Node's turn.
- **`mesoDemand`:** expands the document as the Micro compile does (OD matrices, then static
  decisions with their interval and type weights). It keys classes by composition or lone vehicle
  type, with PCU from `modePcu`.
- **Carved out as M11.3b:** placed decisions and routeless inputs walk lanes, not objects, so they
  are refused with `MESO_ROUTELESS_UNSUPPORTED` until then. The M2.6 template uses them.
- **Rows:** ME12–ME16 in `mesodemand`. Five mutations are caught. A turn preference by `via` was
  removed as dead code, since the graph keeps one turn per (in, out) pair.

## 2026-10-22 — MA15: Sheffi's Table 5.4 as the Macro benchmark (D294)

The owner supplied Markdown copies of four books, among them Sheffi (1985), which was D272's first
candidate. This unblocked MA15.
- **The example:** Table 5.4, Frank–Wolfe on Figure 5.1. It has three parallel BPR links, t₀ =
  (10, 20, 25) and q = 10.
- **Derived capacities:** the copy's image of book p. 114 is empty, so the capacities were derived.
  Table 5.1's 947 and 137 give c₁ = 2 and c₂ = 4. c₃ = 3 reproduces every printed number, apart from
  two misprints. The owner confirms them against the print (OWNER_SITTING B21).
- **Row:** `macrobenchmark` holds MA15 (a) and (b).
  - (a) Runs capped at 1–5 iterations match the printed flows, times, objective and steps.
  - (b) At the 10⁻⁴ target, the run reaches the equal-time equilibrium in 13 iterations, with
    z = 189.33.
  - The engine was unchanged.
- **Cost:** 80 µs and 0.47 M Ir per run (ENGINEERING_NOTES).
- **The M10 gate stays open:** closing it is the owner's call, and the not-validated marker stays
  until M6.

## 2026-10-21 — M11.2: the Meso graph (D292 answers, D293)

The owner answered the Meso choices "เอาตามแนะนำ": 1a–7a (D292).
- **New module `src/meso/`** (imports nothing): links, turns and nodes, the settings and the step
  limit.
- **`compileMesoNetwork`:** each Node becomes one node with its membership turns and conflicts; the
  pieces inside a Node are left out; other junctions become simple nodes with through and merged
  Connector turns. A refused Node is reported and compiled as if it were not drawn.
- **File:** a link type's `jamDensity` (pcu/km/lane, above the critical density) and
  `mesoSettings{step, slice}` at schema 52, only when set; the shipped projects keep their schema.
- **Tests:** ME1–ME2 in `mesonetwork`. Six mutations are caught, two of them only after the rows
  gained fixtures (two lane Connectors; a refused Node over the junction).

## 2026-10-21 — M11.1: the Meso contract (D291, docs only)

The owner asked whether Meso could start. M11.1 can, because everything it needs exists; only M11.5
(dynamic assignment) waits on M9. [MESO_NETWORK](reference/MESO_NETWORK.md) fixes the following:
- **The graph:** the Macro graph's Link pieces, with each Node as one Meso node.
- **The link model:** a triangular diagram with Newell's cumulative curves and Yperman's sending and
  receiving volumes.
- **The node model:** Tampère et al. (2011), with signals as capacity and spill-back.
- **Data:** a `jamDensity` per link type, none shipped.
- **Loading:** deterministic.
- **Outputs:** by time slice, with rows ME1–ME11.

Seven owner choices are listed as B20. Sources came from search summaries; Yperman's thesis was not
found, so §4's formulas are checked before M11.3.

## 2026-10-21 — S0.8b: a comparison's Node rows in Results → Nodes (D290); S0 complete

The editor half of D289. MA15 is still blocked (sheffi.mit.edu and arxiv.org refused again).
- **Results → Nodes, for a comparison:** the Node, leg and turn rows of `Comparison::nodes`: two
  sides, difference and ±95 % for delay, plus the legs' queue. There is no letter on a difference.
  Unmatched names go in the note.
- **Tests:** N21 rewritten. A comparison with itself let a column swap survive, so it now compares a
  busier copy; four mutations each fail it.
- **S0 is complete** but S0.4, which moved to S3 (D272).

## 2026-10-21 — S0.8a: Node rows in the scenario comparison (D289)

MA15 is still blocked (sheffi.mit.edu, web.mit.edu and arxiv.org refused), so the S0 item left,
split per AGENTS into the model and output now and the editor table next.
- **Model:** `compareBatches` matches Nodes, then their in-legs and turns, by name; Welch
  differences of delay, and of the legs' mean queue. Unmatched names are listed per level.
- **Output:** JSON `nodes` and two CSV blocks after `network`, only with a Node; every comparison
  without one keeps its bytes. The editor's Copy and Export carry the blocks.
- **Tests:** CMP8–CMP10 in `compare`; five mutations each fail them.
- **Found while checking sources:** Aimsun's manual cites Florian & Hearn (1995), and Visum names
  Mitradjieva, Lindberg et al. (2013) for BFW and Gentile for LUCE. Neither is a benchmark network.
  Sheffi's PDF is also at sheffi.mit.edu.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
