# Owner sitting, part 2 — the Windows machine

Part 2 of [OWNER_SITTING](OWNER_SITTING.md); its rules, build and screen settings apply here. Triaged
2026-11-20 by [D602](../decisions/RECORD.md#d602): what stays here needs the owner's eyes on Windows.
Older looks were carved out to [OWNER_LOOKS_CARVED](OWNER_LOOKS_CARVED.md) (ROADMAP M1.30, M3.2.13, M14) (full steps:
`git log -p` on this file before D602); I.1, I.3, I.4, F2, D2 and E22 closed. Item names are unchanged
and never reused; an item that took in an older one says so.

**Order for one sitting** (gates and blockers first; OWNER_SITTING A2 passed 2026-11-21, D612): E32 → OWNER_SITTING B46 with item 42, B48 and item 50 → C16 → C17, C18 →
I.5 → F8 → F12 → G1 → H1 → C12 (optional). Answered 2026-11-20 and moved to [PROGRESS](../PROGRESS.md): C14, C15,
E33, E34 (with F4), E35 (with C1, C7, E18; one defect kept in NEXT), E36 (with E5, E8) and B45 (with E13–15). Answered
2026-11-22: P51 and P52 (words in PROGRESS; their findings D620). Answered 2026-11-26 on package #152: F7, F9, F10 and F11, all passed (words in
PROGRESS).

---

## C — Run view (the 125 % screen)

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

## F — Dialogs

8. **CC1 as a distribution in the W99 dialog** (listed 2026-11-24, D648, W99 §5a). Edit a behaviour, choose
    *W99*: is *CC1* set to *Distribution (per driver)* with an empty table and the `cc1` field greyed? Fill
    the other nine keys, add rows (0.6, 0) and (1.5, 90), Confirm: is the error "· cc1Curve[1].share" and
    the row selected? Change 90 to 100, Confirm, reopen: are the points kept? Switch to *Fixed*, enter cc1,
    Confirm: is the curve gone? In Thai and English at 100/150 %: do the table, its headers and the Add /
    Remove buttons fit without clipping? *Owner, on #153 (2026-11-26):* the table should look unused under
    *Fixed*; a double-clicked cell's editor is not on its cell, check other tables. Both fixed (D676).
    - **Recheck on the next package** (D676): under *Fixed*, is the curve table grey, headers included? Double-click a
      cell here, in *Study periods* and in a vehicle type's speed curve: does the editor sit exactly on the cell? Switch
      to *Fixed*, Confirm, reopen and choose *Distribution*: are your points back? (Kept until the program closes.)
      *Owner:*

12. **The `lineSetback` field and a set-back at a red** (listed 2026-11-26; D669–D670;
    [LINE_SETBACK](../reference/LINE_SETBACK.md) §7).
    - **Package:** run #153 of *Package binaries* (`ae07ab7`, D670–D671; succeeded 2026-11-26), artifact
      `multra-windows-x64`. It also carries F8.
    - **a. The field.** Edit a behaviour in *Driving behaviours*, for the prototype, W74 and W99 in turn. On the
      junction page, is there a `lineSetback` checkbox with a field, unticked? Does it fit without clipping in Thai
      and English at 100/150 %?
    - **b. At a red.** Open *File → Open example → Line-stop lab* (D671): every vehicle type is on `w99-lab`, with
      the W74 behaviours `w74-lab` and `w74-sumo` ready, so no key is typed. Edit `w99-lab`, tick `lineSetback`,
      enter 1, Confirm and run. Pause during a red. Does the first car of each queue stand about 1 m short of its stop
      line, and leave at green without a jump? Optionally switch the types to `w74-lab` in the behaviour library and
      look again.
    - **c. Save and reopen.** Save, close, reopen. Is `lineSetback` still 1? Untick it and save: does the file no
      longer carry it?
    - Report the words for a–c. Linux closes none of these (rule 3). *Owner, on #153 (2026-11-26):* with `lineSetback`
      unticked and ticked again, the 1 was gone; keep it, and check every field. Fixed for every such field (D676). Also
      asked whether `lineRest` and `lineSetback` duplicate W99's own stop short of the line (answered in PROGRESS).
    - **d. Recheck on the next package** (D676): tick `lineSetback`, enter 1, Confirm, run; untick, Confirm, run;
      reopen and tick again. Is it 1? Try one more field, e.g. a vehicle type's PCU or a speed area's deceleration.
      *Owner:*

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
