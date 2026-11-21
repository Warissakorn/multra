# Owner sitting — everything that waits on the owner, in one pass

The owner's answer to the 2026-10-09 review's R4 (D143): clear the queue in **one sitting** with
this sheet, on the owner's Windows machine, and carve out whatever the sitting does not reach.
This is the only list of owner items; [NEXT](../NEXT.md) points here instead of keeping a copy
(hard rule 3). A session prepares the build and prints this sheet; it records the owner's words
verbatim and fills in **no verdict itself**.

**Rules.**
- Each item names what to open, what to do, and its failure condition as a yes/no question.
  Write the owner's answer on its *Owner:* line with the date, then move the item's result to its
  ROADMAP/PROGRESS line and delete it here.
- **Carve-out (R4 b).** After the sitting, any item still unanswered and listed for more than 14
  days becomes a numbered carve-out in [ROADMAP](../ROADMAP.md) under its milestone (rule 2), and
  leaves this sheet. A look is never closed by a session (only a new D-row could change that).
- *Listed* is the date of the decision or merge that created the item.

Build for the sitting: `cmake --preset desktop`, `cmake --build --preset desktop`, then
`multra-desktop`. Screen at 100 %, then 125/150/200 % where an item says so.

---

## A — Gates (first: a fail here changes what comes next)

