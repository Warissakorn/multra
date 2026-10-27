# PROGRESS archive — 2026-10-27

Whole session entries moved from [PROGRESS](../PROGRESS.md).

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
