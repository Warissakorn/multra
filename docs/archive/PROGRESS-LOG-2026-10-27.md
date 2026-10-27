# PROGRESS archive — 2026-10-27

Whole session entries moved from [PROGRESS](../PROGRESS.md).

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
