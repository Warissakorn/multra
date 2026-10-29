# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-10-29 — Kerb/far pockets and derived junction layouts (D391, D392, M14 J2)

- **Asked:** continue J2 only on `claude/upbeat-ptolemy-qe1x84`, stop for review, no PR; owner
  approved the two clarifications ("ได้ เอาตามแนะนำ"): physical taper then storage, 45°/135° naming.
- **Changed:** `addTurnPocket` draws a kerb/far widening taper and full-width storage, on both
  driving sides, inside one Command. Original lane footprints, route/head ids, external heads,
  boundary/taper heads and one-step Undo/Redo survive. The legacy split tool is unchanged.
  An implied route continues along the ordinary path of a widening Connector, preserving its
  old outer lane; two ordinary bridges remain ambiguous, explicit Connector selection unchanged.
- **Derived:** `approachLayout` gives lane turns/widths/stop lines, pocket storage/taper (also over
  a bend and a later split), travel bearings and NB/EB/SB/WB + L/T/R. Duplicate/tied directions
  and U-turns are refused; per-call direction overrides change only labels. J3/J6 own persistence/UI.
- **Decided:** D391 uses existing geometry/fields and generated/refine, so no schema step; 61 is
  still unused. D392 fixes 45°/135° boundaries; HCM's current lane-group/storage derivation is
  retained pending NA1–NA23 byte parity and B29 (e).
- **Verified:** Linux desktop build (CMake 3.31.6, GCC 14.2.0, Qt 6.8.2 offscreen), CTest
  **248/248** (247 + `junctionlayout`, seven new cases). JT3/JT4 failed before implementation;
  `check` also passes 248/248. `multra-cli --node-analysis` output is byte-identical before/after except
  `buildCommit`. No new UI or Windows appearance claim; headless has no configured build.

## 2026-10-28 — Posted speed on the Link, schema 60 (D390, M14 slice J1)

- **Asked:** "ทำ J1 ต่อเลย".
- **Changed:**
  - A Link's optional `speedLimit` (km/h, > 0, never a cap) and a cross-section template's
    optional `speedLimit` are saved at schema 60.
  - *Apply template* copies the template speed onto the generated Links. A split keeps the speed on
    both pieces.
  - The inspector's *Posted speed (km/h)* sets every selected Link in one Undo step; the
    cross-section dialog has the field too; the object CSV has `speedLimit_kmh`.
  - Node analysis queues read the in-leg Link's speed.
  - A 58–59 approach row's `speedLimit` moves to the Link it names. A row naming no Link, or two
    rows giving one Link different speeds, is refused, so no speed is lost (D380).
- **Decided (D390):** `LinkType.freeSpeed` stays independent, and Micro does not read the posted
  speed yet. Either would move existing results, so each needs its own contract.
- **Verified:** desktop build, `ctest` 247/247 (246 plus `linkspeed`, JT2) and `check` on Linux
  (offscreen); headless preset not run, and no test drives the new inspector field.
  `multra-cli --node-analysis` on the schema-58 fixture prints the same output as before apart
  from the build commit. Windows look is OWNER_SITTING E25.

## 2026-10-28 — UTDF probe prepared for the owner (M14 slice U0)

- **Asked:** "ทำ U0".
- **Found:** Synchro's documentation and PTV's Visum notes were not reachable from the session. A
  Synchro-written UTDF 8 file in the MIT-licensed `utdf2gmns` package (PyPI) gave the real section
  titles, headers and record names; UTDF_EXPORT §3 now lists them with that source. Its `[Network]`
  has no driving-side record.
- **Written:**
  - [UTDF_PROBE](plans/UTDF_PROBE.md): the left-hand reference junction (far and kerb pockets, a
    shared kerb-through lane, volumes, P2 at 100 s), the owner's steps and an answer table.
  - `tests/data/utdf/u0-handwritten.csv`: what MULTRA would write for that junction.
  - OWNER_SITTING I; UTDF_EXPORT §6 and §8.
- **Checked:** a scratchpad script compared the file's cell counts, lanes, storage, volumes and the
  100 s ring sums against the sheet. No code changed; no import has been tried. UT0 waits for the
  owner's golden file.

## 2026-10-28 — Road renamed Street segment, schema 59 (D389, M14 slice SR)

- **Asked:** "เริ่มทำได้เลย" for the rename D387 booked; "Segment" turned out to be the engine's type
  and word, so the owner chose "Street segment".
- **Changed:** schema 59 writes `streetSegments`, `streetSegmentId` and `streetSegmentDirection`;
  files below 59 keep their keys and still read, and each version refuses the other's at its own
  path (JT1). Codes, types, functions, files, tests, CMake test names, locale keys and strings
  (th "ช่วงถนน"), module maps and maintained docs follow; three reference docs renamed.
- **Unchanged:** no behaviour; documents without the renamed keys keep their version and bytes.
- **Verified:** desktop build (Qt 6.4.2, installed this session), `ctest` and `check` on Linux
  (offscreen): `ctest` 246/246 (245 before plus JT1); headless preset not run. Windows look is
  OWNER_SITTING E24.

## 2026-10-28 — M14 redirected: junction templates, transitions, Road → Segment (D387)

- **Asked:** no junction builder; Roads that cross should make the junction, with a junction
  template that sets the turns and makes their Connectors; rename Road to Segment; how Roads of
  different cross-sections join. Owner: "เอาตามแนะนำ" ("go with the recommendations").
