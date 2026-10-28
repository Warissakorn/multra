# PROBLEM — what this is for

> Read this before ARCHITECTURE or ROADMAP. Everything downstream is an answer to
> something on this page. If a proposed feature does not trace back to a line here,
> it is out of scope until this document is changed first.
>
> Amended by D378 (2026-10-27): Micro, Meso and Macro carry equal weight, in the order of the
> study chain; analytical junction capacity on the Node serves all three; Aimsun Next is a
> reference for the integrated model.

---

## 1. Who this is for

Two roles that work on the same road network and the same demand, with equal standing:

- **Study engineers and consultants** — traffic impact studies (TIA, EIA), intersection
  redesigns, signal retiming, development-access and corridor studies. Public road and signal
  agencies do the same work and are served by the same tool.
- **Transport planners** — the study region's trip generation, distribution, mode choice and
  assignment, and the corridor behaviour that follows from it.

The tool is built for Thai and similar Asian practice as well as the general case: left-hand
traffic drives geometry (D7), motorcycles share and filter between lanes (D177), and the
interface is in English and Thai.

A study runs along a chain, each step at the level that answers it (D145, D378):

1. **Macro:** zones, trip rates and the regional network give the site's trips, their
   distribution, mode shares and link volume/capacity.
2. **Meso:** the corridor carries that demand over the peak; queues form and spill back
   across many junctions.
3. **Micro:** the junctions are drawn over an aerial image or a georeferenced OSM background,
   with counted or cut volumes per interval, priority control or signal timing, run over
   several seeds.
4. **At every level:** delay, level of service and queues are reported, averaged across runs
   where a run is stochastic, for the base, with-project and mitigated scenarios.
5. **On the Node:** the junction table by the published analytical method (HCM 7), from the
   same Node, volumes and signal programs (M13, D370).

The engineer may delegate these operations to an AI agent through structured commands
against the compiled desktop application, while watching the same project and simulation
in its native window. Automated edits must remain validated and undoable (D335).

Today this work is done in separate commercial tools: PTV Vissim for microsimulation, PTV
Visum for the four-step model, analytical packages for junction capacity, and integrated
suites such as **Aimsun Next**, which holds every level in one model. These tools work, and
these users know them.

## 2. The problem

**The commercial tools cost more than most of these users can justify, and no affordable tool
gives them the modelling surface and the output their studies require at each level.**

A tool is usable for this work only if the engineer can express the network as they actually
analyse it, and get out the numbers the report needs. The three levels below carry equal
weight (D378); none is only an input to another.

**Micro — the junction in detail**

| What the engineer needs | Why the study needs it |
|---|---|
| **Conflict areas** — pick which movement yields at each individual conflict point | Right-of-way at a real junction is a local engineering judgement, not something to be inferred from a junction type. The engineer must be able to set it and see its effect. |
| **Priority rules** — set gap time and headway in seconds/metres | These are the parameters minor-road delay is tuned with and justified by in a report. |
| **Signal heads anywhere on a link** | Mid-block crossings, staggered stop lines and pre-signals are ordinary study objects. |
| **Wiedemann car-following** | Calibration practice and published parameter sets are expressed in W74/W99 terms. |
| **Desired speed distributions** | Speed belongs to the vehicle and its driver, and every study dialog assumes it. |
| **Motorcycles and lane sharing** — filtering, sharing a lane, forming at the stop line | Thai and much of Asia's counts are motorcycle-heavy; converting them to PCU hides the queue and start-up mechanics a signal study reports (D177; ROADMAP M3.7–M3.8). |
| **Node evaluation** — delay/LOS/queue per movement | The table that goes into the report. |
| **Multi-run averaging with confidence intervals** | A single stochastic run is not a result. |
| **Site circulation and stall-level parking** — internal roads and aisles, parallel and bay stalls, entry and exit manoeuvres including reversing (D381) | Development-access studies ask whether the site's own roads, aisles and stalls work, not only whether the gate queue fits; Vissim users model them as parking spaces with routing. Not started: ROADMAP *Later*. |
| **Pedestrians** — walking areas, crossings and their conflicts with vehicles (D381) | Site access, crosswalk and signal studies need pedestrian delay and its effect on turning traffic; Vissim users do this in Viswalk. Not started: ROADMAP *Later*, as its own two-dimensional model rather than lanes. |

