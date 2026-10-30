# PROGRESS archive — 2026-10-30

Whole entries moved from the live PROGRESS log.

## 2026-10-30 — J5 transition implementation (D406)

- **Owner B34:** taper entirely upstream, Transition-only selector in J5, two symmetric
  installed alignments and owned mixed directions. J4T remains documentation only (D405).
- **Implemented:** strict transition catalog shape and schema-63 field guards; old recipe
  fields keep their existing floors. Differing end-to-start pairs are Transition rows; the
  selector starts empty and Confirm needs a choice. Confirm and fill each use one Command.
- **Geometry:** cut the incoming arm at its entered centreline taper, keep the outgoing start,
  map aligned lanes one-to-one, then merge/diverge the extra lanes on the opposite side.
  Existing merge order/defaults, generated/refine and named reference protection apply.
  No Node kind/layout is duplicated, no site length is defaulted, no core/HCM method changes.
- **Tests first:** the first three JT10 cases failed on the loader, missing row and fill.
  Extended cases cover both driving sides, mixed alignment, one-way/unused site values,
  bounds/missing values, generated replacement/refine, stationed heads and full JSON Undo.
- **Docs/UI:** PROJECT_SCHEMA, street workflow, module maps and OWNER_SITTING E28 updated.
  Street segment/transition Qt test registration was split before the 500-line limit.
- **Fixture corrections:** allocate new Node ids through the API; a detailed two-leg layout
  still has approaches, while HCM refuses its unsignalised control. Micro compilation needs
  inputs and emits conflict zones for authored merges, not legacy priority rules.
- **Verification:** desktop build, native JT10 **9/9**, and the new Qt selector test on both
  driving sides pass. CTest **253/253** (253.04 s) and `check` **253/253** (241.42 s) pass. CLI output
  matches before/after except the single `buildCommit`. Headless is not configured. Linux,
  GCC 14.2.0/Qt 6.8.2 offscreen; Windows/manual UI were not run (E28). Stop for J5 review.

## 2026-10-30 — J5 preparation and signalised T booking (D405)

- **Asked:** J4 5ab3ea3 approved; start J5 transitions, then stop for review. Add B33 for
  signalised T templates as documentation only, proposing a small slice before U1.
- **B33 answered:** one protected three-stage preset first, retaining J4's compass NEMA
  numbers, omitting unused phases and keeping the 2/6 zero anchor. Recorded D405 and booked
  J4T before U1 with its own contract/rows and review; no T code is added during J5.
- **Before transition code:** re-read §7/JT10, D387/D388, crossings, lifecycle, F4 cuts and
  generated rules. §7 does not fix where taper is placed or how template selection crosses
  the J5/J6 boundary. B34 asks those choices and the initial directional catalog/structure,
  with concrete recommendations; no answer is assumed. JT10 remains unimplemented and 63 unused.
- **Verification:** desktop build and standalone documentation, file-size, architecture and
  whitespace checks pass on Linux. CTest, the full `check` target and headless were not run
  for this documentation-only checkpoint. No Windows or manual UI run; no new UI or stored field.
  The build stays green at the approved J4 implementation while B34 is pending.
