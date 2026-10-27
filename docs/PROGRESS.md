# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-10-27 — Module READMEs rewritten as maps, and checked

- **Asked:** whether to document every program detail per component. Answer given: no; the
  contracts already own behaviour, and a second detailed copy would drift. The gap was the module
  READMEs, which read as decision histories; the owner approved rewriting them.
- **Changed:** every `src/*/README.md` (and `src/model/demand/`, `src/model/control/`) now holds
  purpose, dependency rule, a file table (file, what it holds, contract, used by), entry points and
  tests, with no status or decision history. Substantive rules kept: eval's observer rules,
  project's codec rules. Callers were derived from resolved `#include` paths. The survey found
  `src/editor/` reading three shell and one commands header, now stated in its README.
- **Check:** `multra-check-docs` fails when a header under `src/` is missing from its module's
  README (nearest parent README for a folder without one), with positive and negative fixtures.
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

## 2026-10-27 — Measured optimization pass: graph-header fan-in, doc sizes

- **Measured first** (Linux, 4 cores): the engine runs M2.6's hour in 0.40–0.45 s (Release), editor
  redraw 0.6–0.9 ms, Run Step 2.7 ms, Node detail 0.65–0.78 s; none needed work. A Debug rebuild
  after touching `core/types.hpp` is 540 of 575 TUs (12.5 min); its split stays with the first
  engine slice that edits `Vehicle`/`SimState` (NEXT).
- **Build:** `MacroSettings`/`MesoSettings` move to `macro/settings.hpp`/`meso/settings.hpp`, so the
  authoring definition no longer includes the graphs. Touching `macro/graph.hpp` rebuilds 44 TUs
  (was 456), `meso/graph.hpp` 48 (was 462). No behaviour change; ctest 242/242.
