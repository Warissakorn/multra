# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

Rewrite this file each session ([AGENTS](../AGENTS.md#session-end)); owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md), dated measurements in [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-22

What shipped is in [PROGRESS](PROGRESS.md), its [archive](archive/README.md) and the
[decision record](decisions/RECORD.md); the 2026-10-19 state list is
[archived](archive/NEXT-state-2026-10-20.md). Open state only:
- **M11 (Meso):** the contract is answered (D291, D292: 1a–7a) and M11.2 is in (D293: the Meso graph,
  `jamDensity`, `mesoSettings` at schema 52; [MESO_NETWORK](reference/MESO_NETWORK.md) §14).
- **S2 (Macro, M10.1):** S2.1–S2.4 are in (D271–D277, [MACRO_NETWORK](reference/MACRO_NETWORK.md)),
  and so is MA15, the textbook benchmark: Sheffi Table 5.4 (D294). Its derived capacities and the
  gate wait on the owner (OWNER_SITTING B21). §19 (b) named runs and (c) a procedure sequence are
  booked (D277).
- **Books:** the owner's Markdown copies are on GitHub. Attach one with `add_repo` and cite book
  pages, not the copy's prose. Page images can be empty: Sheffi's p. 114 is 0 bytes.
  - `Warissakorn/Urban-Transportation-Networks-Markdown`: Sheffi (1985), used for MA15.
  - `Warissakorn/Highway-Capacity-Manual-7edition-Markdown`: HCM 7th edition, for S3's coarse
    capacities and a link type's `source`.
  - `Warissakorn/Modelling-Transport-4-edition-Markdown`: Ortúzar & Willumsen, for M10.2–M10.4
    (generation, distribution, mode choice).
  - `Warissakorn/Transportation-Planning-Handbook-Markdown`: the Transportation Planning Handbook (edition unchecked).
- **S1 is complete** (D266–D270, [STUDY_PERIODS](reference/STUDY_PERIODS.md)); no PCU value ships (D267).
- **Nodes: S0 is complete** (D259–D265, D278, D288–D290, [NODES](reference/NODES.md)); S0.4 moved to S3 (D272).
- **Clamps:** 5 left on M2.6 (pull-out 1, late catch-up 4), the lane lab's 1 (D252) and source
  clearance (D108); the gate stays open. `joinAnticipation` ships off (D255); D95 stays off.
- **The optimization pass is complete** (D279–D283, 2026-10-20, Linux): a Node adds +14–20 % Ir;
  M2.6 run view −38 % per Step; mandatory reads −31 %. Windows paint is OWNER_SITTING E22. Not
  booked: the engine's spread costs (`closestVehicle` 8 %, the per-tick `Vehicle` moves 6 %).
- If `map-ui` fails in a parallel `check` again, capture its output before re-running.
- **Name:** MULTRA (D284). The repository is `Warissakorn/multra`; executables, the `multra`
  namespace, every target and `MULTRA_*` options follow (D287). Only the file's
  `"format": "TrafficSim"` stays, until a schema step reads both.
  Domains and trademarks are unchecked. Licence GPL-3.0-or-later (D285); a CLA comes before any
  outside contribution.
- Local evidence is Linux (offscreen Qt 6.4.2); no Windows or high-DPI verification is claimed.
  No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — M11.3a (Meso paths and demand)

- **M11.3a:** Micro's routes and routing-decision weights become Meso paths (D292 5a).
  - Map each route's Links and Connectors to `MesoGraph` links and turns. The pieces inside a Node
    collapse to the Node's turn; a Connector outside one is a simple node's turn.
  - Turn shares at each node per class, from the routes' weights and their intervals.
  - The study period's demand per class in veh/h (constant, or by the matrix's intervals).
  - Rows ME12 and later in MESO_NETWORK §13: a route that cannot be mapped is refused, with its
    path. Write the rows first.
  - Code: `src/project/meso_input.*` beside `mesoNetwork`; the route compile is in
    `src/project/demand_paths.*` and `src/model/network/routing.*`.
- **Before M11.3 (the engine):** check §4's formulas and step limit against Yperman's thesis (try
  Lirias, KU Leuven), and §5 against Tampère et al. (2011)'s requirements. Neither is in the
  owner's book repositories; ask for a PDF if the hosts are refused.
- **Other open items,** if the owner prefers one before Meso:
  - **M3.3.3b, the W99 contract** (ROADMAP M3.3 "Next", D177);
  - **the clamps:** the 5 left on M2.6 (pull-out 1, late catch-up 4) and the lane lab's 1;
  - **S3:** Roads and S0.4's coarse values (D272);
  - **S2.4's booked follow-ups:** §19 (b) named runs or (c) a procedure sequence (D277).

M2.8 has only owner checks left.