2. **M3.2.7d T-junction exercise** (listed 2026-09-26). Fill the sheet in
   [M3_ACCEPTANCE](M3_ACCEPTANCE.md) §3 on `data/projects/t-junction-priority.traffic.json`. Does
   minor-road delay respond to gap time as an engineer expects? It is what M3.2 closes on (R7).
   *Owner:* 2026-11-18, attempt 1, step 1 only (verbatim): "พบว่า conflict area แสดงถูกต้อง แต่เส้นหยุดผิดตำแหน่ง
   link ทิศมุ่งตะวันตก ควรตั้งเส้นหยุดไว้ทางทิศตะวันออกของ conflict area และเส้นหยุดของ conflict area บน
   connector ควรอยู่ก่อนเข้า Link". **Fail at step 1, kept** (§3: fixes are separate attempts).
   *Session note (measured in the file, no verdict):* lanes are 3.5 m with a 1 m median. The merge waiting
   lines sit at the D50/D59 one-metre admission strip at each Connector's end, not before the displayed
   (D115) overlap: `wait-14` (left turn) at (−19.0, 2.1) and `wait-18` (right turn) at (19.0, −2.3) are on
   the major lanes' centre lines, so the minor vehicle waits inside the lane it joins; the major lines
   `wait-13` (westbound, x = −19) and `wait-17` (eastbound, x = 19) are at the strip, downstream of where the
   turn first overlaps the lane (about x = ∓6). The crossing's lines `wait-9` (y = −5.7, before the
   eastbound edge at −4.0) and `wait-10` (x = −13.8, upstream of the area) are before their area. This is
   the planned M3.2.4e gap (M3_CONTRACT: D50/D59 extents and waiting positions unchanged by M3.2.4d).
   *Owner:* 2026-11-19, attempt 1, steps 3–4 (verbatim): "สีของ Conflict area ยังมองยากทำเป็นสีทึมดีหรือไม่ ·
   การเปลี่ยน gap/headway สมเหตุสมผล แต่การหยุดรถตรงเส้นหยุดของ conflict area ยังไม่สมจริง เนื่องจากมีการเคลื่อนตัวเล็กน้อย
   ก่อนถึงเส้น และมีจังหว่ะที่ใกล้ถึงเส้นจะขยับอย่างรวดเร็ว หรือเส้นหยุดนี้ไม่ใช่เส้นหยุดจริงแต่เป็นเส้นหยุดสมมุติเฉยๆ ·
   ในแท็บ edit conflict area ต้องแต่ละปุ่ม 2 ครั้งถึงทำงาน ตรวจสอบปัญหานี้ทั้งหมดของโปรแกรม".
   *Session note (measured, no verdict):* seed 42, crossing turn's line `wait-9`, Debug build on Linux. Vehicles
   35 and 75 never come to rest at the line: in `approaching`/`following` mode the prototype law decays the
   speed toward the line as toward a standing vehicle, so a held minor crawls at 0.5 → 0.02 m/s for about
   20 s and settles 1.9–2.9 m short (F1a's floor binds only in `braking`). Each release jumps to the type's
   2.5 m/s² in one tick, and when `majorBlocks` returns a tick later the vehicle brakes at up to 3.6 m/s²
   (vehicle 75: go–stop twice within 7 s). The line is the model's real hold point for the vehicle's front;
   the two defects are the approach law and the per-tick gap test without hysteresis. The double click was
   not reproduced: one synthetic click acted on every Conflict areas button (cycle, Edit, Restore, Delete),
   after a canvas pick, a row click and a shown tooltip; each handler took 14–156 ms.
   *Owner:* 2026-11-19, follow-up (verbatim): "สีทึบ 100% เลยดีหรือไม่ · ปุ่ม confirm, ปุ่ม กากบาท · การชี้หรือเลือก
   connector ยังเห็นเส้นขอบสีขาวอยู่". *Session note:* the buttons were the Edit dialog's Confirm and close box; a
   row double-click opened that dialog twice, fixed by D587. The white Connector edge is removed by D588.
   Opacity waits for the owner's choice (screenshots at 35 %, 75 % and 100 % sent).
   *Owner:* 2026-11-19 (verbatim): "ความทึบ 100%  และเริ่ม M3.2.4e ได้เลย". *Session note:* D592 (areas opaque,
   an undecided automatic candidate stays faint); D593, [MERGE_MOUTH](../reference/MERGE_MOUTH.md): the
   regenerated fixture puts `wait-14` at 12.31 m (overlap from 13.31 m), `wait-18` at 6.99 m (from 7.99 m),
   `wait-13` at 154.65 m and `wait-17` at 151.36 m on their Links (x = −4.65 and x = 1.36), 1 m short of each
   overlap. Attempt 2 is the owner's.
   *Session note (2026-11-19, D596):* the creep and lurch: a held vehicle now rests with its front at a zone's
   line (measured T-junction seeds 42–51: rest 2.65 m short → 0 m, crawl under 1 m/s 194.7 → 13.8 s per seed,
   go-then-brake 15 → 0), [LINE_APPROACH](../reference/LINE_APPROACH.md) §9.
   **Attempt 2 (D601, owner 2026-11-20 "Yes, cut it"):** steps 1, 3 and 4 and the verdict only; step 2 and
   steps 5–7 are automated (M3_ACCEPTANCE §3). Also look at D587 (a row's dialog opens once), D588 (no white
   Connector edge) and the Stop line tool (`W`).
   *Owner:* 2026-11-20, attempt 2, step 1 (verbatim): "ตอนออกจาก เครื่องมือ conflict area แล้วยังมี เส้นหยุดให้เห็นอยู่".
   Steps 3–4 and the verdict are not yet given. *Session note (read in the file and the code, no verdict):* the
   two lines still drawn are `wait-15` and `wait-17`, each the line of a Yield control (`stop-16`, `stop-18`);
   D590 shows a line carrying Stop or Yield under every tool, and hides only plain lines. Whether a controlled
   line should also hide outside the tool is the owner's choice (it would amend D590).
   *Owner:* 2026-11-20 (verbatim): "ข. และ เครื่องมือเส้นหยุดสามารถสร้างได้โดยไม่ต้องใช้ Conflict area ก็ได้ ให้สร้าง
   พื้นที่ให้ทางเหมือน vissim", then "ข. Priority rule แบบ Vissim". *Session note:* controlled lines now hide outside
   the right-of-way tools (D603); priority rules are drafted as PRIORITY_RULES (D604), waiting on its §8.
   *Owner:* 2026-11-20 (verbatim): "ตรวจสอบว่าการวิ่งมาหยุดที่เส้น มีรายละเอียดมากกว่านี้หรือไม่ ลองศึกษาจากโปรแกรมอื่น".
   *Session note (read, not measured):* [line-approach-other-programs](../evidence/line-approach-other-programs.md):
   Vissim, SUMO and Aimsun all limit how far a yielding driver sees the major road (a visibility distance), so it
   approaches at a speed from which it can stop; MULTRA sees every major vehicle at any distance and does not slow
   when the gap is clear. Choices V (visibility), D (dwell distribution), I (impatience) or none wait for the owner.
   *Owner:* 2026-11-20 (verbatim): "V ทำเหมือน vissim เริ่มตัดสินใจเรื่อง gap ทำด้วยหรือไม่". V chosen, as Vissim;
   the contract waits on the default distance and the reading of "visibility" (asked in chat the same day).
   *Owner:* "1ก 2ก", then "ทุกไฟล์". *Session note:* done as D607 ([VISIBILITY](../reference/VISIBILITY.md)); at 100 m no
   shipped result changes, so on the T-junction attempt 2 looks as before unless a shorter visibility is entered
   in the area's dialog (20 m: crossing 19.15 → 19.86 s, near turn 9.93 → 11.64 s).
   *Owner:* 2026-11-20, attempt 2, step 3 (verbatim, with a picture): "gap/headway สมเหตุสมผล แต่ต้องการปรับการแสดง
   conflict area ให้มีสีแบบในรูปและลูดศรชี้ตามทิศทาง ลักษณะคล้ายในรูป". *Session note:* done as D610 (green, red and
   olive areas; arrows on every edge under the Conflict area and Stop line tools). Step 4 and the verdict remain.
   *Owner:* 2026-11-21 (verbatim): "เหลือลูกที่กึ่งแล้วแนวที่เกิด conflict แต่ละแนว"; in chat: a centre line with the
   arrowhead at its exit end. *Session note:* D610 (b), each band now carries one.
   *Owner:* 2026-11-21 (verbatim): "ปรับให้การชี้ที่ Link และ connector แสดงเส้นขอบที่มีลูกศรสีดำ ส่วนการเลือกให้เป็นเป็นสีฟ้า
   แล้วปรับให้การเลือก conflict area ทำให้ link และ connector ขึ้นสถานะถูกชี้", then "ไม่ปรับสีถนน เพิ่มแค่ข้อ 2". *Session
   note:* D611.
