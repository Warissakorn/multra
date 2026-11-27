# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-27 — Dialog geometry checked on Linux: UI_GEOMETRY U1 and U2, and a crash it found (D680–D681)

- **Owner:** "UI_GEOMETRY U1, U2".
- **U1:** four checks run in every `requireFitsScreen` call (22 places in 12 UI suites). A cell editor must cover
  its cell, a widget must not be clipped by its parent, siblings must not overlap, and text must show whole.
- **U2:** `ui-dialog-sweep` at 1×, 1.5× and 2× opens every editor action's dialog, then the Add/Edit dialogs inside
  it. It runs in English and Thai, and each dialog is checked as opened and again with every box ticked.
- **Forcing:**
  - UG1–UG5 make each check fire on a dialog built to break it.
  - Removing D676's delegate makes the sweep report the owner's F8 geometry exactly: editor at 4,2, 91×24, on a
    99×23 cell.
- **Measured:** 0 findings in 111 U1 checks and in 104 dialog views per scale. One false-positive kind was removed:
  a label's mnemonic `&`.
- **A crash found (D681):**
  - In an active dialog, *Add lane* in the junction lane override recursed through the table focus rule until the
    stack overflowed. The rule dates from 2026-11-02.
  - The tests had clicked in an inactive window, which receives no focus events. U1's layout pass made the dialog
    active, and `junction-recipe-ui` segfaulted.
  - The rule now uses the cell widget's own cell and cannot re-enter itself.
- **Also changed:** G1 runs only in the sweep, since opening an editor moves a table's current cell under a test
  still driving the dialog.
- **Not reached:** about 40 dialogs that need a selected object. That is the next extension, listed in NEXT.
- **Verified (Linux only):** `check` passes, `ctest` 382/382.
- **Then UI_GEOMETRY §5's next extension and the D679 leftovers (D683; the owner: "เริ่มจาก UI_GEOMETRY §5 กับบั๊กเล็ก D679 เลย"):**
  - the sweep selects a Link, a Connector, two Links and each table's first rows before triggering actions, and undoes
    an action that edits: 222 actions and 124 dialog views per scale (104 before);
  - G1 now checks widgets set on cells too. It found 376 per scale in 6 dialogs, the D679 combo among them. A field in a
    table now fills its cell, and a field's minimum width widens its column. 0 findings at 1×, 1.5× and 2×; UG6 forces it;
  - a decision's position clamped by a shorter Link is kept and comes back on a longer one;
  - recovery and Compare now say when a file's `lineRest` off is not used.
  - **Verified (Linux only):** `check` passes, `ctest` 382/382. Not run locally: the sanitizer tree (not built here;
    `early (sanitizer)` runs on the push). 34 actions are still not reached (UI_GEOMETRY §5).
