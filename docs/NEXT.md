# NEXT — what a session does first

The single live to-do, **open items only**: what shipped is in [PROGRESS](PROGRESS.md), the
[archive](archive/README.md) and the [decision record](decisions/RECORD.md); gates are in
[ROADMAP](ROADMAP.md); owner items only in [OWNER_SITTING](plans/OWNER_SITTING.md). Rewrite this file
each session ([AGENTS](../AGENTS.md#session-end)); never append a finished slice here (D319). The
previous full list is [archived](archive/NEXT-state-2026-10-23.md).

## Standing facts

- **Name:** MULTRA (D284, D287); only the file's `"format": "TrafficSim"` stays until a schema step
  reads both. Licence GPL-3.0-or-later (D285); a CLA comes before any outside contribution.
- **Evidence is Linux only** (offscreen Qt; session versions in PROGRESS); no Windows or high-DPI claim. No session closes a
  milestone gate.
- **Books:** five owner copies (Sheffi, HCM 7, Ortúzar & Willumsen, Planning Handbook 4th ed.,
  Yperman); repositories, page rules, caveats and the chapter map by milestone are in
  [BOOKS](reference/BOOKS.md). Attach with `add_repo`, never vendor; cite the book, read the image
  before using a number. Checks of 2026-10-23: [book-copies-check](evidence/book-copies-check.md).
- **Before an engine slice:** `src/core/` files are at the 500-line limit; split along the
  responsibility a change touches first (D319). The owner deferred splitting `types.hpp` on its
  own: estimated 466 → 420 objects rebuilt after a runtime-state edit, 338 with `eventJson`,
  `checkpointJson` and `summaryJson` moved out of `project/json.hpp`. Do it with the first engine
  slice that edits `Vehicle` or `SimState`, and measure the rebuild before and after.
- **New dialogs** are free functions with their own file (as `behaviour_editor.cpp`), not new
  `EditorWindow` members, and one whose rows can grow uses `dialog_fit` (D318, D319).
- If `map-ui` fails in a parallel `check`, capture its output before re-running.
- **Shipped projects (D337):** all four stay; [their README](../data/projects/README.md) names each
  one's purpose and builder. `four-leg-signalised` now carries a 300 s cool-down (schema 27): a test
  that needs the bare drawing clears it with `changeEvaluationPeriod(d, 0, std::nullopt, 0)`.

## Next session — the owner's pick (S5 is complete, D327–D332)

- **Open from S5:** restoring the full Macro or Meso table on reopen would need a new owner
  choice: the owner chose the cut source only (D331). The stored source and freshness line remain
  the reopen contract.
- **Other picks:**
  - **Desktop automation (D335):** on a newly packaged Windows build, run the PowerShell
    workflow in [DESKTOP_AUTOMATION](reference/DESKTOP_AUTOMATION.md): control the visible
    editor, watch playback, patch/Undo a Link, save/reopen and capture a screenshot. Check
    duplicate endpoint refusal and Unicode file paths. D336's server-owner check needs a second
    Windows account: a pipe of the same name created by it must give `UNTRUSTED_SERVER`.
    Local evidence is Linux only.
  - **Vehicle library/display (D333):** owner appearance check on Windows/high DPI for the
    seven types at working and overview zoom; illustrative type parameters still need study-specific calibration.
    D334 adds fixed/mixed colour preferences: include colour editing, restart and lab appearance.
  - **OSM (M1.28):** MAP10 still needs the owner's real-study check: align the background,
    trace roads, pan/click without a camera jump, zoom out to the overview, choose a country
    with and without a province, interrupt/restore the connection and retry, then save/reopen.
    Check Windows and the packaged TLS/PROJ dependencies; Linux
    local-server tests do not establish those results. See [MAP_BACKGROUND](reference/MAP_BACKGROUND.md).
- **Lateral, before any shipped behaviour carries its keys**
  ([lateral-run-audit](evidence/lateral-run-audit.md)): on `m2.6-study-template` a changer past its
  window straddles the line and jams both lanes (seed 45: 1908 → 1531 completed; clamps 1 → 33).
  LANE_CHANGE_DURATION §9's reopen condition is met; abandoning a change is the owner's call. The
  switched-on cost (+10–28 % wall) is not worth cutting until then.
- **Meso (M11, in through M11.4b):** the gate needs the benchmark: waiting on the owner's copy of
  Boyles et al. 2025 (D315), then the tolerance and rows. Priority at a Node waits on Brilon and Wu
  (2001) and a cited `t_ij`; HCM 7 Ch. 20's headways are a citable candidate (BOOKS §4). M11.5
  waits on M9, whose two-route fixture has a published candidate (Ortúzar Example 11.1).
- **Macro (S2):** B21, Sheffi Fig 5.1's capacities, for the owner; §19 (b) named runs or (c) a
  procedure sequence are booked (D277).
- **Roads:** demand on a Road-only network (zone connectors attach to Links only) is booked.
- **M3.3.3b**, the W99 contract (ROADMAP M3.3, D177).
- **Clamps:** 5 left on M2.6 (pull-out 1, late catch-up 4), the lane lab's 1 (D252) and source
  clearance (D108); `joinAnticipation` ships off (D255); D95 stays off.
- **Engine spread costs, not booked:** `closestVehicle` 8 %, per-tick `Vehicle` moves 6 % (D283).
- **Owner checks on Windows:** D318's dialogs at 1366×768 or 150 %; E1's tool glyphs and click rule
  (D306); paint, OWNER_SITTING E22. M2.8 has only owner checks left.