**Meso — the corridor over the peak**

| What the engineer needs | Why the study needs it |
|---|---|
| **Mesoscopic corridor results** — link travel times, queues and spill-back over time | Corridor and work-zone studies need the whole peak over many junctions, not one junction in detail. |
| **Dynamic assignment** among alternative paths | When congestion or a scheme changes travel times, traffic diverts; fixed turning shares cannot show it. |

**Macro — the study region**

| What the engineer needs | Why the study needs it |
|---|---|
| **A four-step macroscopic model** — generation, distribution, mode choice, assignment | Site trip distribution and regional volume/capacity are a study's own results, and the context of the corridor and the junction. |
| **A subarea cut** from the regional model to the finer levels | The corridor and the junction then carry the regional demand without re-entry (M12). |

**Shared by every level**

| What the engineer needs | Why the study needs it |
|---|---|
| **Zones and OD matrices** shared by every level | Trip tables are how site and regional demand is stated; entering them once keeps the levels consistent. |
| **Parking lots** with capacity, dwell time and a gate | Development-access studies ask whether the car park and its access queue fit the site. |
| **Analytical junction capacity on the Node** — HCM 7 capacity, control delay, LOS and queue per lane group | The junction method reports and reviewers expect; it needs no calibration, and it checks simulated delay at any level (M13, D370). |

