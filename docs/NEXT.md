# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

Rewrite this file each session ([AGENTS](../AGENTS.md#session-end)); owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md), dated measurements in [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-21

What shipped is in [PROGRESS](PROGRESS.md), its [archive](archive/README.md) and the
[decision record](decisions/RECORD.md); the 2026-10-19 state list is
[archived](archive/NEXT-state-2026-10-20.md). Open state only:
- **S2 (Macro, M10.1):** S2.1–S2.4 are in (D271–D277, [MACRO_NETWORK](reference/MACRO_NETWORK.md));
  MA15, the textbook benchmark, is open. §19 (b) named runs and (c) a procedure sequence are booked (D277).
- **S1 is complete** (D266–D270, [STUDY_PERIODS](reference/STUDY_PERIODS.md)); no PCU value ships (D267).
- **Nodes:** S0.1–S0.3 and S0.5–S0.7 are in (D259–D265, D278, D288, [NODES](reference/NODES.md)); S0.4 moved to S3 (D272).
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

## Next session — MA15 when a book is reachable; else Node rows in the batch comparison

S0.7 is in (D288): selecting a Nodes row lights the Node and its legs.
- **MA15 is blocked on access.** On 2026-10-21 the proxy refused both hosts after the owner
  allowed them. That suggests a running container keeps its old policy, but this is unconfirmed.
  - **Try first:** in a new session, fetch both books before any other work.
  - **If both are still refused:** ask the owner to upload a PDF.
  - **The books:**
    - Sheffi, *Urban Transportation Networks* (1985), D272's first candidate:
      https://web.mit.edu/sheffi/www/urbanTransportation.html
    - Boyles, Lownes & Unnikrishnan, *Transportation Network Analysis, Vol. I* (arXiv 2502.05182,
      2025): a newer, free candidate. Its licence and whether it has a small network with a
      published equilibrium are unchecked. Choosing it over Sheffi is a new decision amending D272.
  - **When one is readable,** follow the plan:
    - the Frank–Wolfe chapter's worked example;
    - the evidence note with its citation and the PDF hash;
    - the fixture;
    - MA15;
    - the cost measurement.
- **Otherwise, the S0 item left:** Node rows in the batch comparison. `Comparison` gains Node, leg
  and turn rows, which is a model, output and editor change. It is likely two sessions: the
  `src/runner/` model and outputs, then Results → Nodes (NODES §15, row N21 changes).

Also open: **M3.3.3b, the W99 contract** (ROADMAP M3.3 "Next", D177); the 5 clamps left on M2.6 and
the lane lab's 1. M2.8 has only owner checks left.
