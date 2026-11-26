# Clamps on `signalised-node-analysis`: a front exactly at a connector's start — diagnosis — 2026-11-26

Evidence for D672. The defect was found by D669's set-back runs: on `signalised-node-analysis`, today's engine
(`lineRest` on) clamps the prototype 4.7 times per run, with +7 s of delay, and a set-back of 0.01 m removes it
([line-setback-options](line-setback-options.md)). Nothing here is committed to the engine. The probes were built
against the branch at `30ef5af`, Release, seed 42 for the traces and seeds 42–51 for the counts.

## The mechanism (traced, seed 42, the first clamp at t = 403.8 s)

The project has no conflict zones. Its turning connectors merge with straight connectors through derived merge
rules. The one involved is `give-way/connector-44/to/connector-42`:
- the turner waits at 21 m on its 22 m connector `connector-44`;
- the conflict point is the end of the straight connector `connector-42`, which is **2.73 m long**;
- gap time 3 s, headway 7 m.

1. **A false hold.** Straight car 161 stops at its red head with its front at exactly 289.0000 m. That is the end
   of `lane-29` and position 0 of `connector-42`.
   - Its span is therefore on `connector-42`, 2.73 m from the conflict point.
   - That is inside the 7 m headway, so the rule holds turner 114 at its line from about t = 362 s to 402.9 s,
     although car 161 cannot move on its red.
2. **A release blind to the next car.** At green, car 161 pulls away, and once its rear passes the point the turner
   is released (t = 402.9 s).
   - The next straight car, 166, is then at 286.18 m on `lane-29`, at 4.12 m/s: 5.55 m and 1.35 s from the point.
     By the rule's own tests (headway 7 m, gap 3 s) it should block.
   - It does not, because the rule scans only spans on the conflict segment (`simulation.cpp`, "Priority rules";
     PRIORITY_RULES §1: "a major vehicle on the marker's segment").
3. **The clamp.** The turner enters `lane-23` 0 m ahead of car 166, and car 166's leader cap binds at 0
   (`leaderCap 0`, `holdCap inf`) at t = 403.7 s.

With the first car resting 0.01 m short (on `lane-29`), step 1 does not happen. Turners go during the straight
stream's red, and nothing is released into its green.

## Two defects

- **E1, the boundary.** A front exactly at a segment's start counts as on that segment in the rule's scan. D653
  settled the same boundary for the observers: a front exactly at a line has not passed it.
- **E2, the scan range.** The rule sees only the conflict segment. On a short straight connector (2.73 m here), an
  approaching major car is invisible until it is almost at the point. This is how PRIORITY_RULES §1 is written, not
  a slip; Vissim's conflict markers look upstream across links.

## E1's fix, measured with a reverted patch ([patch](exact-line-rule-options.patch), `ZZ_F2`)

The rule's scan skips a span whose front is at most 1e-9 m into the conflict segment. Per run, seeds 42–51,
`lineRest` on:

| Project, arm | Clamps today → fixed | Delay today → fixed (s) |
|---|---|---|
| node analysis, prototype | 4.7 → 0 | 41.85 → 34.54 |
| node analysis, W74 fixture | 19.0 → 15.7 | 97.04 → 85.79 |
| node analysis, W99 fixture | 18.6 → 12.1 | 47.87 → 37.19 |

- **Unchanged to the digit, every arm:** four-leg, M2.6, the T-junction, the priority-rule lab, the
  waiting-acceptance lab and the line-stop lab.
- `multra-cli 42` is byte-identical after the revert.
- E2 is not measured. Changing it alters what every derived merge rule sees, so it needs a contract first.
