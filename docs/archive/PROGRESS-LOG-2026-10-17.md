# PROGRESS log archive — 2026-10-17

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-17 — B-3: accepted deceleration for mandatory lane changes (D220, D221)

NEXT's item, contract first ([MERGE_DECELERATION](../reference/MERGE_DECELERATION.md),
[evidence](../evidence/merge-deceleration.md)).
- **The owner's choices:** two keys, as Vissim has (`mandatoryDecelerationOwn`,
  `mandatoryDecelerationTrailingVehicle`), and the trailing one is read from the changer's behaviour.
- **What changed:**
  - mandatory changes only; the room condition still prevents clamps;
  - schema 37 and two editor rows.
- **Tests:** four rows failed first, and two mutations are caught. The third mutation showed the
  contract's `min(key, maxDeceleration)` could never matter, because following already clamps there,
  so it was removed.
- **Measured** in the shipped projects: the keys barely move anything (delays by ≤ 0.16 s, no
  clamps, the lab's one 2.5 s dead-end wait gone), because their demand rarely meets a refused
  change. Keys absent: byte-identical, +0.002 % Ir.

`check` 168/168, Linux only.

## 2026-10-17 — B-2: commitment deceleration at give-way lines (D218, D219)

NEXT's item, contract first ([COMMITMENT](../reference/COMMITMENT.md),
[evidence](../evidence/commitment-deceleration.md)).
- **The owner's choices:**
  - the key applies at give-way lines only, and the red head keeps the type's maximum (red-light
    running is B-9);
  - the lower of the key and the type's `maxDeceleration` wins.
- **What changed:**
  - `commitmentDeceleration` (schema 36, either model) in the give-way commitment test;
  - an editor row; no Lab row, because the circuits have no give-way line.
- **Tests:** five rows failed first, and three mutations are caught. The first scene clamped even
  without the key, so a sweep chose an 8 m scene.
- **Measured** with the key on every behaviour:
  - T-junction delay falls from 6.45 to 5.65 s at 2 m/s², while clamps rise from 0.2 to 0.8 per seed;
  - M2.6 clamps rise from 1.1 to 8.3, as D69 warned.

  No value ships. Key absent: byte-identical, +0.04 % Ir.

`check` 167/167, Linux only.

