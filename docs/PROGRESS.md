# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-10-28 — M14 booked: junction builder and UTDF export, contracts only (D386)

- **Asked:** could the editor build an HCM network first and export it to PTV Vistro; then
  Synchro too (confirmed: Vistro 2026, Synchro 12); review the network tools; "อนุมัติ ช่วยเขียน
  รายละเอียดก่อน" ("approved, write the details first").
- **Found:** a four-leg HCM junction needs six manual steps; Road + *Find crossings* + F4 is fast
  but shares lanes kerb to far with no pocket, so HCM lane groups and UTDF lanes cannot come from
  it; no cross-section template ships; there is no posted speed on the Link, no kerb-side pocket,
  no integer Node number and no NEMA phase numbering.
- **Written:** [JUNCTION_BUILDER](reference/JUNCTION_BUILDER.md) and
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

## 2026-10-28 — Stall-level parking and pedestrians recorded as needs (D381)

- **Asked:** would a posted speed on the Link limit Vissim-style site roads, parallel and bay
  parking and pedestrians; then "ทำเลย" ("do it") to the offered docs change.
- **Changed (docs only):** two PROBLEM §2 Micro rows, two ROADMAP *Later* entries, and the three
  posted-speed rules in NEXT for the 1a contract. Limits recorded in D381: no stall objects, no
  reverse motion in core, no pedestrian model, Connector topology and scale.

## 2026-10-28 — B28 decided on delegation (D380)

- **Asked:** the owner answered B28 with "ช่วยตัดสินใจ" ("help decide"), after the session said item 1
  reverses D373.
- **Decided:** 1a, one posted speed on the Link, the approach speed derived with every stored value
  migrated; 2a, D378's order rule reworded so a finer level's gate does not close before the coarser
  one it depends on. PROBLEM §3 and MULTI_LEVEL_MODELLING §7 carry the new wording. Reasons in D380.

## 2026-10-28 — Review of D378/D379: corrections and B28

- **Asked:** `/scrutinize`, then the plan was approved.
- **Corrected:** D379 said `seam_checks.cpp` compares two speed homes; it compares only adjacent
  link types' `freeSpeed`. D379 now states `freeSpeed`'s Macro/Meso role and leaves its relation to
  the posted speed to the 1a contract.
- **Returned to the owner (B28):** 1a, because it reverses the owner's own D373 answer; and D378's
  order rule, which the current Meso and Micro work already departs from. PROBLEM §3 names those
  exceptions; §2 says the Aimsun facts come from search excerpts.

## 2026-10-28 — B27 decided on delegation (D379)

- **Asked:** the owner answered B27 with "ช่วยตัดสินใจ" ("help decide"), as for B19 and B25.
- **Decided:** 1a one posted speed on the Link, read by every level and M13 (D373's approach field
  becomes derived); 2a per-lane allowed modes; 3a *Fill from model* for Node turn volumes; 4a only
  within an HCM 7 planning-level method, read before the contract, else back to the owner. Reasons
  are in D379. Bookings only: no slice starts until the owner asks.

## 2026-10-27 — Link, Connector and Node reviewed; stale audit rows fixed

- **Asked:** "รีวิว link connector และ node" ("review Link, Connector and Node"), then "ทำ A" ("do A").
- **Found:** speed stored in three places, no per-lane vehicle restrictions, M13 turn volumes typed
  by hand, and M13 refusing coarse Nodes. These are put to the owner as OWNER_SITTING B27; nothing
  is decided here. Gaps already booked (M1.22 geometry, M3.4/M6 lane-change distance, the mouth rule
  D80) stay where they are.
- **Changed (docs only):** VISSIM_PARITY's Nodes row no longer says "Absent entirely";
  CONNECTOR_PARITY_AUDIT §2 is marked as a 2026-09-21 baseline with a 2026-10-27 recheck.
