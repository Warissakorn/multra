# PROGRESS archive — 2026-10-27

Whole session entries moved from [PROGRESS](../PROGRESS.md).

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
  [archive](ROADMAP-implemented-2026-10-27.md); ROADMAP is 494 lines. No status, gate or
  decision changed.
- **Not done:** the owner-sitting R4 (b) carve-out of the items listed more than 14 days ago is the
  owner's call, and was left as it is.

## Older entries

Older entries are in [`archive/`](README.md), newest first. Decisions live in the
[decision record](../decisions/RECORD.md); the M0 backlog and question register are
[archived](PROGRESS-M0-backlog-and-questions.md).

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
