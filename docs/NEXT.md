# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

Rewrite this file each session ([AGENTS](../AGENTS.md#session-end)); owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md), dated measurements in [ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-23

What shipped is in [PROGRESS](PROGRESS.md), its [archive](archive/README.md) and the
[decision record](decisions/RECORD.md); the 2026-10-19 state list is
[archived](archive/NEXT-state-2026-10-20.md). Open state only:
- **M11 (Meso):** the contract is answered (D291, D292: 1a–7a). M11.2 is in (D293: the Meso graph,
  `jamDensity`, `mesoSettings` at schema 52), and so are M11.3a and M11.3b (D295, D296: every Micro
  demand source as Meso paths, with turn shares and demand per class; [MESO_NETWORK](reference/MESO_NETWORK.md)
  §15–§16). The M2.6 template's demand loads whole. M11.3's node model (D297, Tampère et al. 2011)
  and link model with the loading engine (D298, Yperman 2007, `loadMeso`) are in, and so are signals
  as averaged green (D299, choice 4b), M11.4 (D300): `runMeso` over the whole run, outputs,
  `--meso`, and Run Meso in the editor, and M11.4b (D301): a turn's free-flow time as a lag whose
  vehicles count against the out-link's storage.
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
  - HCM 7th edition was used for D299's effective green (Ch. 19, Eq. 19-1).
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

## Next session — S4.4 (Node proposals), or the owner's pick

- **M11 is in through M11.4b** (D291–D302; MESO_NETWORK, MESO_NETWORK_IMPLEMENTED §14–§21). M11.5
  (dynamic assignment) waits on M9.
- **M11 follow-ups, if the owner wants Meso first:**
  - **The M11 gate** (ROADMAP) needs a Meso benchmark before the marker can go (rule 4). B23 is
    answered (D315): Boyles et al. 2025 (M1-d, M2-c) primary, Daganzo 1994/1995 secondary,
    not-blind cases as verification only. Waiting on the owner's copy of Boyles et al. (a Markdown
    repository, or `arxiv.org` allowed; it is blocked here today); then the tolerance (option sheet
    §5) and the benchmark rows.
  - **Priority at a Node** waits on Brilon and Wu (2001) and a cited `t_ij`
    (`MESO_PRIORITY_IGNORED`).
- **Other open items:**
  - **M3.3.3b, the W99 contract** (ROADMAP M3.3 "Next", D177);
  - **the clamps:** the 5 left on M2.6 (pull-out 1, late catch-up 4) and the lane lab's 1;
  - **S3** ([ROADS](reference/ROADS.md), D303, D304): the owner chose 1a–5a, 6b, 7a. S3.2 is in:
    eleven HCM link types and the capacity basis. S3.3 is in (D305): Roads, owned templates,
    `roadId`, *Apply template* and detaching. S3.3b is in (D307): the Road tool (O), the Roads and
    Cross-sections tabs and the detached display. S3.4 is in (D308): *Join opposite Links* and
    *Assign template*. S3.5 is in (D309): *Find crossings*, overpasses, junction splits and
    *Merge nodes*. S3.6a is in (D310): the link-type `rank`, `coarse` on Nodes without detail
    with its checks, and Macro and Meso compiling coarse Nodes and Link-less Roads
    ([ROADS_IMPLEMENTED](reference/ROADS_IMPLEMENTED.md) §19). S3.6b is in (D311): the *Coarse
    values…* dialog, refine, derive back and the two-leg lifecycle, with detail revised to the
    junction's interior and Link legs compiled (§20).
    S3.7 is in (D312): seam checks on demand, with *Use Link* / *Use Node* (§21). **S3 is complete.**
    Booked: demand on a Road-only network (zone connectors attach to Links only);
  - **S4** ([READINESS](reference/READINESS.md)): the owner chose S4-1a–S4-4a (D313). S4.2 is in
    (D314): the Problems tab checks Micro, Meso or Macro with its period, from the runs' own
    refusal functions, plus seams. S4.3 is in (D316): *Fill defaults* F1–F3, listed with
    sources and applied only when ticked. **Next, S4.4:** Node proposals after a run (Macro v/c
    ≥ 1.0 or Meso spill-back on a Node's leg; a link end without a Node offers *Suggest nodes*;
    a stale result proposes nothing; RS9, RS10). F4 (a coarse Node's Micro detail) is its own later
    slice;
  - **E1 is in (D306):** tool pointers with glyphs, plain clicks after a draft's first point, solid
    previews. The owner checks the glyphs and the click rule on Windows;
  - **S2.4's booked follow-ups:** §19 (b) named runs or (c) a procedure sequence (D277);
  - **B21:** Sheffi Fig 5.1's capacities, for the owner.
  - **Boyles et al. 2025:** the copy for the Meso benchmark, for the owner (D315).

M2.8 has only owner checks left.
