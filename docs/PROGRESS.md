# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## 2026-10-27 — Later M13 slices outlined; A3's choices put to the owner

- **Asked:** whether contracts should be written ahead. Answer given and accepted ("ทำตามข้อเสนอ"):
  a full contract only for the next slice; later slices get an outline and their open questions,
  no formulas or values; questions known ahead go to OWNER_SITTING for one sitting.
- **Changed:** NODE_ANALYSIS §12 outlines A3–A9 with their questions. A3's five (scope, recognising
  protected-permitted operation, opposing flow, pedestrians and bicycles, acceptance) are
  OWNER_SITTING B25. §7 now cites A3's real source, Ch. 31 §3 Steps 1–7 (PDF p. 2986–3015), not §2.
  No D-row: nothing is decided, and A3 still starts only when the owner asks.

## 2026-10-27 — Module READMEs rewritten as maps, and checked

- **Asked:** whether to document every program detail per component. Answer given: no; the
  contracts already own behaviour, and a second detailed copy would drift. The gap was the module
  READMEs, which read as decision histories; the owner approved rewriting them.
- **Changed:** every `src/*/README.md` (and `src/model/demand/`, `src/model/control/`) now holds
  purpose, dependency rule, a file table (file, what it holds, contract), entry points and
  tests, with no status or decision history. Substantive rules kept: eval's observer rules,
  project's codec rules. A review then dropped a hand-written "used by" column (nothing could
  keep it true) and corrected a claimed editor/shell boundary breach: they are one target.
- **Check:** `multra-check-docs` fails when a header under `src/` is missing from its module's
  README as the start of a table row (nearest parent README for a folder without one; a prose
  mention does not count), with positive and negative fixtures.
- **Verified:** the docs, architecture and file-size checks pass; removing one row fails the docs
  check as expected.

## 2026-10-27 — Analytical junction capacity, slice A2 (M13, D373, D374)

- **Asked:** continue with A2, and finish it. The owner answered A2's three questions: all of it in
  PR #200; critical v/c over a non-overlapping chain; a speed limit stored per approach (D373).
- **Changed:** `src/analysis/` gains `lane_flow` (Ch. 31 §2's lane-flow procedure, protected),
  `critical` (Xc over the largest-Σy chain of non-overlapping greens) and `queue` (Ch. 31 §4: Q1, Q2,
  Q95, storage ratios). Lane groups are lane-based, so shared through+kerb lanes are analysed.
  Approaches gain `speedLimit` (schema 58). Eqs. 31-46 to 31-66, 31-131 to 31-156 and 19-30/19-31
  were read from their images; Eq. 31-151's clipped image is completed from the PDF text layer.
- **Exhibit 19-41:** 29 of 36 cells within ±5 %; the seven others (three are the capped-looking
  "2,000" entries) are listed in NODE_ANALYSIS §11, not tuned. Prototyping caught a conservation
  bug (Eq. 31-64 must distribute the trading groups' own demand) before it shipped.
- **Verified:** Linux Debug ctest passes, including NA9–NA14 and the CLI fixture now carrying all
  twelve turns: node 59.86 s (E), Xc 0.726, West sr v/s equal to t's 0.1513, a pocket queue's R_Q
  0.754, each hand-checked.

## 2026-10-27 — Analytical junction capacity, slice A1 (M13, D372)

- **Changed:** a new pure engine `src/analysis/` computes HCM 7 Ch. 19 for pretimed, protected,
  exclusive lane groups: Eq. 19-8's factors, capacity, v/c, d1/PF, d2, flow-weighted approach and
  intersection delay, and Exhibit 19-8 letters from the new `los/hcm7-signalised.json`.
  `compileNodeAnalysis` derives lane groups from a Node's Micro detail and reads timing from the
  existing programs. Nodes store only volumes and adjustments (`analysis`, schema 58), and the CLI
  gains `--node-analysis`. Results are labelled analytical and not validated.
- **Read from the copy:** Eqs. 19-9 to 19-14 and Exhibits 19-11, 19-15 and 19-20, from their images.
  Exhibit 19-41 assumes shared right-turn lanes, so its reproduction is carved out to A2.
- **Verified:** Linux Debug ctest 245/245 (`nodeanalysis` NA1–NA8 plus an end-to-end four-leg run:
  pocket turns D, node 46.69 s; hand-checked); architecture, file-size and docs checks pass. All four
  shipped projects' `--project 42` outputs are identical to the Release build of the previous commit.

## 2026-10-27 — Analytical junction capacity booked as M13 (D370)

- **Asked:** should MULTRA analyse junctions as Vistro, Synchro and SIDRA do? Answer approved by the
  owner: yes, as a deterministic, flow-based method on the Node (macroscopic at junction scale), a
  fourth method beside Micro, Meso and Macro, using published methods only (HCM 7, Webster).
- **Contract** [NODE_ANALYSIS](reference/NODE_ANALYSIS.md): A1 is HCM 7 Ch. 19 Steps 1–9 for pretimed
  protected exclusive lane groups. Eqs. 19-8, 19-19, 19-20 and 19-26 and Exhibit 19-8 were read
  from the owner's copy (`74f4bc3`). Also slices A0–A9, rows NA1–NA8, and nine owner choices with
  recommendations. Chapter 31's Example Problem 1 is actuated and protected-permitted, so it is
  reserved for A3 + A8.
- **Owner answer (D371):** "ใช้ตามแนะนำทั้งหมด", every recommendation 1a–9a. A1 derives lane groups
  from the Node's Micro detail rather than storing them.