- **Written:** JUNCTION_BUILDER became [JUNCTION_TEMPLATES](reference/JUNCTION_TEMPLATES.md): templates
  by leg role holding structure only, site values on the Node, a `junction` fill after F3, transition
  templates aligned per template, `SEAM_JUNCTION_DIFFERS`; slice SR (the full rename, schema 59)
  comes first. UTDF_EXPORT, ROADMAP M14, OWNER_SITTING B30 and NEXT follow. About 13–16 sessions.
- **Found:** Micro has no join for two Roads of different cross-sections today; only Macro and
  Meso join free ends. No code changed.

## 2026-10-28 — M14 booked: junction builder and UTDF export, contracts only (D386)

- **Asked:** could the editor build an HCM network first and export it to PTV Vistro; then
  Synchro too (confirmed: Vistro 2026, Synchro 12); review the network tools; "อนุมัติ ช่วยเขียน
  รายละเอียดก่อน" ("approved, write the details first").
- **Found:** a four-leg HCM junction needs six manual steps; Road + *Find crossings* + F4 is fast
  but shares lanes kerb to far with no pocket, so HCM lane groups and UTDF lanes cannot come from
  it; no cross-section template ships; there is no posted speed on the Link, no kerb-side pocket,
  no integer Node number and no NEMA phase numbering.
- **Written:** [JUNCTION_BUILDER](reference/JUNCTION_TEMPLATES.md) and
  [UTDF_EXPORT](reference/UTDF_EXPORT.md), ROADMAP M14, OWNER_SITTING B30. UTDF's field list is
  provisional until U0 reads files the owner's Synchro 12 and Vistro 2026 write and import;
  left-hand traffic in both is the main open risk. No code changed.

## 2026-10-28 — Road workflow: Apply on several Roads, crossings warning (D385)

- **Asked:** batch D of the road-model review, on the D382–D384 branch, whose PR is still open.
- **Changed, each with a `road-ui` row that failed first:** *Apply template* takes every selected Road
  in table order in one Undo step, and one refusal leaves the batch unapplied (RD34; 200 Roads about
  150 ms in a Debug desktop build); *Find crossings* marks a place whose Road has neither template
  nor Links, with a tooltip and one note, without blocking Confirm (RD35; en/th strings).
- **Docs:** ROADS RD34–RD35, ROADS_IMPLEMENTED §24, OWNER_SITTING E24; decision index gains D382–D385.
- **Verified:** Qt 6.4.2 desktop build, `ctest` 245/245 and `check` on Linux (offscreen); Windows
  and screen scale are OWNER_SITTING E24.

## 2026-10-28 — Road-scale edits made fast, results unchanged (D384)

- **Asked:** "ทำชุด C ต่อเลย" ("go on with batch C") of the road-model review.
- **Changed, each measured before and after (Release, median of three):** `applyFills` checks only the
  offer it applies (F3 on 480 Roads 96.3 s → 80 ms); `polygonRuns` skips a line whose box is over
  1 m from the polygon's (`settleNodes` 466 → 118 ms); `nodeIssues` tests overlap only where boxes
  meet (744 → 1.4 ms). One edit on 1,860 Roads: 1.24 s → 121 ms.
- **Same results:** characterization rows passed before and after; documents, issues and settled
  results compared byte for byte at each step. Numbers and fixture in ENGINEERING_NOTES.
- **Verified:** headless build, `ctest` 164/164 and `check` on Linux.

## 2026-10-28 — Node-analysis inputs follow ordinary edits (D383)

- **Asked:** "ทำชุด B ต่อเลย" ("go on with batch B") of the road-model review.
- **Fixed, each with a row that failed first:** splitting an approach dropped three turns' volumes
  while Node delay was still shown (NA21: rows now follow the in-leg's end); rows naming no turn or
  in-leg were ignored silently (NA22: `NODE_ANALYSIS_STALE_TURN` / `_STALE_APPROACH`); a polygon
  taking in the pockets read as needing detail (NA23: `NODE_CONNECTOR_CROSSES` is advised instead).
- **Left:** totals under `NODE_ANALYSIS_PARTIAL` stay shown until the owner answers B29 (d);
  batches C and D in NEXT.
- **Verified:** headless build, `ctest` 164/164 and `check` on Linux.

## 2026-10-28 — Road-model review: four defects fixed (D382)

- **Asked:** review the road network model, its workflow and its use by HCM; then "เริ่มชุด A ได้เลย"
  ("start batch A") on the revised plan.
- **Fixed, each with a row that failed first:** Find crossings grouped places across levels (RD30);
  `splitRoad` left a far coarse Node naming the old Road and was refused (RD31); F4 listed a Node
  narrower than its arms, then failed on a gap, now `FILL_NODE_ARM_OUTSIDE` (RS19); a detach made the
  backward Link read as forward, so re-apply turned it around (RD32, RD33).
- **Crosses the Road layer:** the direction fix also changes the lane-count seam and F4's template
  geometry. ARCHITECTURE no longer calls Road and Node planned.
- **Left:** batches B–D and the dialog warning in NEXT; six owner choices in OWNER_SITTING B29.
- **Verified:** headless build, `ctest` 164/164 and `check` on Linux. No Qt here: no desktop claim.
