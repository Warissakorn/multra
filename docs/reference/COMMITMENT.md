# Commitment deceleration at give-way lines (B-2)

Written 2026-10-17 (D218), and implemented the same day (D219,
[evidence](../evidence/commitment-deceleration.md)). This is tier B-2 of
[BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), a level-4 key under D216: driver behaviour, local by
road. Rule 4 applies: nothing here is validated, and no value ships (D178).

## 1. Scope and the owner's choices

M3_8_CONTRACT §1 commits a driver who cannot stop at a give-way line: `v² > 2 · d · gap`. There, `d`
is always the vehicle type's `maxDeceleration`. That one value sets how assertive minor-road drivers
are, and so sets minor-approach capacity at every priority junction. Assertiveness differs by
country, so a study needs to change it.

The owner chose on 2026-10-17:
- **Give-way lines only.** The key applies at conflict-zone waiting lines and derived priority-rule
  stop lines. The signal head's "cannot stop at all" test (AMBER §1, red) keeps the type's
  `maxDeceleration`: a lower value there would be red-light running, which is B-9. Amber already has
  `amberDeceleration`.
- **The lower value wins.** The deceleration used is `min(commitmentDeceleration, maxDeceleration)`,
  so one behaviour can serve several vehicle types and no vehicle is asked for more than it can do.

## 2. The key

- **`commitmentDeceleration`** is an optional behaviour key, in m/s², on either model. Its range is
  (0, 15]; a value outside it, or a non-number, is refused with `INVALID_BEHAVIOUR_PARAMETER` at its
  path.
- **Absent means today's rule.** The deceleration is the type's maximum, and every run is
  byte-identical.
- **Schema 36.** The key is written only when an owned behaviour carries it. An older schema carrying
  it is refused with `EDIT_UNSUPPORTED_FIELD`.

## 3. The rule

A vehicle is committed to a give-way line when

    v² > 2 · min(commitmentDeceleration, type.maxDeceleration) · gap

with `v` and `gap` from the tick's snapshot, as before. Everything else in M3_8 §1 is unchanged:
- Equality stops.
- A standing vehicle is never committed.
- Commitment is never stored.
- A committed driver ignores only the anticipation part of the gap test, never occupancy, an
  unserved Stop, receiving space or the swept check.

**What a value means:**
- A value at or above the type's maximum is today's behaviour.
- A lower value commits a driver earlier, further from the line. That driver then accepts a shorter
  gap.

## 4. The known risk

D69 measured `comfortableDeceleration` (2 m/s² for a car) as the commitment value. Drivers then
committed up to about 2 s ahead of a major vehicle, and the major road clamped at the T-junction and
at an M2.6 merge ([ENGINEERING_NOTES](ENGINEERING_NOTES.md), "Do not retry"). The key makes that
setting possible again, on purpose, for a study that has the evidence. Therefore:
- no shipped behaviour carries it (D178);
- the evidence measures clamps on both roads at several values;
- a study choosing a value near the comfortable deceleration should expect major-road clamps, and
  report them.

## 5. Consumers

| Site | Change |
|---|---|
| `src/core/conflicts.hpp` `committed` | An overload that takes the behaviour; the head test keeps the type-only one |
| `src/core/conflicts.cpp` `zoneHold` | Uses the overload |
| `src/core/simulation.cpp` priority rules | Uses the overload |
| `src/core/simulation.cpp` signal heads | Unchanged (owner) |
| `src/core/validate.cpp` | Range, in both model branches |
| Codec (`parse.cpp`, `definition.cpp`, `behaviour_library.cpp`, `document.cpp`) | The key; schema 36 |
| Behaviour editor | One row in D217's either-model section |
| Behaviour Lab | **No row:** the circuits have signals but no give-way line (`lab_urban.cpp`: "no merge, no rule"), so a row would look as if it worked when it does not (§5 of BEHAVIOUR_PARAMETERS) |

## 6. Rows (BA72–BA78)

| Row | Case | Asserts |
|---|---|---|
| BA72 | Key absent | The `commitment` group is unchanged; the CLI pin and the shipped projects are byte-identical |
| BA73 | Priority rule: a minor vehicle 8 m short at 10 m/s, a major vehicle 2.5 s off | Forcing first: not committed without the key (it can stop at 8 m/s²), committed with key 4. Without the key it is held at the line; with it, it goes first. No clamp either way |
| BA74 | Conflict zone: the same scene | The same |
| BA75 | Key 9 on a heavy vehicle (max 6) | The trajectory is identical to the key absent (min rule) |
| BA76 | Red head, with `amberDeceleration` and the key 2 | Trajectory identical to the key absent (the head test ignores the key) |
| BA77 | BA73 under W74 | The same hold |
| BA78 | Codec | Schema 36 round-trips; 35 carrying the key → `EDIT_UNSUPPORTED_FIELD`; a string, 0 or 16 → `INVALID_BEHAVIOUR_PARAMETER`; 15 accepted; 37 → `EDIT_VERSION` |

Plus a measurement: `t-junction-priority` and `m2.6-study-template`, seeds 42–51, with the key at 6,
4 and 2 m/s². Record minor delay and clamps against absent.

## 7. Performance

One `value_or` and one `min` per give-way test. With the key absent, the cost is unchanged within
noise.

## 8. Failure conditions (reopen D218)

- A study needs the red-light test to use the key: that is B-9, red-light running.
- Clamps appear on the major road at values a cited study uses: the anticipation part of the gap
  test then needs its own model.
