# PROGRESS archive — 2026-10-29

Whole entries moved from [PROGRESS](../PROGRESS.md); historical, not the current queue.

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
