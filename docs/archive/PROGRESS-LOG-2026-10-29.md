# PROGRESS archive — 2026-10-29

Whole entries moved from [PROGRESS](../PROGRESS.md); historical, not the current queue.

## 2026-10-29 — J2 review: preserve authored ambiguity and stop pockets at Nodes (D391)

- **Asked:** fix the two J2 review items, update D391, verify and push; stop again before J3a.
- **Fixed:** only `addTurnPocket`'s still-generated 1→2 widening skips its zero-source-width
  path in an implied Link-to-Link step. Hand-drawn/refined widenings retain pre-J2 ambiguity;
  explicit Connector selection remains unchanged.
- **Fixed:** `pocketOf` stops before a Connector with an inside run in any Node, including a
  straight-through movement. A pocket lane upstream of a junction is not a pocket at the next.
- **Rows:** both new JT3 regressions failed first after asserting their setup: a hand-drawn 1→2
  really has a zero-width source path; the straight continuation really belongs to the first
  Node and that Node's pocket exists. Both driving sides, and kerb/far pockets, are covered.
- **Verified:** desktop build and focused suite 9/9 pass on Linux (GCC 14.2.0, Qt 6.8.2 offscreen).
  Full CTest and `check` pass 248/248 each. CLI Node analysis is byte-identical apart from `buildCommit`.
  No new UI or fields; headless is unconfigured and Windows was not run.

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