- **Then the tree was trimmed (D682, the owner's "R1-R5 ตามแนะนำ"; recorded small, as its own rule now says):**
  - 26 files and 3.0 MB left the tree: images and raw evidence, each recoverable with `git show 82f5e8d0:<path>`;
  - the measuring patches moved to `docs/archive/patches/`;
  - NEXT went from 25,991 to about 5,900 characters.
  - Correction: only 12 images were unreferenced, not the 72 first stated. That count matched file names only,
    and most images are named by their folder.

## 2026-11-27 — The development pipeline, an early sanitizer, and BA263's use-after-free (D678–D679)

- **CI:** PR #232's `linux (sanitizer)` job failed on `linestop`. ASan found BA263 (D661) holding a reference into a
  temporary scenario. The row now copies the vehicle type: under `build/sanitizer`, the failure reproduced before the
  fix and the group passes 20/20 after it.
- **Owner:** asked for the most suitable pipeline, then "ตามแนะนำ".
- **Done** ([DEV_PIPELINE](plans/DEV_PIPELINE.md), D678):
  - `native.yml` runs on every push to `claude/**`;
  - a local sanitizer step joins the ladder;
  - one package per sitting, at most five owner items a round, one pull request per topic.
- **Contract only:** [UI_GEOMETRY](reference/UI_GEOMETRY.md), dialog geometry checked on Linux. Options U1–U3, with
  U1 then U2 recommended; the owner chooses.
- **Review, then "ตามแนะนำ" (D679):**
  - the push trigger ran every job twice and cancelled Windows runs, so only `early (sanitizer)` (`early.yml`) now
    runs on push;
  - kept unticked values could carry a number into another project with the same behaviour id. They are now cleared
    on open, on new and on recovery. A row checks this, and it fails without the fix;
  - the `lineSetback` note now says the field's number is used only when ticked.
  - verified on Linux only: `check` passes, `ctest` 378/378.
- **Also asked:** whether the repository is bloated. Answered in chat with measurements, no change: docs are
  16 MB of the 28 MB tracked, against 4.3 MB of source.

## 2026-11-27 — The owner's Windows checks F8 and F12 pass on package #155

- **Owner, on package #155** (`fca6669`, D676–D677): "F8 ผ่าน F12 d และ e ผ่านทั้งหมด".
  - **F8:** the CC1 table, its muted look under *Fixed*, and cell editors on their cells.
  - **F12 d:** an unticked value comes back when ticked again.
  - **F12 e:** no `lineRest` box, a 1 m rest by default, 0 at the line, and the warning for a file saved with it off.
  - **F12 a and c**, then: "F12 a และ c ผ่าน". The field and its default note fit, and 0.5 survives save and reopen
    and leaves the file when unticked.
- **Closed:** F8 and F12, removed from the [Windows sheet](plans/OWNER_SITTING_WINDOWS.md). F12 b, at a red, is
  covered by e. Section F of the sheet is now empty.

## 2026-11-26 — `lineRest` always on, `lineSetback` 1 m by default (D677)

- **Owner:** asked whether to remove `lineRest` and `lineSetback`. The session recommended removing only the switch,
  because off brings back the standing gap and the 10–14 s crawl (LINE_STOP §1). Then: "ตามแนะนำ
  ไฟล์ที่บันทึกโดยปิด lineRest เปิดได้โดยไม่สนค่านั้น พร้อมแจ้งเตือน ตั้งค่าเริ่มต้น lineSetback ไว้อย่างเหมาะสม".
- **Done** ([LINE_SETBACK §9](reference/LINE_SETBACK.md), [LINE_STOP §11](reference/LINE_STOP.md)):
  - Run settings has no `lineRest` box.
  - A file's `clampRules.lineRest` is checked and ignored, never written. A file saved with it off opens with
    `EDIT_LINE_REST_IGNORED` in the message line (Thai and English).
  - `ClampRules::lineRest` stays engine-only, for rows and the M6 U record.
- **Default:** an absent `lineSetback` is 1 m (`kDefaultLineSetback`):
  - the source is SUMO's `jmStoplineGap` at signal stop lines, and the NGSIM look's median is 0.96 m;
  - at 1 m, the measured options show no red pass and delay within 0.4 %, except W74 on M2.6 (+4–6 %);
  - the dialog shows 1 with a note when unticked;
  - an explicit 0 rests at the line.
- **Rows:**
  - BA245, BA273 and BA274 amended;
  - rows that pin the at-the-line law (BA239, BA247, BA262, BA268 and the rest of `linestop`) set `lineSetback` 0;
  - `signal-ui` checks the missing box, the warning and that Save drops the value.
- **Verified (Linux only):** `check` passes, `ctest` 378/378; `multra-cli 42` byte-identical before and after. The owner's look is F12 e on the next package (rule 3).

## 2026-11-26 — The owner's F8 and F12 findings: cell editors, unused tables, kept values (D676)

- **Owner, on package #153** (`ae07ab7`), with two pictures of the CC1 table:
  - "เมื่อสร้างตารางใน Distribution และเพื่อมาใช้ Fixed ควรแสดงตารางด้วยสีที่จางลงเพื่อให้ผู้ใช้มองเห็นว่าตารางไม่ถูกใช้"
  - "การดับเบิลคลิก เพื่อเติมตัวเลขช่องที่ขึ้นมาให้แก้ไขตัวเลขไม่ตรงกับช่องตารางเดิม ตรวจสอบตารางอื่นๆในโปรแกรม ที่มีข้อผิดพลาดแบบเดียวกัน"
  - "พฤติกรรมเดิมของ W99 ในการไม่ติ้ก lineRest และ lineSetBack รถก็จอดห้างจากเส้นอยู่แล้ว … การตั้ง 2 ค่าที่จะทำให้การตั้งค่าซ้ำซ้อนหรือไม่"
  - "เมื่อติ้ก lineSetBack และตั้งค่า เป็น 1 … เอาติ้กออก … กลับมาจะติ้กใหม่พบว่าค่า 1 ที่ตั้งไว้ หายไป โปรแกรมควรคงค่านี้ไว้ … ตรวจสอบช่องอื่นๆทั้งหมดในโปรแกรมด้วย"
- **Fixed in the shell** (D676; no engine or file change):
  - Reproduced offscreen: the editor of a 99×23 cell was 91×24 at (4, 2). Every table without a delegate of its own
    now places it on the whole cell, so the fix reaches every table at once.
  - A disabled table is muted (grey cells and headers, no current-cell frame).
  - Seventeen places hid a value behind a tick box or a choice and dropped it on Confirm. Each now keeps it for the
    session and brings it back when ticked again. It is never written to the file.
  - The W99 fields show 0.35, not 0.34999999999999998.
- **Answered, no change:** with `lineRest` off, each model rests at its own standstill gap short of the line. For W99
  that is CC0, measured at a median of 1.40 m (LINE_STOP §1), which is why it looks like `lineRest` plus 1 m. The two
  are not the same: the off-mode gap is also the gap between queued cars, it applies at every kind of line, and
  for the prototype and W74 it comes with a 10–14 s crawl. `lineSetback` acts only under `lineRest`.
- **Rows** in `behaviour-library-ui`: the editor is on its cell; the CC1 curve is kept beside *Fixed*;
  `lineSetback` 1 survives untick and re-tick.
- **Verified (Linux only):** `check` passes, `ctest` 378/378; F8 and F12 stay open for a recheck on the next package (rule 3).

## 2026-11-26 — The node-analysis clamps: diagnosed, E1 fixed, E2 measured and kept out (D672–D675)

- **Owner:** "ไล่หาสาเหตุ clamp บน signalised-node-analysis ต่อ".
- **Traced** ([evidence](evidence/exact-line-rule.md)), seed 42:
  - A straight car resting at its red with its front exactly at a 2.73 m connector's start is read as on it by the
    merge rule `give-way/connector-44/to/connector-42`, so it holds the turner for the whole red (E1).
  - At green the turner is released while the next straight car, 1.35 s out on the link upstream, is invisible to
    a rule that scans only its conflict segment (E2), and that car is clamped behind it.
- **E1's fix, measured with a reverted [patch](archive/patches/exact-line-rule-options.patch):** node analysis clamps
  4.7 → 0 (prototype), 19.0 → 15.7 (W74), 18.6 → 12.1 (W99). Every other shipped project is identical.
- **E1 fixed (D673, "ทำ E1 เลย"):** in `simulation.cpp` as measured, plus a guard for a marker at the segment's start. Row
  PR17; both mutations fail it (no fix, and no guard). E2 (PRIORITY_RULES §1's scan range) is open.
- **E2's contract** ([RULE_LOOKBACK](reference/RULE_LOOKBACK.md), D674; "เขียน contract E2 และวัดทางเลือก"):
  - Vissim's manual defines the gap by "the first upstream vehicle", across connectors, with *Look beyond red
    signals* optional.
  - Measured with a reverted [patch](archive/patches/rule-lookback-options.patch): G1R (upstream, stopping at a red head)
    is identical to today except on M2.6 (prototype clamps 0.2 → 0, W99 hard braking 646 → 603); G1 (past red)
    worsens node analysis by 7.5–13.7 s of delay.
  - Recommendation: G1R with an index and callgrind first. The owner chose it ("ตามแนะนำ").
- **G1R built and kept out (D675):** callgrind +3.31 % instructions on M2.6 and +1.80 % on node analysis, against a gain
  on M2.6 only. G0 stays as a known limit; the code is [a patch](archive/patches/rule-lookback-g1r.patch).
- **Verified (Linux only):** `multra-cli 42` byte-identical; `check` passes (378 of 378).

## 2026-11-26 — A set-back at signal heads: `lineSetback`, and a lab for the checks (D670–D671)

- **Owner:** "ตามแนะนำ" (H1 + C1 of [LINE_SETBACK](reference/LINE_SETBACK.md)).
- **Engine:** a behaviour's optional `lineSetback` (m, [0, 10]) moves the line law's target that far short of a signal
  head under `lineRest` (`setBackHead`). Give-way, Stop, zone and gate lines, the cap, the amber decision and the
  observers keep the real line.
- **File:** schema 88 when an owned behaviour carries it; refused below; absent means 0. The *Driving behaviours*
  dialog has the field on its junction page for every model.
- **Rows** BA273–BA277 (`linesetback`, a new file). Two mutations each fail a row: no set-back (BA273), and a set-back
  at give-way lines too (BA275).
- **Not changed:** M0 and every shipped project (no file carries the key). The exact-line clamps on
  `signalised-node-analysis` are partly diagnosed: a turner passes its give-way line and enters 0 m ahead of a
  straight car. Why is still open.
- **Verified (Linux only):** `multra-cli 42` byte-identical; `check` passes (378 of 378).
- **The line-stop lab** (D671, the owner's request for W74 and W99 test profiles): `data/projects/line-stop-lab.traffic.json`
  on the examples menu. It is the four-leg with `w74-lab`, `w74-sumo` and `w99-lab` ready, and every type on `w99-lab`.
  Built by `tools/line_stop_lab_network.hpp`.
- **Owner:** [F12](plans/OWNER_SITTING_WINDOWS.md) on the next package, in the lab.

## 2026-11-26 — A set-back at holding lines: sources and the contract (D668–D669)

- **Owner:** "ให้การหยุดแบบ lineRest หยุดห่างจากเส้นหยุด ด้วยระยะที่เหมาะสม ดีหรือไม่", then "ตามแนะนำ" twice (sources, then
  a contract for a key with default 0).
- **Sources** ([evidence](evidence/line-setback-sources.md), D668): Vissim 0.5 m before signal heads, SUMO
  `jmStoplineGap` 1 m (0 at minor lines). Both are program defaults without a field source. NGSIM, on an unverified
  stop-bar stand-in, gives a median of 0.96 m.
- **Contract** ([LINE_SETBACK](reference/LINE_SETBACK.md), D669), measured with a reverted
  [patch](archive/patches/line-setback-options.patch):
  - at signal heads, 0.5–1 m passes no red and changes delay by under 0.4 % (W74 on M2.6 +4–6 %);
  - startup lost time rises about 0.1 s per 0.5 m;
  - at give-way lines it costs 2–22 % of delay.
  - Recommendation: a behaviour key at signal heads only, default 0.
- **Found:** on `signalised-node-analysis`, a front resting exactly at a connector's start clamps 4.7 per run
  (prototype, +7 s delay). It is gone at 0.01 m short; a separate defect, not yet diagnosed.
- **Verified (Linux only):** `multra-cli 42` byte-identical with the patch and after the revert; `check-docs` passes.

## 2026-11-26 — The owner's Windows checks F7, F9, F10 and F11 pass on package #152

- **Owner, on package #152** (`a18a229`, D643–D665):
  - **F7:** "ข้อความ error ลงท้ายด้วย "· cc0" และเคอร์เซอร์ยังอยู่ที่ช่อง cc0" (D643's fix holds).
  - **F9:** "ผ่าน".
  - **F10:** "ผ่านทั้งหมด" (W99 queues at rest, with and without `reactionTime` 0.5 s).
  - **F11:** "ผ่านทั้งหมด" (the prototype's first car eases into its stop at a red line).
- **Left in F:** F8, the CC1 table, on the same package. Removed from the
  [Windows sheet](plans/OWNER_SITTING_WINDOWS.md); its order line is now `… I.5 → F8 → G1 …`.
- **Also asked:** whether `lineRest` should stop a set distance short of the line. Sources first ("ตามแนะนำ", D668,
  [evidence](evidence/line-setback-sources.md)): Vissim 0.5 m, SUMO `jmStoplineGap` 1 m, both program defaults
  without a field source. On NGSIM, using an unverified stop-bar stand-in, first cars rest a median 0.96 m short.
  Recommendation: a key with default 0. No code.
- **Docs** cut NEXT to open items (27,890 → 24,163 characters; history in
  [archive/NEXT-state-2026-11-26](archive/NEXT-state-2026-11-26.md)).

## 2026-11-26 — The approach to a car at rest, measured on NGSIM Lankershim (D666)

- **Owner:** "ทำเรื่องการชะลอเข้าหาแถวรถต่อ หาแหล่งอ้างอิงก่อน", then "ตามแนะนำ" (measure).
- **Sources** ([queue-approach-sources](evidence/queue-approach-sources.md)): CAMP's last-second braking (DOT HS 809 574),
  Rittger's simulator, and Treiber et al. None measures the onset toward a standing queue.
- **Measured** ([evidence](evidence/queue-approach-ngsim.md)): NGSIM Lankershim, 1.6 million rows. Required deceleration at
  onset:
  - the first episode toward a car at rest has a median of 1.42 m/s²;
  - the last braking phase has a median of 1.88;
  - toward a line it is 2.07.

  Today's law starts at 2.0 and B3 at about 0.77, which is earlier than 84 % of the first onsets.
- **No code.** The owner kept today's law toward vehicles at rest ("ตามแนะนำ", D667). B is closed with B3P. The staged
  approach, the higher final braking and the last step to rest are known deviations.
- **Verified (Linux only):** `check-docs` passes. The scripts are committed beside the evidence; the data is not.
