# PROGRESS archive — 2026-11-03

Whole entries moved from the live PROGRESS log.

## 2026-11-03 — drawing level is not separation (D436, D201 B3)

- **Owner request:** continue NEXT. Every other open line waits on the owner, so this session took the
  booked fix. Owner choice: separation is a confirmed overpass, or Street segments on different levels.
- **Change:** `automaticConflicts` no longer skips path pairs because their drawing `level` differs
  (M3_CONTRACT §1). Pairs of Links generated from an overpass pair or from Street segments on
  different levels stay apart, so Find crossings and D382 keep their meaning. Hand-drawn Links and
  Connectors on different levels now get a passive crossing, as the contract says.
- **Failure first:** the old `levels_apart_do_not_conflict…` became
  `drawing_level_is_not_separation…`; two new rows cover the overpass and level separators. All three
  failed before the fix. Shipped projects have every Link at level 0, so nothing they derive changes.
- **Verification:** release build, `ctest` 272/272, desktop/headless builds and `check`; `multra-check-docs`.