The reference for holding all of this in one model is Aimsun Next, as its public documentation
describes it: one network database shared by every level; Micro, Meso, hybrid meso–micro and
macro–meso experiments inside scenarios; stochastic route choice and dynamic user equilibrium;
static and dynamic OD adjustment; and traversal matrices that cut a subnetwork's demand out of
a regional assignment ([integration tutorial](https://docs.aimsun.com/next/23.0.0/Tutorials/7_Integration/7_Integration.html),
[hybrid meso–micro](https://docs.aimsun.com/next/24.0.0/UsersManual/HybridSimulator.html),
[vehicle-based simulators](https://docs.aimsun.com/next/24.0.0/UsersManual/VehicleBasedSimulatorsIntro.html),
[OD adjustment](https://docs.aimsun.com/next/24.0.0/UsersManual/ODAdjustment.html); taken from
the pages' search excerpts, as the host could not be reached from the session that wrote this). It is a
modelling surface to learn from, as Vissim is for Micro; no result is compared with it (D38, §5).

## 3. What this project is

**One road network and one demand, analysed at three equally weighted levels — microscopic,
mesoscopic and macroscopic — with analytical junction capacity on the Node beside them, usable
in real engineering work**, with the evaluation output each level's studies require (D145, D378).
The levels are milestones M8–M12 in [`ROADMAP.md`](ROADMAP.md), designed in
[`plans/MULTI_LEVEL_MODELLING.md`](plans/MULTI_LEVEL_MODELLING.md); the Node method is M13.

Its own engines, so that conflict areas, priority rules, signal placement, driver behaviour,
assignment, the corridor model and per-movement evaluation are all first-class inputs rather
than things worked around.

**Work order follows the study chain, Macro → Meso → Micro (D378):** among open work, the coarser
level's next slice comes first, and a finer level's gate does not close before the gate of the
coarser level it depends on (D380). Work at a finer level may continue meanwhile.
The Node method serves all three levels and is not a fourth level.

This is the expensive answer, chosen deliberately. See decision **D1** in
[`PROGRESS.md`](PROGRESS.md) for the reasoning, **D38** for the statement of purpose, **D145**
for the levels and **D378** for their weight.

## 4. What "done" looks like

Each level has its own success sentence, and they carry equal weight:

> **Micro:** a traffic engineer opens an aerial image, draws a four-leg signalized intersection,
> enters counted turning volumes, sets the signal timing, runs 10 seeds, and gets a
> movement-level delay and LOS table they can paste into a report — without opening
> Vissim and without opening Excel.
>
> **Meso:** the engineer runs a corridor of signalised junctions over a two-hour peak and gets
> link travel times and queue lengths by fifteen-minute slice, with spill-back shown.
>
> **Macro:** the engineer enters zones, trip rates and a road network, runs the four-step model,
> and gets the site's trip distribution, mode shares and link volume/capacity, then cuts the
> study area out as an OD matrix for the meso or micro model.

The Node method has its own sentence:

> **Node:** on the same four-leg, with the same volumes and timing, the engineer gets an HCM 7
> lane-group and movement table — capacity, control delay, LOS and queue — in the editor, ready
> to paste into a report.

Milestones M1–M7 are slices of the Micro sentence; M8–M12 of the Meso and Macro sentences; M13
of the Node sentence.

## 5. Non-goals

Stated now so that later sessions do not quietly drift into them:

| Not doing | Why |
|---|---|
| Bit-exact agreement with Vissim, Visum or Aimsun Next | Different code is different code. Every model must be recalibrated to local field data. Claiming parity would be false and would be believed. |
| Activity-based demand, transit assignment, national-scale models | The macro level (D145) is a four-step model for a study's region; anything wider is a different product. |
| 3D presentation | Presentation value only; enormous cost. |
| Multi-user collaborative editing | Needs its own architecture. Not before the single-user tool is good. |
| Importing Vissim `.inpx`, Visum `.ver` or Aimsun `.ang` files | Proprietary, unstable, and would tie the data model to someone else's. Users rebuild networks; that is accepted. |
| Reproducing proprietary formulas (SIDRA, Aimsun or PTV internals) | Published methods only (D370). A reference tool's surface is studied; its unpublished internals are not copied. |
| Being a general-purpose simulation platform | Scope is study work on a road network: the junction, the corridor and the study region. Depth over breadth. |

## 6. Why this can work

The gap is narrower than it looks:

- **Every level is well-documented published science.** Wiedemann 74/99, gap acceptance, lane
  changing (MOBIL, Gipps), signal control, link transmission models, user-equilibrium
  assignment and the four-step model are papers and textbooks, not trade secrets. So is HCM 7.
- **The hard part is not the physics, it is the modelling surface**: the link-based network,
  conflict areas, the object model engineers already know. That is design work, and it has
  already been mapped in detail by the prior effort.
- **One model for every level is practical.** Aimsun Next holds Micro, Meso and Macro on one
  network database; MULTRA's one network and one demand (D145) is the same structure.
- **The evaluation layer has to be written either way**, so it is not extra cost here.
- **Scope is narrow.** The junction, the corridor and the study region for impact and planning
  studies. Not national models, not rail, not freight logistics.

## 7. What would make this project wrong

Recorded honestly, so it can be checked rather than defended:

1. **If an engineer cannot complete a real study with it.** A tool that cannot carry one
   real study from blank network to report table, in reasonable time, is not usable in
   engineering work however good its engines are. Micro was tested with a real study before
   milestone 3 (the M2 gate in `ROADMAP.md`); each other level is tested with its own real
   study or published benchmark before its gate closes (D378).
2. **If fidelity cannot be validated.** An engine nobody trusts is worthless in a report that
   goes to a regulator. Validation against published benchmarks is a gate for every level
   (M6 for Micro; each level's own M6 benchmark), not a nice-to-have.
3. **If the effort of the engines starves the UI.** The prior effort's lesson was that
   almost all of the value was in UX. An excellent engine behind a bad interface loses to
   the commercial tools on day one.

## 8. Open questions

Recorded here so they are decided against this page, not lost in a session queue.

1. **Motorcycles and lane sharing.** The intended users include Thai consultants, whose counts are
   motorcycle-heavy, and motorcycles share and filter between lanes. Nothing in §2 or the
   engine represents that. **Answered 2026-10-15 (D177):** a §2 row and milestones M3.7
   (two-dimensional occupancy) and M3.8 (motorcycles), ahead of the M5.7 rehearsal that R3 had
   waited for.
2. **Hybrid meso–micro simulation.** Aimsun Next runs a Micro area inside a Meso network in one
   run. MULTRA's M12 cuts a subarea and runs each level separately. Is a simultaneous hybrid
   run booked? Open: [OWNER_SITTING](plans/OWNER_SITTING.md) B26 (D378).
