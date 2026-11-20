# Owner sitting, part 2 — the Windows machine

Part 2 of [OWNER_SITTING](OWNER_SITTING.md); its rules, build and screen settings apply here. Triaged
2026-11-20 by [D602](../decisions/RECORD.md#d602): what stays here needs the owner's eyes on Windows.
Older looks were carved out to [OWNER_LOOKS_CARVED](OWNER_LOOKS_CARVED.md) (ROADMAP M1.30, M3.2.13, M14) (full steps:
`git log -p` on this file before D602); I.1, I.3, I.4, F2, D2 and E22 closed. Item names are unchanged
and never reused; an item that took in an older one says so.

**Order for one sitting** (gates and blockers first): OWNER_SITTING A2 (M3.2.7d attempt 2, steps 1,
3–4 and the verdict, D601) → E32 → C15 and E33 → E34 → C14 → E35 → E36 → OWNER_SITTING B45, B46 with
item 42, B48 and item 50 → C16 → C17, C18 → I.5 → G1 → H1 → C12 (optional).

---

## C — Run view (the 125 % screen)

14. **Run view after D463/D487** (listed 2026-11-07). On Windows, select objects of each kind: does the
    Properties summary fit its panel? Run, Pause and Step a few times: does the Run/Pause button keep
    its size and the layout stay still? (`run-presentation-ui` guards the relayout on Windows CI too;
    a failure there only is a Windows LayoutRequest source.) *Owner:*

15. **Run vehicles, slice C** (listed 2026-11-04, D465/D466; images `docs/images/canvas-run-d465/`,
    `docs/images/canvas-heads-d466/`). Run four-leg at 100/150/200 %: do the vehicles, their heads
    and casings read as in the images? Slice D (lane arrows, white stop line) starts only on your go. *Owner:*

16. **Random streams** (listed 2026-11-12, D510). *Run settings → Random streams*: does the list show
    `shared` and `perInput`, a new project `perInput`, and a shipped project `shared`? Compare a
    `perInput` project with a `shared` one: is the refusal line on the Compare page clear? *Owner:*

17. **Dynamic assignment page** (listed 2026-11-13, D552, DYNAMIC_ASSIGNMENT §17). Open a project with a
    routing decision. In *Results → Micro → Dynamic assignment*, Run at Micro, then at Meso (the seed is greyed).
    Check four things: the δ and cell tables fit; Cancel stops; *Apply weights* is one Undo step and greys after any
    edit; *Save route costs…* writes a file. Is the page where you expect it, and is the wording clear in
    Thai? *Owner:*

18. **Consistency page** (listed 2026-11-14, D554, SUBAREA_CONSISTENCY §13). Open a project with a study area cut from
    Macro. In *Results → Cross-level → Consistency*, Run with seeds 42-46. Check four things: the table fits; Cancel
    stops; an edit marks the result stale; Export writes the CSV. Is the level's name clear in Thai, and is a table
    with no pass/fail colours what you want to read? *Owner:*

12. **MSVC Release timing** (listed 2026-10-14, D163/D164). On P-cores, the M2.6 CLI run: the
   compile figure (≈116 ms before D163) and the wall time (before D164). Record both numbers; a
   session cannot measure this machine. *Owner:*
    Also (D597, D598): MSVC Release wall time of the Ratchada study and of
    `multra-engine-benchmark 48 900`, before and after; Linux callgrind showed −32.9 % Ir on the benchmark.

## E — Drawing and gestures (100/150/200 %)

32. **A9 Fill from model / Micro side by side** (listed 2026-10-31, D421/D422). On Windows at
    100/150/200 %, Thai and English, on both driving sides: open Nodes → Analysis… → Fill from
    model…, choose explicit times or StudyPeriod and HV types, inspect before/after/T/PHF and
    source SHA, Cancel then Confirm (one Undo). Check drawn turn/approach Clear keeps the row
    visible. Run seeds, wait, Analyse → Micro side by side: read distinct HCM/Micro labels,
    source/window/seeds, n/CI/unfinished, blank values and localized refusal text. Change inputs,
    Undo, cancel a batch, reopen, and Export/Copy: old comparisons must disappear and the CSV
    marker/quoted numbers must remain. For manual counts, select a Node and explicitly confirm its
    window; mismatched window/T disables Confirm, Cancel is unchanged, Manual counts labels and
    CSV countsSource are clear. Re-Analyse/reopen requires confirmation again and Save adds no
    field. Are tables, notes, scrollbars and fixed footers readable without clipping? *Owner:*

33. **Editor fixes D427–D431** (listed 2026-11-01). On Windows at 100/150/200 %, Thai and English:
    run four-leg and drag the view while it runs. Do the roads stay under the vehicles (D427)?
    Snap starts off (D428). Make a run fail and check the message line is one row, with
    "and N more", a tooltip and an × button (D429). Zoom in and out on dashed lane lines:
    do the 3 m dashes keep their length (D430)? Drag the network view by its marker line to another
    side, double-click it to float it onto a second screen, use it, resize the window, then View →
    Reset panel layout (D431). *Owner:*

34. **UI audit remediation D432** (listed 2026-11-02). On Windows at 100/150/200 %, English and
    Thai, open four-leg. At 1366×768 and 1024×768 switch View → Workspace between Drawing,
    Tables and Results; at 800×600 expand Appearance and scroll Properties. Choose all three
    UI densities, type seed 4294967295, inspect the named/icons palette and movement/queue
    names, then open Behaviour Lab. Fit and pan the network: do labels stay inside the view,
    clear of the HUD, with all twelve input rates after Fit at 1024×768? Submit an invalid
    Input interval and correct it in the same form; reopen a second-day period without editing.
    Are text, fields, row selection, scrolling and fixed dialog buttons usable without clipping?
    Linux coverage and exact repro commands: [ui-audit-fixes](../evidence/ui-audit-fixes.md). *Owner:*
    *Takes in F4 (D318, [dialog-fit](../evidence/dialog-fit.md)):* at 1366×768 and 150 %, are OK/Cancel of
    every growing dialog reachable without moving the window?

35. **Tables like Excel** (listed 2026-11-07, D490/D490a). Paste a Links block and a Results table
    into Excel; check the grid and row header at 100 % and high DPI; open and fold a Results note;
    check the Tables layout's height and the Thai group picker. Does each paste land as cells? *Owner:*
    *Takes in C1, C7 and E18:* Results → Run seeds and Compare with… (M2.6 template, seeds 42-51): Cancel
    part way, run again, Copy, Export; Thai strings; does the comparison read as alternative minus base?
    Demand catalog, Behaviour library and an OD matrix: figures right-aligned and unclipped, OD rows tall
    enough to edit, the full-text popup on a cut-off name helpful?

36. **Route tool and marks** (listed 2026-11-11, D507–D509d). Press `R`, click a Link once, then two
    or three destinations: is the status hint clear and each destination one Undo; does Esc end the
    origin? Hold `Ctrl`+right on a road and move: does the green (origin) or red (destination) bar
    follow until release? Check the thin end bars beside inputs, decisions and signal heads, the
    yellow 50 % tint between the bars and its 0.6 m inset at the road edges. *Owner:*
    *Takes in E5 and E8:* a Route 2-lane Link → 1-lane Connector → 2-lane Link, end-mounted and mid-body:
    both lanes shown, nothing upstream of a mid-body arrival; Undo/Redo, Backspace, save/reopen.

## G — Object interchange

1. **M1.23.1 object table CSV** (listed 2026-10-27, D365). On packaged Windows, open a project,
   make an unsaved name edit containing Thai text, a comma and quotes, then use File → Export
   object table (CSV) for Links, Connectors, inputs, routes and decisions. Do Unicode filenames
   and nested JSON cells open correctly in the study's spreadsheet/CSV reader? Check an empty
   draft, Cancel, a failed destination and refusal to overwrite this window's project/recovery;
   confirm no dirty/revision/Undo or Run state changes. Does a name starting with `=` open as
   text with one leading `'` (D368)? *Owner:*
    *Takes in C2 (D104):* Results → Export results (CSV) after a run: does it open in a spreadsheet,
    marker line first, and is it greyed before the end?

## H — Desktop automation on Windows

1. **D335/D336 desktop automation** (listed 2026-10-25,
   [DESKTOP_AUTOMATION](../reference/DESKTOP_AUTOMATION.md)). On a newly packaged build, run its
   PowerShell workflow: control the visible editor, watch playback, patch and Undo a Link,
   save/reopen and capture a screenshot. Is a duplicate endpoint refused, and do Unicode file
   paths work? From a second Windows account, does a pipe of the same name give
   `UNTRUSTED_SERVER`? *Owner:*

## I — UTDF probe (M14 U0)

The junction, the steps and the answer table are on [UTDF_PROBE](UTDF_PROBE.md); write each answer there
(§6) verbatim and dated. I.1, I.3 and I.4 are answered (D602); the golden `u0-vistro.csv` is committed.

5. **First MULTRA-written file (UT10, D442, D443).** Import `tests/data/utdf/u2-u0-junction-geometric.csv` into
   Vistro: do geometry, volumes and timing match §1–§3 (as v3 did); are the empty `[Network]` cells accepted? *Owner:*

2. **Synchro import** (listed 2026-10-28; revised by D438). Can Synchro import UTDF? If so, import
   `u0-vistro.csv`. Does it take the lanes, pockets, volumes and timing as drawn? *Owner:* Optional.
