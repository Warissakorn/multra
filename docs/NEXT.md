# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

Rewrite this file each session ([AGENTS](../AGENTS.md#session-end)); owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md), dated measurements in [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-22

What shipped is in [PROGRESS](PROGRESS.md), its [archive](archive/README.md) and the
[decision record](decisions/RECORD.md); the 2026-10-19 state list is
[archived](archive/NEXT-state-2026-10-20.md). Open state only:
- **M11 (Meso):** the contract is answered (D291, D292: 1a–7a). M11.2 is in (D293: the Meso graph,
  `jamDensity`, `mesoSettings` at schema 52), and so are M11.3a and M11.3b (D295, D296: every Micro
  demand source as Meso paths, with turn shares and demand per class; [MESO_NETWORK](reference/MESO_NETWORK.md)
  §15–§16). The M2.6 template's demand loads whole. M11.3's node model (D297, Tampère et al. 2011)
  and link model with the loading engine (D298, Yperman 2007, `loadMeso`) are in; signals are not
  applied yet.
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
  - `Warissakorn/the-link-transmission-model-for-dynamic-network-loading-markdown`: Yperman (2007),
    used for D298. It has no page numbers: cite sections and equations.
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

## Next session — M11.3d: signals in Meso (needs the owner's choice), then M11.4

- **In:** the link model and `loadMeso` (D298, MESO_NETWORK §4, §18), checked against Yperman's
  Example 4.5 and §7.1. Signalised Nodes are advised `MESO_SIGNAL_IGNORED` and load as unsignalised.
- **The choice to put to the owner first** (AskUserQuestion; the reasons are in D298 and §5):
  - **(a) Averaged green:** turn capacity `g/C · s`, Yperman Eq. 5.13 and Tampère Eqs. 33–36. Cited,
    and FIFO stays exact, but there is no queue cycle, so ME7's cycle clause changes (choice 4a → 4b).
  - **(b) Explicit green with each turn queued separately at the link end:** a point queue per turn,
    in the spirit of the thesis's ch. 6 movement queues. This keeps 4a, but the separate queues break
    the link FIFO that Eq. 5.2 relies on, so it needs its own design.
  - **(c) Explicit green under strict FIFO:** any red turn holds the in-link. Simple and cited by CTF,
    but far too pessimistic where turns have their own lanes.
- **Then M11.4:** the outputs (§7: link and turn rows by slice from `MesoLoading`), `--meso PERIOD`,
  and the editor surface (ME10, ME11). The run horizon (the demand's end plus clearance) is chosen
  there.
- **Priority at a Node** still waits on Brilon and Wu (2001) and a cited `t_ij`
  (`MESO_PRIORITY_IGNORED`).
- **Other open items,** if the owner prefers one before Meso:
  - **M3.3.3b, the W99 contract** (ROADMAP M3.3 "Next", D177);
  - **the clamps:** the 5 left on M2.6 (pull-out 1, late catch-up 4) and the lane lab's 1;
  - **S3:** Roads and S0.4's coarse values (D272);
  - **S2.4's booked follow-ups:** §19 (b) named runs or (c) a procedure sequence (D277);
  - **B21:** Sheffi Fig 5.1's capacities, for the owner.

M2.8 has only owner checks left.
