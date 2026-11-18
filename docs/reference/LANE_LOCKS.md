# Lane locks at dead ends — draft contract (D578)

**Status: the owner chose A, then B off by default; not D (B50, 2026-11-18, "ทำตามแนะนำ").** LL1
ran first and found a span defect, fixed as D579 (§2a). A and B are not implemented yet. Rule 4
applies: nothing here is validated.

## 1. The defect

A vehicle whose route needs another lane is on a **stub** chain ([M3_8_CONTRACT](M3_8_CONTRACT.md)
§2). It changes lanes when the five rules hold, and otherwise stops at its **dead end**, held like a
red head, until a gap opens. The engine never removes a vehicle and never deletes demand (M3 §4,
[M3_CONTRACT](M3_CONTRACT.md)).

**A lane lock** is a dead-end wait that does not end. The target lane's queue stands, so no gap
both safety tests accept appears. Cooperation (§2 "Cooperation") helps only a vehicle that a
moving neighbour can stop for. Everything behind the waiting vehicle then stands on its lane until
the run ends. One vehicle turns a lane into a dead lane, and the results report the jam as
the junction's delay.

The defect is known: M3.2.8b measured a 404 s wait on M2.6, which led to the cooperation rule. D207
(swapping pair) and D450 (shared dead end) closed two particular locks. What is new is its scale on
a real multi-lane, oversaturated Thai junction.

## 2. Evidence

The study [ratchada-latphrao](../../studies/ratchada-latphrao/README.md) (D575–D577), seed 42, one
run, `multra-cli 42 --project … --lane-changes --wait-causes`:

| Project | Vehicles waiting at a dead end | Waiting, s (sum) | Longest, s |
|---|---|---|---|
| ratchada-latphrao, D577 lanes (N 5/3/2, S 4/3/2, NW 5/3/1, SE 3/3/1) | 82 | 36,267 | 3,411 |
| m2.6-study-template | 0 | 0 | 0 |
| signalised-node-analysis | 2 | 58 | 42 |
| four-leg, lane-change lab, weaving lab, lateral lab, T-junction, M6 S1 | 0 | 0 | 0 |

A throwaway probe (session of 2026-11-17, not kept) followed every vehicle's segment. It found
22 lanes standing for hundreds to thousands of seconds, each behind one vehicle waiting for a lane
change:

- **At the end of an upstream Link (≈ 220 of 220 m).** A right-turner or a through vehicle must
  enter a lane whose queue stands.
- **Near the stop line (38 of 40 m).** NW vehicle 848, a left-turner, made two of its three changes
  (561 s, 874 s) and stood from 874 s to the end. The left Connector leaves the kerb lane at 33 m.
  Row LL1 must first confirm where this vehicle's dead end lies.

As the lanes became more specific (D576, D577), completed trips fell: 4,422 → 4,014 → 3,385. Each
single-movement lane adds forced changes.

## 2a. LL1 result: a dead end past the point of no return (D579)

On the D576 file, NW vehicle 848's route `path-6` (lane 2 of 5) had spans to the full left route
(lane 1) up to **273 m**, where the left Connector leaves. Its dead end was nonetheless **280 m**,
the end of its lane, because a span outward to `path-7` (a stub too) ran on to 280. A vehicle past
273 m could only change to another stub that could no longer reach the left turn. It waited at 280 m
for good. D579 makes the dead end the last metre from which a change still leads to a full chain,
and cuts spans there ([M3_8_CONTRACT](M3_8_CONTRACT.md) §2). Test `countstudy.a_left_turn_stub_is_held_before_its_turn_leaves`
fails without it.

| Seed 42 | Before D579 (D577 file) | After D579 |
|---|---|---|
| Completed / active / never entered | 3,385 / 601 / 6,754 | 4,698 / 539 / 5,503 |
| Vehicles waiting at a dead end / sum, s / longest, s | 82 / 36,267 / 3,411 | 187 / 19,600 / 2,683 |
| Safety clamps | 1 | 6 |
| Shipped projects and `multra-cli 42`, seed 42 | — | byte-identical |

More vehicles wait, but for shorter times. Those that remain stand at the end of an upstream Link
before a standing queue, which is option A's case.

## 3. What every option keeps

- **Replay.** The same scenario, seed, engine and toolchain give the same trajectory (rule 2).
  Nothing reads a clock or a random draw that is not seeded.
- **Layers.** `core/` gets any new value as scenario data, as spans and dead ends are today.
- **Safety.** A safety test is never waived silently. A relaxed test is a named rule with its own
  parameter, and clamps are counted as today.
- **Accounting.** Completed, active and pending stay in every result. A new outcome (diverted,
  removed) is a new count beside them, never hidden in one of them.
- **Shipped files.** They stay byte-identical unless the decision names them and regenerates them.

## 4. Options

### A. Entry lane by destination (compile; no engine change)

