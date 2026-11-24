# Owner sitting, part 2 — the Windows machine

Part 2 of [OWNER_SITTING](OWNER_SITTING.md); its rules, build and screen settings apply here. Triaged
2026-11-20 by [D602](../decisions/RECORD.md#d602): what stays here needs the owner's eyes on Windows.
Older looks were carved out to [OWNER_LOOKS_CARVED](OWNER_LOOKS_CARVED.md) (ROADMAP M1.30, M3.2.13, M14) (full steps:
`git log -p` on this file before D602); I.1, I.3, I.4, F2, D2 and E22 closed. Item names are unchanged
and never reused; an item that took in an older one says so.

**Order for one sitting** (gates and blockers first; OWNER_SITTING A2 passed 2026-11-21, D612): E32 → OWNER_SITTING B46 with item 42, B48 and item 50 → C16 → C17, C18 →
I.5 → F7 → F8 → G1 → H1 → C12 (optional). Answered 2026-11-20 and moved to [PROGRESS](../PROGRESS.md): C14, C15,
E33, E34 (with F4), E35 (with C1, C7, E18; one defect kept in NEXT), E36 (with E5, E8) and B45 (with E13–15). Answered
2026-11-22: P51 and P52 (words in PROGRESS; their findings D620).

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

7. **W99 in the Driving behaviours dialog** (listed 2026-11-23, D640, W99 §9). Open a project, *Driving
    behaviours*, edit a behaviour and choose *W99 (Wiedemann 99)*, in Thai and English at 100/150 %: are
    the ten cc fields empty, the help note readable, an empty field named in the error, and do the tab and
    footer fit without clipping? Package: run #145 of *Package binaries*, artifact `multra-windows-x64`
    (expires 2026-12-07). *Owner, 2026-11-24, on #145:* "cc0–cc9 ว่างครบทั้ง 10 ช่อง; หมายเหตุช่วยเหลือของ W99
    อ่านได้ครบ; กด Confirm ทั้งที่ช่องยังว่าง หน้าต่างไม่ปิด; กรอกค่าที่อยู่นอกช่วง เช่น cc0 = 0 แล้วกด Confirm
    ข้อความยังไม่แจ้ง แต่เลื่อนไปให้ใส่ค่าถัดไป; แท็บเลื่อนดูได้ และปุ่ม Confirm/Cancel ด้านล่างอยู่ในจอตลอด ไม่ล้นจอ;
    ทำข้อ 3–4 ซ้ำทั้งภาษาไทยและอังกฤษ ที่ 100% และ 150% แล้วสามารถทำได้". Empty fields, help, refusal of an
    empty key and the fit passed. The out-of-range finding is fixed by D643 (the first key in order that is
    empty **or** out of range is named and focused; the message no longer says "w74"). **Open for that one
    check on the next package:** `cc0 = 0`, Confirm: the error ends "· cc0" and the cursor stays in cc0. Package for
    the recheck: run #146 of *Package binaries* (`cc57f2d`, D643; W99 also runs, D642), artifact
    `multra-windows-x64` (expires 2026-12-08).

8. **CC1 as a distribution in the W99 dialog** (listed 2026-11-24, D648, W99 §5a). Edit a behaviour, choose
    *W99*: is *CC1* set to *Distribution (per driver)* with an empty table and the `cc1` field greyed? Fill
    the other nine keys, add rows (0.6, 0) and (1.5, 90), Confirm: is the error "· cc1Curve[1].share" and
    the row selected? Change 90 to 100, Confirm, reopen: are the points kept? Switch to *Fixed*, enter cc1,
    Confirm: is the curve gone? In Thai and English at 100/150 %: do the table, its headers and the Add /
    Remove buttons fit without clipping? *Owner:*

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
