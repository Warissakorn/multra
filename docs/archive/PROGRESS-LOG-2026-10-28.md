# PROGRESS log — 2026-10-28

Whole entries moved from [PROGRESS](../PROGRESS.md); historical, not the current queue.



## 2026-10-28 — M14 booked: junction builder and UTDF export, contracts only (D386)

- **Asked:** could the editor build an HCM network first and export it to PTV Vistro; then
  Synchro too (confirmed: Vistro 2026, Synchro 12); review the network tools; "อนุมัติ ช่วยเขียน
  รายละเอียดก่อน" ("approved, write the details first").
- **Found:** a four-leg HCM junction needs six manual steps; Road + *Find crossings* + F4 is fast
  but shares lanes kerb to far with no pocket, so HCM lane groups and UTDF lanes cannot come from
  it; no cross-section template ships; there is no posted speed on the Link, no kerb-side pocket,
  no integer Node number and no NEMA phase numbering.
- **Written:** [JUNCTION_BUILDER](../reference/JUNCTION_TEMPLATES.md) and
  [UTDF_EXPORT](../reference/UTDF_EXPORT.md), ROADMAP M14, OWNER_SITTING B30. UTDF's field list is
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

## 2026-10-28 — Stall-level parking and pedestrians recorded as needs (D381)

- **Asked:** would a posted speed on the Link limit Vissim-style site roads, parallel and bay
  parking and pedestrians; then "ทำเลย" ("do it") to the offered docs change.
- **Changed (docs only):** two PROBLEM §2 Micro rows, two ROADMAP *Later* entries, and the three
  posted-speed rules in NEXT for the 1a contract. Limits recorded in D381: no stall objects, no
  reverse motion in core, no pedestrian model, Connector topology and scale.