A routing decision may say that its destinations enter on the lanes that serve them. Each
destination's share then goes to its **full** chains on the entry Link only. Stubs get a share only
where no full chain exists, as today.

- **Effect:** where every movement has its own lane from the entry Link, no vehicle needs a forced
  change, so nothing can lock.
- **Opt-in:** a key on the decision, for example `"entryLanes": "serving"`. Absent means today, so
  every shipped run stays byte-identical. Schema step and editor checkbox.
- **Limits:** it does nothing for a change that a later object forces (a downstream decision, a
  weave, a lane drop). Entry lanes become deterministic by destination, so a driver's late
  choice is not modelled. On a short entry Link it moves the queue to the network edge.
- **Cost:** compile and schema only, plus the input table's split preview. Small.

### B. Missed exit (engine)

A vehicle that has waited at its dead end for `missedExitAfter` seconds gives up its destination.
It takes the full chain of its current lane (the lane's own continuation, usually straight on) and
keeps driving.

- **Effect:** no wait lasts longer than the threshold, so no lock outlives it. Demand is
  conserved: the vehicle arrives somewhere else.
- **Reporting:** a new appended event, `MissedExitEvent` (time, vehicle, from route, to route).
  Results gain a `diverted` count per movement, and the movement table says the counted turn was
  not served.
- **Parameter:** a behaviour key in seconds, absent = never (today). A shipped value needs its own
  decision. Vissim's analogue removes the vehicle after 60 s; this keeps it.
- **Limits:** turning counts are no longer delivered exactly when it fires. The engineer sees
  that through `diverted`. If the current lane has no full chain onward, the vehicle still waits
  (row LL6).
- **Cost:** one rule in the lane-change step, one event, one result column. Medium.

### C. Zipper at a standing queue (engine)

This extends cooperation to a target lane whose queue stands. When that queue moves off, the
first vehicle behind the target place holds its standstill gap for one vehicle. The waiting
vehicle then enters by the five rules.

- **Effect:** the vehicle keeps its destination, and the zipper merge Thai drivers use is
  approximated.
- **Limits:** it cannot help a vehicle already past where its target chain leaves. It slows the
  target lane by one vehicle per change. It overlaps M3.4 (cooperative lane change) and M3.8
  (motorcycles).
- **Cost:** a second cooperation rule plus its interaction rows. Larger, and it belongs with M3.4.

### D. Diffusion (remove the vehicle) — against M3 §4

As Vissim does: after a wait, the vehicle leaves the network and is counted as removed. It is
listed only because the owner may weigh it. It would need M3 §4 to be overridden by decision. It
hides the cause behind a count, and the delay it removes is real delay on site.

## 5. Recommendation

1. **A first.** It is opt-in, changes no shipped number, and removes this study's forced changes.
   The study's owner item (B4/A3) can then proceed.
2. **B next, off by default,** as the guarantee that no run is ever held by one vehicle. It applies
   to any project with a forced change, and `diverted` makes it visible.
3. **C with M3.4.** It is the realistic behaviour, but it needs the cooperative-lane-change
   contract around it.
4. **Not D,** unless the owner overrides M3 §4.

## 6. Draft rows (failure-first; numbered when the owner picks)

| Row | Check | Fails today? |
|---|---|---|
| LL1 | Locate the dead end of NW vehicle 848 (seed 42, D576 file): station, chain and target. If it lies past where its target chain leaves, the stub's span is wrong and is fixed first, as its own defect | **done (D579):** 280 → 273 m; test `countstudy.a_left_turn_stub_is_held_before_its_turn_leaves` |
| LL2 | ratchada-latphrao, seed 42: no dead-end wait longer than the chosen threshold (A: none at all on the entry Link; B: `missedExitAfter`) | yes: 3,411 s |
| LL3 | Every shipped project, seeds 42–51: byte-identical results with the option absent | must pass |
| LL4 | Accounting: completed + active + pending (+ diverted for B) equals vehicles generated, per input | must pass |
| LL5 | (A) With `entryLanes: "serving"`, each destination's per-lane input equals its share on full chains, with zero on stubs, wherever a full chain exists | new |
| LL6 | (B) A vehicle whose lane has no onward full chain keeps waiting and is reported, never removed | new |
| LL7 | (B) `MissedExitEvent` replays byte for byte. The movement table's `diverted` matches the events | new |
| LL8 | (C) After a zipper hold, no clamp is added on the target lane (M2.6 and the lane lab, seeds 42–81) | new |

## 7. Questions for the owner

1. Which options, in which order: A, B, C or D (recommended: A then B)?
2. For A: is a key on the routing decision right, or should the entry Link's lanes say which
   movement they serve?
3. For B: which threshold (Vissim's 60 s, or longer, such as 120 s), and is it off by default?
4. For B: should a diverted vehicle still count in its counted movement's delay, or only under
   `diverted`?
5. Is this a new sub-milestone (proposed: ROADMAP M3.2.11, a row in the M3.2 table), and does it come before the study's
   rehearsal (A3)?