- **Docs:** AGENTS gives RECORD's real size (≈95k tokens); ROADMAP M3.2.8a.1's history moves whole
  to [archive](archive/ROADMAP-implemented-2026-10-27.md#m328a1-clamp-history-d105d258)
  (ROADMAP −2,628 characters, still 494 lines).

## 2026-10-27 — M0 plausibility accepted by owner ruling (D369)

- **Owner ruling:** "ผ่าน แบบ A" — pass, with the amber caveat on the record. The owner holds the
  gate; the session measured and compared, and recorded the answer.
- **Evidence** ([note](evidence/m0-plausibility.md)): `crossing.json`, seeds 42–51, against SUMO
  1.18 Krauss and IDM with the same geometry, timing, demand and vehicle data. MULTRA discharges at
  1.83 s (1,967 veh/h; IDM 1.89 s, Krauss 1.42 s), stands at 7.24 m front to front, and has no
  red passes. Its 1–3 clamps and 6–30 full-braking ticks per run all fall on amber within 40 m of
  the line, because a bare M0 scenario holds amber as red (D147).
- **Changed:** ROADMAP M0 is closed; OWNER_SITTING A1 is removed; M1_ACCEPTANCE, M2_PLAN G2, M3_ACCEPTANCE
  and NEXT now say the same. The unvalidated marker (D13) stays; no code changed.

## 2026-10-27 — Documentation review fixes: owner items, a lost fix, ROADMAP headroom

- **Owner items in one place:** NEXT carried Windows/owner checks that OWNER_SITTING, the only
  owner list, did not: D306 (now E23), D318 (F4), D333/D334 (C13), D335/D336 and D344–D350 (new
  section H). Each carries its decision date as *listed*. NEXT now points to them. MAP10's extra
  steps from NEXT join E11, and G1 asks about D368's leading `'`. No verdict was recorded.
- **Lost booking restored:** D201 B3 queued a fix for `automatic_conflicts.cpp` skipping pairs on
  different drawing levels against M3_CONTRACT §1. No queue carried it, and a test still asserts the
  skip. NEXT books it again, failure first.
- **Contract gap:** DESKTOP_AUTOMATION now documents `simulation.cancel` and the `cancelled`/`playback`
  run-until outcomes the code already reports.
- **Headroom:** ROADMAP was at 499 lines with one 3,846-character status paragraph. The S3–S5
  paragraph and the M8 Road-demand follow-ups move whole to
  [archive](archive/ROADMAP-implemented-2026-10-27.md); ROADMAP is 494 lines. No status, gate or
  decision changed.
- **Not done:** the owner-sitting R4 (b) carve-out of the items listed more than 14 days ago is the
  owner's call, and was left as it is.

## 2026-10-27 — Authored network PNG export (M1.23.2, D367)

- **Changed:** File menu and command palette export the whole visible network or current view
  at chosen pixel dimensions, with optional backgrounds and English/Thai controls. An isolated
  canvas reuses authoring display/level/wireframe rendering at output scale, excluding grid,
  selection, drafts and Run overlays. Whole-network output is independent of live camera zoom.
- **Safety and scope:** bounded dimensions precede allocation; encoding precedes atomic file
  replacement. Shared CSV/PNG destination protection preserves the current project/recovery.
  Embedded images retain orientation/opacity. Online maps use only already-loaded tiles, with
  attribution/URL in a footer and PNG metadata; export neither downloads nor moves the camera.
- **Verified:** Release desktop application and four selected UI targets build; network-image-ui,
  object-export-ui, workspace-ui and results-tabs-ui pass on Linux offscreen Qt 6.8.2. The PNG
  suite also passes after adding camera-independent output coverage. Release headless build and
  all 161 CTest entries pass (10.70 s); docs, file sizes and diff checks pass. Unicode paths,
  cancellation, size/protected-file refusals, embedded image orientation and cached-map/no-new-
  request behavior are covered. Packaged Windows/high-DPI appearance and owner acceptance
  remain open, as do other M1.23 formats/workflow and the full milestone gate.

## 2026-10-27 — Pin the bundled UI font against installed-family collisions (D366)

- **Found:** this host's installed Noto Sans Thai has no ASCII digits, while the bundled cut
  does. Qt 6.8.2 matched the installed same-name family and used proportional fallback digits.
- **Fixed:** renamed only bundled font naming tables to the private MULTRA Sans Thai family.
  Every glyph, metric and layout table is byte-identical. The existing workspace assertion now
  compares with the application's data-loaded family, retaining equal digit advances, 12 px
  and real 600-weight checks. No font fallback or disabled assertion was added.
- **Verified:** Release desktop target builds; workspace-ui passes (7.97 s) on Linux offscreen
  Qt 6.8.2 with the conflicting system font still installed. Font-generation instructions are
  updated; Windows appearance and owner gates remain separate.

## 2026-10-27 — Authored object table CSV export (M1.23.1, D365)

- **Asked:** switch from schema/AI work to the recommended M1.23 native object-table CSV slice.
- **Changed:** File → Export object table (CSV), also in the palette, exports Links, Connectors,
  inputs, routes and decisions from the current document without Run/catalog resolution. A
  Qt-free formatter shares persisted serialization on table-only projections, keeps full numeric
  precision, UTF-8 BOM/CRLF, authored row order and nested JSON cells. The shell selects Unicode
  paths, writes atomically and protects project/recovery destinations without changing History,
  revision, dirty/save state or results. A UI test caught nonexistent paths comparing equal in
  QFileInfo; protection now compares cleaned absolute names and nonempty canonical paths.
- **Verified:** five native regressions cover known 3–4–5 geometry, precise values, quoting/Thai,
  optional attachments, nested demand/counts, empty tables/order and locale independence; project
  tests pass 70/70. Twenty sample CSVs from all four shipped projects pass independent Python
  CSV/JSON checks. Release headless build and all 161 CTest entries pass (14.09 s). A local Qt
  6.8.2 SDK enabled a Release desktop build and object-export-ui (all five actions, unsaved edits,
  Unicode paths, cancellation, failed writes, protection and translation) plus results-tabs-ui pass.
- **Limit:** workspace-ui fails its existing numeric-font/tabular-digit assertion under this
  Qt environment; a separately linked pre-D365 workspace reproduces the same failure. A direct
  Qt font probe also disagrees with the bundled TTF's equal digit metrics. No test was weakened;
  the issue is recorded in NEXT. Windows/packaged acceptance is OWNER_SITTING G1. This slice
  closes no full M1.23, owner, runtime-model or scientific gate; other interchange formats remain.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
