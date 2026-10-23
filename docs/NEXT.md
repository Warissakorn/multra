# NEXT — what a session does first

The single live to-do, **open items only**: what shipped is in [PROGRESS](PROGRESS.md), the
[archive](archive/README.md) and the [decision record](decisions/RECORD.md); gates are in
[ROADMAP](ROADMAP.md); owner items only in [OWNER_SITTING](plans/OWNER_SITTING.md). Rewrite this file
each session ([AGENTS](../AGENTS.md#session-end)); never append a finished slice here (D319). The
previous full list is [archived](archive/NEXT-state-2026-10-23.md).

## Standing facts

- **Name:** MULTRA (D284, D287); only the file's `"format": "TrafficSim"` stays until a schema step
  reads both. Licence GPL-3.0-or-later (D285); a CLA comes before any outside contribution.
- **Evidence is Linux only** (offscreen Qt 6.4.2); no Windows or high-DPI claim. No session closes a
  milestone gate.
- **Books** (owner's Markdown copies; attach with `add_repo`, cite book pages, not the copy's prose):
  `Warissakorn/Urban-Transportation-Networks-Markdown` (Sheffi 1985),
  `Warissakorn/Highway-Capacity-Manual-7edition-Markdown` (HCM 7),
  `Warissakorn/Modelling-Transport-4-edition-Markdown` (Ortúzar & Willumsen),
  `Warissakorn/Transportation-Planning-Handbook-Markdown`,
  `Warissakorn/the-link-transmission-model-for-dynamic-network-loading-markdown` (Yperman 2007; no
  page numbers, cite sections and equations).
- **Before an engine slice:** `src/core/` files are at the 500-line limit; split along the
  responsibility a change touches first (D319).
- **New dialogs** are free functions with their own file (as `behaviour_editor.cpp`), not new
  `EditorWindow` members, and one whose rows can grow uses `dialog_fit` (D318, D319).
- A crashed local UI run leaves `~/.qttest/share/<suite>/recovery`; delete it before re-running,
  or the suite waits at the recovery dialog. If `map-ui` fails in a parallel `check`, capture its
  output before re-running.

## Next session — the owner's pick: F4 (S4.5), S5, or an item below

- **F4 (S4.5):** a coarse Node's Micro detail; needs its own contract section
  ([READINESS](reference/READINESS.md)). **S5:** the study area and stale marking.
- **Lateral, before any shipped behaviour carries its keys**
  ([lateral-run-audit](evidence/lateral-run-audit.md)): on `m2.6-study-template` a changer past its
  window straddles the line and jams both lanes (seed 45: 1908 → 1531 completed; clamps 1 → 33).
  LANE_CHANGE_DURATION §9's reopen condition is met; abandoning a change is the owner's call. The
  switched-on cost (+10–28 % wall) is not worth cutting until then.
- **Meso (M11, in through M11.4b):** the gate needs the benchmark: waiting on the owner's copy of
  Boyles et al. 2025 (D315), then the tolerance and rows. Priority at a Node waits on Brilon and Wu
  (2001) and a cited `t_ij`. M11.5 waits on M9.
- **Macro (S2):** B21, Sheffi Fig 5.1's capacities, for the owner; §19 (b) named runs or (c) a
  procedure sequence are booked (D277).
- **Roads:** demand on a Road-only network (zone connectors attach to Links only) is booked.
- **M3.3.3b**, the W99 contract (ROADMAP M3.3, D177).
- **Clamps:** 5 left on M2.6 (pull-out 1, late catch-up 4), the lane lab's 1 (D252) and source
  clearance (D108); `joinAnticipation` ships off (D255); D95 stays off.
- **Engine spread costs, not booked:** `closestVehicle` 8 %, per-tick `Vehicle` moves 6 % (D283).
- **Owner checks on Windows:** D318's dialogs at 1366×768 or 150 %; E1's tool glyphs and click rule
  (D306); paint, OWNER_SITTING E22. M2.8 has only owner checks left.
