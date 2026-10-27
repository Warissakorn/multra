# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## 2026-10-27 — RoutingDecision structural schema and unchanged counted synchronization (D364)

- **Changed:** shared metadata now covers RoutingDecision rows, route shares, periods and
  type-rule numeric matrices. Their native decoder moves to routing_codec; the parser is now
  222 lines. The composed partial schema/CLI uses 73 shared definitions and 16 version profiles.
  The schema and document precheck share the position version floor. Envelope-only schema and
  desktop observed discovery stay unchanged.
- **Preserved:** always-strict decision/share/type keys, open period keys, nullable optionals
  and contains-based routingDecisions/typeRules null refusal. The document-only pre-20 position
  guard still rejects null before normal decoding; bare/standalone reads retain support.
  Numeric-vector, matrix-row and matrix-cell diagnostics retain their original order/paths.
  Counted synchronization stays after complete definition decoding; model validators retain
  matrix dimensions, weights, time windows and destination/run-readiness checks.
- **Verified:** 5,073 cases across schemas 1–57 retain byte-identical native outcomes/errors/
  canonical documents and pass independent Draft-2020-12 expectations. Four shipped projects
  validate structurally and keep byte-identical canonical output. Five new regression tests
  cover strict/open shapes, nulls, position guards, matrix errors/semantics and counted sync;
  an independent review found no compatibility defect. Release headless build and all 161
  CTest entries pass (16.23 s), including CLI, docs, architecture and file-size checks. Linux
  headless evidence only; no engine algorithm, Qt/Windows, scientific or owner gate change.

## 2026-10-26 — Base Route/VehicleInput schema and preserved demand derivation (D363)

- **Changed:** shared metadata now covers definition duration/timeStep, Routes, VehicleInputs
  and volume intervals. Demand readers move to demand_codec; the parser is now 263 lines.
  The composed partial schema/CLI uses 67 shared definitions and 15 version profiles. Schema
  and document prechecks share the counted-volume version floor; the envelope-only API and
  desktop observed response stay unchanged.
- **Preserved:** open Route/input/interval keys, contains-based null refusal, scalar-before-
  interval diagnostics, interval-derived totals and empty-period behavior. Route migration and
  counted-volume synchronization retain their stages after full definition decoding; bare
  standalone reads retain their existing behavior. No catalog ownership, demand compilation,
  simulation dynamics, writer version-selection or serialized-output change.
- **Verified:** 6,156 cases across schemas 1–57 have byte-identical before/after native outcomes,
  errors and canonical documents and pass independent Draft-2020-12 expectations. Four shipped
  projects validate structurally and retain byte-identical canonical output. Five new tests
  cover required/open fields, nulls, integer-independent numeric types, scalar derivation,
  counted synchronization/version guards, route migration and semantic exclusions. Release
  headless build and all 161 CTest entries pass (11.59 s), including project, CLI, docs,
  architecture and file-size checks. Linux headless evidence only; no Qt/Windows, scientific
  validation or owner gate is closed.

## 2026-10-26 — Queue-counter and travel-time-section structural schemas (D362)

- **Changed:** shared metadata covers queue counters/measurement lines and travel-time sections/
  section lines/controlType. Native readers move to measurement_codec at the original parser
  stages; control points/paths share the right-of-way metadata. The composed partial schema/CLI
  uses 62 shared definitions and 14 version profiles; the parser is now 290 lines. The old
  incidental definition-count bound is replaced by an actual shared-ControlPoint reference check.
- **Preserved:** schema 16/23/24 gates, nullable network arrays and optional names/sources/types,
  empty-source decoding of non-object measurement lines, reference-before-point and section-
  lines-before-controlType diagnostics. Exactly-one-source, owner/reference and section bounds/
  order checks remain model validation; measurement/LOS algorithms and writer rules do not change.
  The envelope-only API, observed desktop response and existing serialized output are unchanged.
- **Verified:** 5,928 cases across schemas 1–57 retain byte-identical native outcomes/errors/
  canonical documents and pass independent Draft-2020-12 expectations. Four shipped projects
  validate structurally and keep byte-identical canonical output. Five new project tests cover
  gates/nulls, shared points, empty/ambiguous sources, diagnostics, save versions and semantic
  exclusions. Release headless build and all 161 CTest entries pass (16.64 s), including CLI,
  architecture, documentation and file-size checks. Linux headless evidence only; no Qt/Windows,
  scientific-validation, LOS-validation or owner gate is closed.

## 2026-10-26 — Right-of-way structural schema and preserved legacy reads (D361)

- **Changed:** shared metadata now covers waiting lines, control points/paths, conflict areas/
  sides, priority rules and Stop/Yield controls. Their native reads move to right_of_way_codec;
  queue counters reuse the same path decoder. The composed partial schema/CLI uses 56 shared
  definitions and 13 version profiles. The parser is now 319 lines; no geometry/engine changes.
- **Preserved:** schema 14/15 gates, optional nullable collections/names, legacy non-object
  right-of-way containers and path decoding, authored enums and generated:true-only from 54.
  Waiting-line point-before-id diagnostics and generated checks after each row keep their order.
  Semantic owner/path/line/area checks, bounds, positive gaps, priority cycles and run-readiness
  remain existing validators. The envelope-only API and desktop observed response are unchanged.
- **Verified:** 12,967 cases across schemas 1–57 retain byte-identical before/after native
  outcomes, errors and canonical documents and pass independent Draft-2020-12 expectations.
  All four shipped projects validate structurally and keep byte-identical canonical output.
  Five new project tests cover gates/nulls/enums, legacy paths, diagnostics/generated ordering,
  semantic exclusions and structural refusals; project tests pass 50/50. Release headless build
  and all 161 CTest entries pass (13.10 s), including docs, architecture, CLI and file-size checks.
  Evidence is Linux headless only; no Qt/Windows, Vissim, scientific or owner gate is closed.

## 2026-10-26 — Signal control structural schema and codec parity (D360)

- **Changed:** shared field metadata now covers definition signal-array presence, fixed-time
  controllers/groups and legacy programs/phases. Their readers move to signal_control_codec;
  the composed partial schema/CLI includes them using 41 definitions and 12 version profiles.
  The envelope-only API, writer version selection and desktop observed discovery stay unchanged.
- **Preserved:** controller/group keys stay strict at every version, program/phase keys stay
  additive, nullable names and integer-valued float groups retain existing conversion. The
  pre-54 generated guard, generated-value error order and pre-13 project migration keep their
  original stages; bare M0 programs remain legacy. Timing, uniqueness, reference resolution and
  run-readiness remain existing semantic checks. No engine or runtime signal algorithm changes.
- **Verified:** 6,669 cases across schemas 1–57 have byte-identical before/after native results,
  diagnostics and canonical documents, and pass independent Draft-2020-12 expectations. Four
  shipped projects pass structural validation and retain byte-identical canonical output.
  Five new project tests cover presence/strictness, numeric/null/error ordering, version guards,
  migration and semantic exclusions; project coverage is 45 tests. Release headless build and
  all 161 CTest entries pass (15.15 s), including CLI, docs, architecture and file-size checks.
  Evidence is Linux headless only; no Qt/Windows, scientific-validation or owner gate is closed.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