3. **M5.7 rehearsal** (listed 2026-10-08). The PROBLEM §4 sentence on a real aerial image and the
   M2.6 template with real volumes (B4), Run seeds 42-51, Copy into the report. Record file,
   commit, table and the image size against the 32 MiB cap; labelled partial until M6. Did any
   step need a workaround, hand-edited JSON or outside help? *Owner:*

## B — Decisions in chat (one line each)

4. **The M2.6 template** (listed 2026-09-25). `data/projects/m2.6-study-template.traffic.json`
   has placeholder volumes, a guessed timing-to-approach mapping and no aerial image. Supply the
   real study's counts, timing sheet and image for M5.7 (A3)? *Owner:*
21. **MA15's capacities and the M10.1 gate** (listed 2026-10-22, D294). Open Sheffi (1985), book
   p. 114, Figure 5.1, in print or a PDF. The Markdown copy's image of that page is empty, and the
   capacities c = (2, 4, 3) were derived from Tables 5.1 and 5.4
   ([evidence](../evidence/macro-sheffi-benchmark.md)). Does the figure print 2, 4 and 3? If yes,
   does MA15 (Table 5.4 reproduced, equilibrium within the 10⁻⁴ gap) meet the M10 gate's benchmark
   clause? *Owner:*
22. **M11.3 sources** (listed 2026-10-22). The LTM engine waits until §4 and §5 of
   [MESO_NETWORK](../reference/MESO_NETWORK.md) are checked against Yperman (2007, KU Leuven thesis)
   and Tampère et al. (2011, TR-B 45(1) 289–309). The environment refused every host that holds them
   (proxy 403). Either allow `arxiv.org`, `lirias.kuleuven.be` (and `sciencedirect.com` if possible)
   under the environment's Network access → Allowed domains
   (https://code.claude.com/docs/en/cloud-environments#network-access), or upload the PDFs
   (optionally Boyles et al., arXiv 2502.05182, too). Which? *Owner:*
   **2026-10-22:** the owner uploaded Tampère et al. (2011); the node model is in (D297). Still needed:
   Yperman (2007), or Boyles et al. as a citable secondary source for the LTM link model.
   **2026-10-22:** the owner supplied Yperman (2007) as a Markdown repository; the link model and the
   engine are in (D298). Boyles et al. are not needed. Only the host question remains, for later sources.
26. **Hybrid meso–micro, and the user roles** (listed 2026-10-27, D378). Aimsun Next runs a Micro
   area inside a Meso network in one run; MULTRA's M12 cuts a subarea and runs each level
   separately. Book a simultaneous hybrid run (a new milestone after M12), or keep M12's cut only?
   Also: PROBLEM §1 now names two equal roles (study engineers and consultants; transport
   planners), with road and signal agencies doing the same work, as following from the equal
   weight. Correct that wording? *Owner:*
29. **Road-model review follow-up** (listed 2026-10-28, [D382](../decisions/RECORD-D200-D399.md#d382)). Each is a
   choice, recommended first: (a) generated Nodes resize to their arms' templates, needing a "derived
   polygon" mark apart from `generated`, or keep D382's refusal only; (b) one turn key across levels,
   Street segment id plus end, for coarse values and Node-analysis volumes (a schema step), or keep Link ids and
   remap on each edit; (c) keep `forwardOnStreetSegment` (D382) or store each Street segment direction explicitly, needed
   if a Street segment carries a pocket and an approach one way; (d) suppress approach and Node delay and LOS
   under `NODE_ANALYSIS_PARTIAL` for the approaches missing a turn, or keep showing them; (e) HCM
   storage traced upstream to the previous Node or pocket start, or the in-leg Link's length; (f)
   Node-analysis volumes read from the planned CountSet rather than stored on the Node. *Owner:*
33–36. **J4T/J5/J6 choices:** answered and approved; [whole B33–B36](../archive/OWNER_SITTING-B33-B36-2026-10-31.md).

Answered 2026-10-29: **B32**, owner selected the recommendations ([D404](../decisions/RECORD.md#d404)):
- “ใช้สอง ring ตามหมายเลขเสมอ; P4 ปล่อย 4 แล้ว 8 โดยอีก ring รอแดง (แนะนำ)”
- “allRed คือ clearance ที่กรอก; เวลาแดงรอ barrier คำนวณแยกจาก timing ของอีก ring (แนะนำ)”
- “เริ่ม phase 2/6 ที่เวลา 0 ตาม U0; leading far turn อยู่ท้าย cycle ก่อนหน้า (แนะนำ)”

Answered 2026-10-29: **B31**, owner: defer all-way-stop generation until A5 or U1 needs it;
keep the fill listed but disabled (`FILL_NODE_ALL_WAY_STOP`), leave StopControl unchanged and
use no schema 63 ([D399](../decisions/RECORD-D200-D399.md#d399)).

Answered 2026-10-28: **B30**, owner: “B30 เอาตามแนะนำ” (“B30 as recommended”): JT-1a–JT-5a and
UX-1a–UX-4a ([D388](../decisions/RECORD-D200-D399.md#d388)); U0 is a session plus the owner's import check.
Answered 2026-10-28: **B28**, owner: “ช่วยตัดสินใจ” (“help decide”), given after being told that
item 1 reverses D373; decided on that delegation in [D380](../decisions/RECORD-D200-D399.md#d380): 1a, 2a.
Numbers are not reused.

Answered 2026-10-28: **B27**, owner: “ช่วยตัดสินใจ” (“help decide”), decided on that delegation in
[D379](../decisions/RECORD-D200-D399.md#d379): 2a, 3a; 1a by B28 (D380); 4 returns to the owner only if HCM 7 prints no
planning-level method for a Node without lane detail. Bookings only; each slice starts when the owner asks. Numbers are not reused.

Answered 2026-10-27: **B25**, owner: “ช่วยตัดสินใจ” (“help decide”), decided on that delegation in
[D375](../decisions/RECORD-D200-D399.md#d375): M13 A3's choices 1a–5a ([NODE_ANALYSIS](../reference/NODE_ANALYSIS.md) §12). Numbers are not reused.

Answered 2026-10-23: **B23**, owner: “เอาตามแนะนำ” (“as recommended”), [D315](../decisions/RECORD-D200-D399.md#d315):
QM (c), M1-d with M1-b, M2-c with M2-b, not-blind cases as verification only, Boyles et al. 2025
supplied as a Markdown repository or with `arxiv.org` allowed. Numbers are not reused.

Answered 2026-10-23 (in chat, AskUserQuestion): **S4's four choices**, as recommended,
[D313](../decisions/RECORD-D200-D399.md#d313): S4-1a, S4-2a, S4-3a, S4-4a ([READINESS](../reference/READINESS.md) §8).

Answered 2026-10-22: **B24**, owner: “เอาตามแนะนำ” (“as recommended”), [D304](../decisions/RECORD-D200-D399.md#d304):
S3's choices 1a, 2a, 3a, 4a, 5a, 6b, 7a ([STREET_SEGMENTS](../reference/STREET_SEGMENTS.md) §11). Numbers are not reused.

Answered 2026-10-21: **B20**, owner: “เอาตามแนะนำ” (“as recommended”), [D292](../decisions/RECORD-D200-D399.md#d292):
Meso choices 1a–7a ([MESO_NETWORK](../reference/MESO_NETWORK.md) §11). Numbers are not reused.

Answered 2026-10-17: **B19**, owner: “ช่วยตัดสินใจ” (“help decide”), decided on that delegation in
[D214](../decisions/RECORD-D200-D399.md#d214): the [tiers](../reference/BEHAVIOUR_PARAMETERS.md) stand, the 50 km/h
lateral reference moves to fixed, and tier B is ordered by effect on Thai study outputs (kerb bias,
commitment deceleration, merge deceleration, speed limits, …). Numbers are not reused.

Answered 2026-10-16: **B1, B2, B3, B5, B6, B8, B12, B13**, owner: “ปิดตามข้อเสนอ” (“close as
proposed”), [D201](../decisions/RECORD-D200-D399.md#d201). Keep `TrafficSim` until a release; D95/D101 stay off
and reopen only by route (i) in M3.4; the `level` skip is a discrepancy with M3_CONTRACT, queued as
a fix in NEXT; the standstill cap joins M3.2.8a.1; more routing stations wait for a study that needs
them; `four-leg-signalised` gets a 300 s cool-down (done 2026-10-25, D337); B12 is asked when multi-level S1
starts; the `w74` preset stays out of the catalog until M4.2 is revisited. Numbers are not reused.

Answered 2026-10-15: **UI B16/B17**, owner: “ทำตามแนะนำ” (“follow the recommendation”).
Proceed with layout A (task selector within the existing Objects dock) and the five proposed tool
groups, after slice 0. B18's six logic/state ideas stay separate and deferred; the reply does not
request those features. The [proposal](UI_PRESENTATION_SPEC.md) and
[slice plan](UI_PRESENTATION_SLICES.md) record the chosen direction; no visible UI change is made
by slice 0. Numbers B16–B18 are not reused.

Answered 2026-10-15: the fourteen multi-level workflow questions, as recommended
([D175](../decisions/RECORD-D001-D199.md#d175), [workflow](MULTI_LEVEL_WORKFLOW.md) §9); B14 and B15, moved out
([D178](../decisions/RECORD-D001-D199.md#d178): order accepted, keep-left default, standard motorcycle values for now).
Answered 2026-10-14 and moved out: B7 and B10 ([D158](../decisions/RECORD-D001-D199.md#d158)), B11
([D159](../decisions/RECORD-D001-D199.md#d159)). Numbers are not reused.

37–38. **J4T structure/cuts and polygon fitting:** answered and approved; [whole items B37–B38](../archive/OWNER_SITTING-B37-B38-2026-10-31.md), D416/D417.

39–41. **J7 analysis source; A9 Fill from model; A9 manual-count window:** answered (D418; D419, D420; D422). [B39](../archive/OWNER_SITTING-B39-2026-10-31.md), [B40](../reference/NODE_ANALYSIS_MODEL.md#b40-answered-choices), [B41](../reference/NODE_ANALYSIS_MODEL.md#b41-manual-count-comparison-window). Do not re-ask.

42. **M10.2 trip generation:** answered 1a–8a (D469); G2 choices D471. **Look (Windows):** Edit Zone's
   attributes, *Trip-rate sets…* (import `tests/data/trip-generation/trip-rates/example-4-1.json`) and the *Trip generation* page; layout ok?
   M10.3 D2 (D474): *Deterrence sets…*, *Cost matrices…* and the *Trip distribution* page (import `tests/data/trip-distribution/`'s sets). M10.4 E2 (D479): the *Mode choice* page, *Mode choice sets…* (import `tests/data/trip-distribution/mode-choice/hand.json`), *Period factors…* and *Write OD matrices*, then Run Macro on the period. M10.4 N1 (D481): *Mode choice sets…* → *Nests…* (add `foot` with `walk`, φ 0.5; try φ 1.5). D4 (D482): import `deterrence/hand-destination.json` and check the *Trip distribution* note. D485: *Procedure* page: add a procedure in *Procedures…*, run it, turn a step off. *Owner:*

45. **UI review fixes:** answered 2026-11-20, "แยกได้"; result in [PROGRESS](../PROGRESS.md).

46. **The Macro example** (listed 2026-11-10, D492). Open `macro-four-step`, run *Procedure*, check
    the Macro links table: do the example's size and story suit a first-time user? *Owner:*

47. **The M6 tolerance** (listed 2026-11-12, D515). The option sheet's
    [§6](../evidence/m6-benchmark-options.md#6-what-a-comparison-fixes-before-it-runs) now fixes
    every run setting; no comparable run has been made. Per kind, the engine mean passes when it is
    within: **(a)** a fixed band (S1 ±5 % of `s0`, S2 ±15 % of delay, U ±15 % of capacity) · **(b)**
    the band of (a) *and* the reference inside the 95 % CI of the mean · **(c)** a band per kind
    taken from the published field scatter of each source (needs the sources read first).
    *Session's recommendation:* **(a)** now, because (b) narrows as seeds are added and (c) waits on
    reading. The bands are a starting proposal, not derived from any engine figure. Also confirm U's
    `t_c` → `gapTime` translation (`gapTime = t_c − τ`, §6). *Owner:* **(a), with the translation, 2026-11-12 (D516).**

48. **The object-table pilot** (listed 2026-11-12, D525, [contract](OBJECT_TABLE.md)). On Windows,
    open *Study periods*: double-click a name, start or end to edit it in place (Enter commits, one
    Undo step; a time that is not `HH:MM:SS` is refused); right-click a header to hide or show a
    column; type in the field above the table to filter rows. Should the other tabs move to this
    table (one per commit), or does the pilot need changes first? *Owner:*

49. **Vehicle-type reader limits** (listed 2026-11-14, D560, [PROJECT_SCHEMA_DEMAND](../reference/PROJECT_SCHEMA_DEMAND.md#owned-vehicle-type-extension-d560)).
    **(a)** A missing or non-numeric `desiredSpeed.min`/`max` is refused with the JSON library's
    own message (e.g. `type must be number, but is boolean`), not MULTRA's. *Session's
    recommendation:* read them like every other row (`Missing field: min`, `Expected number: max`);
    only the error text changes. **(b)** An unknown key inside `desiredSpeed` or a curve point is
    accepted at every version and dropped on save (checked at the latest schema). Options: **(b1)**
    refuse it from a new schema step 77; the writer picks a file's version from its features, so
    today no file would declare 77 and the refusal would cover only files using a later feature;
    **(b2)** refuse it from 21, as the type row is, which can refuse an existing 21+ file that has
    such a key (none in the repository); **(b3)** leave it open, as routes and inputs are (D363).
    *Session's recommendation:* (b2), since the repository has no such file and the row it sits in
    is already closed from 21. *Owner:* **"ทำต่อตามแนะนำ", 2026-11-14: (a) and (b2) (D562).**
50. **Lane locks at dead ends** (listed 2026-11-17, D578, [LANE_LOCKS](../reference/LANE_LOCKS.md)).
    A vehicle waiting for a lane change into a standing queue holds its lane for the rest of the
    run (study ratchada-latphrao: 82 waiting, longest 3,411 s, 22 lanes blocked). Pick from §4:
    **A** entry lane by destination (opt-in, compile only), **B** missed exit after a threshold
    (engine, off by default), **C** zipper at a standing queue (with M3.4), **D** removal (against
    M3 §4). Also §7's questions: A's key, B's threshold and reporting, and the order relative to A3.
    *Session's recommendation:* A, then B off by default; not D. *Owner:* **"ทำตามแนะนำ", 2026-11-18:
    A, then B off by default; not D; C with M3.4.** LL1 ran first and found a span defect (D579).
    A is done (D580), B too, off by default (D581). *Owner, 2026-11-18:* **"ยอมรับค่าเริ่มต้นทั้ง 3
    ข้อ ใช้ตามแนะนำ": the key on the decision, no shipped threshold, diverted delay on the movement
    driven and `diverted` on the counted one (D582). Closed.** Still to see in the editor (with item
    C/E checks): the decision dialog's "enters on the lanes that serve it" checkbox and the
    behaviour editor's `missedExitAfter` field.

## C–I — on the Windows machine

Sections C (Run view), D (Right-of-way), E (Drawing and gestures), F (Dialogs), G (Object
interchange), H (Desktop automation) and I (UTDF probe) are in
[OWNER_SITTING_WINDOWS](OWNER_SITTING_WINDOWS.md); the rules above apply to them, and their item
names (C14, E32, I.5) are unchanged. Triaged 2026-11-20 ([D602](../decisions/RECORD.md#d602)): that sheet
holds the sitting's order; the older looks are carved out to [OWNER_LOOKS_CARVED](OWNER_LOOKS_CARVED.md) (ROADMAP M1.30, M3.2.13, M14).
