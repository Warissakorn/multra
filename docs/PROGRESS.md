# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## 2026-10-27 — Purpose amended: Micro, Meso and Macro weighted equally (D378)

- **Asked:** after a review of the purpose, "ต้องการให้น้ำหนัก micro meso Marco เท่ากัน" ("I want
  Micro, Meso and Macro weighted equally") and "ศึกษาจากโปรแกรม aimsun next ด้วย" ("study from the
  Aimsun Next program as well"); then, by AskUserQuestion: the study-chain order, M13 as a Node
  method serving the three levels, and Aimsun as a reference in PROBLEM only.
- **Changed (docs only):** PROBLEM §1–§8 (two equal user roles, needs per level, the Node method,
  Aimsun Next's integrated model cited from its public docs, a done sentence per level and the Node,
  non-goals for `.ang` files and proprietary formulas, a per-level §7, §8 question 2); PRINCIPLES
  rule 8 (each level's vocabulary); MULTI_LEVEL_MODELLING §7, ROADMAP M8–M12 and M13, NEXT's order,
  README's lead and OWNER_SITTING B26. No code, gate or done-condition changed.
- **Open for the owner:** B26, a simultaneous hybrid meso–micro run, and the user-role wording.

## 2026-10-27 — M13 A3: permitted and protected-permitted far turns (D376, D377)

- **Asked:** "ทำ A3" ("do A3"), with D375's choices.
- **Contract (D376):** NODE_ANALYSIS_A3, from images of Ch. 31 §3 (Eqs. 31-94 to 31-129, Exhibits 31-12
  to 31-19) and §2's permitted terms. Exhibit 31-12's nine phase-sequence rows turned out to be one
  rule, G_U = E − max(S_a, S_o + G_q2), once its phases are labelled (2 the opposing through).
- **Built (D377):** a periodic queue polygon, the permitted formulas, the compile's per-turn windows,
  and a Node run that iterates the opposing queues. Protected results are unchanged.
- **Open for the owner:** l+sl, lr and one-lane layouts with a permitted far turn are refused (the
  book prints no form with a kerb turn in the same lane), and permitted lane groups have no queue yet.
- **Verified:** NA15–NA20 pass; NA19's permitted row is hand-checked (c 170.01, d1 55.29, d2 59.00).

## 2026-10-27 — M13 A3's choices decided on delegation (D375)

- **Asked:** the owner answered B25 with "ช่วยตัดสินใจ" ("help decide"), as for B19 (D214).
- **Decided:** 1a–5a. Shared and exclusive lanes, far and kerb turns; protected-permitted derived
  from green overlap with a yielded conflict (the test A2 already uses); opposing flow from Node
  membership; no pedestrians or bicycles, stated; hand rows, with Example Problem 1 left to A3 + A8.
  Reasons are in D375. A3's contract and code still start only when the owner asks.
