# Anticipation across a join (M3.2.8a.1b)

Written 2026-10-19 (D253). **Options measured for the owner's choice; nothing is implemented, and
none is recommended.** This is the contract ROADMAP M3.2.8a.1 asks for before code, for the clamps
left after D251 ([MERGE_CLAMPS](MERGE_CLAMPS.md) §1, §5). Measurements are in the
[evidence](../evidence/join-anticipation.md). No clamp-free claim follows, and no gate is closed.

## 1. The problem

Where two segments feed one, a vehicle sees the other stream only once that vehicle's front is on
the shared segment (MERGE_CLAMPS §1). By then a major vehicle can be a few metres behind a minor
that has just pulled out, and it clamps:
- the **pull-out** class J1 has 9 clamps on M2.6 after D251;
- the **late catch-up** class J2 has 4.

The idea is that a vehicle approaching a join would follow the other stream's vehicle that will
reach the join first, before that vehicle enters.

## 2. The rule measured (A4, the only workable variant)

For a vehicle X whose front is within `R` = 50 m of a join on its route, at distance `dX`:
- **Who counts.** A vehicle Y counts when all of these hold:
  - its front is on another predecessor segment of the same join, at distance `dY`;
  - it is moving, at 0.1 m/s or more;
  - it has passed its own give-way line toward that join, or its approach has no rule toward it;
  - it is nearer the join (`dY < dX`; on a tie, the lower id is nearer).
- **The gap.** Y's rear is projected onto X's route: `gap = dX − (dY + L_Y)`. Y counts only when
  that gap is positive. Side by side is the rule's business.
- **The effect.** X runs its following law a second time, against the nearest such Y at Y's speed
  and acceleration, and keeps the stricter of the two accelerations. The hard cap, receiving
  space, zones and the rules themselves are unchanged.

Variants that replaced the following leader, or counted vehicles held upstream or alongside, made
thousands of clamps (A1, A2) or 177 clamps and 12 overlaps (A3). See the evidence.

## 3. What it does (measured, seeds 42–81, from D251's defaults)

| | D251 | A4 |
|---|---|---|
| M2.6 clamps | 17 (J1 9, J2 4, M 3, 1 other) | 16 (J1 5, J2 1, M 3, 7 other) |
| M2.6 mean delay, s | 46.010 | 46.014 (no movement more than 0.03 s) |
| T-junction mean delay, s (±95 %) | 7.069 (±0.484) | 7.545 (±0.558): majors +0.8–1.1 s, minors −1.5–1.7 s |
| Four-leg | — | byte-identical |
| Lane-change lab, pin, TS reference rows, core rows | — | unchanged; 949/949 rows pass |
| Body overlaps | 0 | 0 |

- **What it buys:** it removes 8 of the 13 J clamps but adds 7 elsewhere, for a net of 1.
- **What it costs:** it changes the T-junction, whose minor-road delay is what M3.2 closes on
  (M3_ACCEPTANCE §3, R7). Its majors would yield to minors that have pulled out, which no rule asks
  them to do.

## 4. Recommendation (the session's, for the owner)

**Do not adopt A4.** It hardly moves the clamp count, it adds a behaviour no Vissim-style rule
states, and it shifts the T-junction exercise's numbers just before the owner's acceptance sheet
is filled.

J1, J2 and A4's new clamps point at the **line approach**: a minor crawls onto its line still
moving, so it reads as `committed` and pulls out (D227, D232, D249's J). Making that approach stop
cleanly is its own contract, with D227's kinematic braking floor as its starting point. It would be
the better next candidate, if the remaining 17 clamps are to be pursued at all.

**Rows, if the owner chooses A4 anyway (BA168–BA172, planned).** The rule would become a fourth
`clampRules` switch, `joinAnticipation`, off or on by default as the owner decides:

| Row | Case | Asserts |
|---|---|---|
| BA168 | A major approaching a join behind a minor that has passed its line, today a clamp | It eases before the join; no clamp |
| BA169 | The minor is held at its line | It is not a leader; the major runs as today |
| BA170 | A faster virtual leader nearer than a slower real one | The stricter acceleration is kept; no acceleration into the real leader |
| BA171 | Side by side (projected gap ≤ 0) and a tie in `d` | Not a leader; the tie goes to the lower id |
| BA172 | Switch off; codec and dialog | Today's bytes; schema 45's `clampRules` gains the key |

## 5. What stays out

Anticipation for lane changes, another `R`, and calibration. Rule 4 applies.
