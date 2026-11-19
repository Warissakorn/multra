# Merge admission over the physical mouth (M3.2.4e)

Written 2026-11-19 (D593), the owner's go-ahead the same day ("เริ่ม M3.2.4e ได้เลย"). Rule 4
applies: nothing here is validated. It is slice 1 of [STOP_LINES](STOP_LINES.md) §8, without the
derived lines and `frontStopDistance`, which follow in their own slice.

## 1. The defect

An authored merge (Take over merge) stores the D50/D59 strip: on a Connector the last metre, its
line one metre short of the Connector's end; on a Link the metre before the join. The surfaces
overlap much earlier. In the T-junction fixture (OWNER_SITTING A2, measured):

| Side | Stored entry / line | Measured overlap |
|---|---|---|
| Crossing turn joining westbound (Connector) | 26.23 m / 26.23 m | 13.32–27.23 m |
| Westbound at that join (Link) | 169.00 m / 169.00 m | 155.65–170.10 m |
| Near turn joining eastbound (Connector) | 24.01 m / 24.01 m | 7.99–25.01 m |
| Eastbound at that join (Link) | 169.00 m / 169.00 m | 152.36–170.01 m |

A minor vehicle waiting at its line already stands inside the lane it joins, and a major vehicle
passes the first metres of the overlap without the zone seeing it.

## 2. Rule

One function, `mergeMouth` (`right_of_way.hpp`), for Take over merge, for the automatic merge
candidates the editor shows, and for a taken-over area following a geometry edit (D86). The
solver (M3.2.3c) is unchanged.

| # | Rule |
|---|---|
| R1 | Each area's side covers the measured overlap of its pair (`conflictAreaGeometry`): entry at the first overlapping station and exit at the last, with a Link side's exit clipped to the join. Past the join the two streams share one lane and car-following separates them. |
| R2 | Each side's waiting line is 1 m (D50's setback) short of the earliest entry over the areas of its group, and not before the start of a Connector. One line per side, as before. |
| R3 | When no overlap of positive area is measured for a pair, its sides keep the D50/D59 strip. No guessed polygon (M3_CONTRACT §1). |
| R4 | A side stays on the segment that joins: an overlap beginning upstream of a section cut is clipped to the cut. Merge topology and group membership are read there unchanged. Not exercised by a row: the T-junction has no such cut. |
| R5 | Derived merges (nobody took them over) stay on the M3.1 rule at the strip (D59): the four-leg and M2.6 numbers do not move. |
| R6 | Stored files are not migrated: an existing area keeps its extents until the author takes the merge over again or edits the geometry it lies on (D86 rederives it with this rule, and its line keeps its old distance to the entry, so an old strip's line lands on the new entry, not 1 m short). Shipped projects regenerate through their builders. |

## 3. Acceptance rows (failure first)

| Row | Check | Today |
|---|---|---|
| MM1 | T-junction, both drives: each merge's minor line is at least 1 m short of the measured overlap on its Connector | fails (`wait-14`, `wait-18`) |
| MM2 | Each major line is at least 1 m short of the overlap on its Link | fails (`wait-13`, `wait-17`) |
| MM3 | Each side's extents contain the measured overlap (Link exit clipped to the join) | fails |
| MM4 | Seed 42, base fixture: no tick has a minor and a major body over the same merge's measured overlap at once | fails: 266 ticks of 0.1 s before the change, 0 after |
| MM5 | The crossing turn's merge chains with its crossing (A15): it waits at the crossing line for both | holds; a guard |
| MM6 | Every movement still discharges; no vehicle is left at the end of the run's cool-down that the base left none of | guard |
| MM7 | Four-leg (derived merges only) is byte-identical (existing `fourleg` pins) | guard |

## 4. Consequences

- The T-junction fixture and `data/projects/t-junction-priority.traffic.json` regenerate. The
  M3.2.7 sweep archive stays tied to its original input (D80); it is not evidence for this rule.
- Junction fills (`transition_fill`) and the M6 U network call `takeOverMergesOf`; their numbers
  move with this rule.

## 5. Out of scope

Hidden derived lines, `frontStopDistance` and the Stop line tool (STOP_LINES slices 1b–3); the
line-approach law; derived merges on the solver; swept-body safety.
