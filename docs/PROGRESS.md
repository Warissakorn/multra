# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-19 — Nodes S0.3: Micro node evaluation (D264)

Each Node now reports per-turn, per-leg and per-Node delay ([NODES](reference/NODES.md) §4, §13).
- **How:** one travel-time section per turn (in-leg Link start → out-leg crossing) and one queue
  counter per in-leg at its crossing, measured by the existing accumulators after the authored ones;
  `nodeRows` weights leg and Node delay by vehicles.
- **Output:** JSON `nodes[]`, a CSV block after LOS with its marker line, batch estimates
  `{n, mean, sd, halfWidth95}`, letters with the Node's control type. Nothing without a Node: the
  shipped projects' single and batch outputs are byte-identical.
- **Finding:** vehicles enter from rest, so an in-leg starting at a source includes the start-up
  loss (about 3 s of the T-junction's 3.2 s major-road delay). Choice 1a kept; N14 uses a split
  in-leg.
- **Cost:** no change without a Node; with one, Ir +40 % (four-leg) and +26 % (T-junction).
- **Rows:** N12–N17 (ctest `nodeevaluation`); five mutations caught. Pin and TS rows unchanged.
  Linux only.

## 2026-10-19 — Nodes S0.2: membership (D262, D263)

What a Node holds is now derived from the network inside its polygon ([NODES](reference/NODES.md) §3, §12).
- **Contract fixed first (D262):** §3 refused any connected Link crossing the boundary twice, which
  refused the shipped T-junction, whose major roads run through as single Links. Legs are now
  boundary crossings: an entry is an in-leg, an exit an out-leg, and a Link's own run through is a
  turn. `NODE_LINK_REENTERS` replaces `NODE_LINK_CROSSES_TWICE`.
- **Model:** `nodeMembership` (`src/model/network/node_membership.*`) gives inside objects, legs,
  turns (checked with `routeLaneChains`), the turn-conflict table and control. Refusals are advisory
  diagnostics at `nodes[i]`, so Run is never blocked.
- **Editor:** the Nodes tab shows control, legs, turns and issues.
- **Rows:** N6–N11 (ctest `nodemembership`); the four-leg gives 4 + 4 legs, 12 turns, signalised;
  the T-junction 3 + 2 legs, 4 turns, unsignalised and 3 conflict rows.
- **Unchanged:** the engine reads no Node; the pin and the TS rows are unchanged. Linux only.

## 2026-10-19 — Nodes S0.1: object, file, tool and tab (D260, D261)

The owner accepted every Node recommendation (D260) ([NODES](reference/NODES.md) §11).
- **Model:** `Network::nodes` and `nodeIssues` (ids, simple polygon, no overlap). The polygon tests
  are shared with zones in `polygon.*`.
- **File:** `network.nodes` is written only when a Node exists, at schema 48, with
  `EDIT_NODE_INVALID` at its path. `generated` is written only when true.
- **Commands:** `putNode`, `renameNode`, `deleteNode`. An edit clears `generated`.
- **Editor:** the Node tool (`N`) shares the zone draft, and the Nodes tab in the Measurement group
  can rename and delete. Each action is one Undo step.
- **Rows:** N1–N5 (ctest `nodes`, `node-ui`). The workspace and object-group UI tests list the new
  tool and page.
- **Unchanged:** the engine does not read Nodes, so every run output, the pin and the TS rows are
  unchanged. Linux only.

## 2026-10-19 — The Node contract (S0, D259, docs only)

The owner chose to start the multi-level work (D145, D175) at S0, before W99 ([NODES](reference/NODES.md)).
- **The object:** a simple polygon at `network.nodes`, at schema 48, merged with M5.1's evaluation
  nodes.
- **Derived from the Micro detail inside, never stored:** legs, turns (the `routeLaneChains`
  walk), conflicts and control.
- **Micro evaluation:** one automatic travel-time section per turn, with leg and Node rows weighted
  by vehicles, a queue per leg, and LOS letters labelled as LOS §4.
- **Coarse values** and refine/derive-back are specified but booked with S2.
- **Rows:** N1–N18 planned.
- **Five owner choices,** each with a recommendation. Also fixed: MULTI_LEVEL_MODELLING's dangling
  "VISSIM_PARITY §6" (now §2b).

No code. Linux only.

## 2026-10-19 — `standingStopFloor` implemented, on by default (D257, D258)

The owner took D256's recommendation, on by default ([LINE_APPROACH](reference/LINE_APPROACH.md) §7).
- **Engine:** in the prototype law's `braking` mode, behind a standing leader, braking is at least
  `v²/(2·gap)`, capped at the type's maximum.
  - Only behaviours with `amberDeceleration` get it, through the runtime-only
    `DriverBehaviour::stopFloor`, set when the simulation is created.
  - Bare M0 scenarios keep their bytes.
- **File and editor:** `clampRules.standingStopFloor` is written only when off, at schema 47. There
  is a fifth Run settings checkbox.
- **Rows:** BA173–BA178 and the Run settings UI test. BA164's forcing also switches the floor off.
- **Measured:**
  - On reproduces D256's F1a byte for byte: M2.6 17 → 5 clamps, T-junction 7.069 → 7.099 s.
  - Off is byte-identical to D255.
  - The pin is unchanged, and the TS rows pass 5/5.
  - Ir +0.13–0.18 %.

## 2026-10-19 — Stopping cleanly at a line measured (D256, docs only)

The owner asked for the line approach as a contract with numbers first (D254)
([LINE_APPROACH](reference/LINE_APPROACH.md), [evidence](evidence/line-approach.md)).
- **Eleven reverted prototypes** of the prototype law, on D251's defaults.
- **D227's floor**, scoped to behaviours with `amberDeceleration` (F1a):
  - cuts M2.6 17 → 5 (J1 9→1, M 3→0, none new);
  - keeps the four-leg and lane lab byte-identical, the pin, the TS rows 5/5, and every row but
    BA164's forcing;
  - moves delays by ≤ 0.03 s. Unscoped, it fails one TS row.
- **A floor behind a braking leader** (F2) removes only the lane lab's clamp, but binds about 1,900
  times per M2.6 run. **J** (held means not committed) gives 17 → 27.
- **Recommended:** F1a as a fifth `clampRules` switch, with rows BA173–BA178 planned. On or off by
  default is the owner's call.

No engine code is committed. Linux only.

## 2026-10-19 — `joinAnticipation` as an off-by-default switch (D254, D255)

The owner chose D253's A4 anyway, as a fourth project switch that is off by default, and asked for
the line approach as a contract with numbers first (D254, [JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md) §5).
- **Engine:** `src/core/join_anticipation.*`. With the switch on, a vehicle within 50 m of a join runs
  its following law a second time against the other stream's first moving vehicle past its line,
  and keeps the stricter acceleration. Off, the step reads one empty vector.
- **File and editor:** `clampRules.joinAnticipation`, absent = off, written only when true at
  schema 46; a fourth Run settings checkbox; one Undo step.
- **Rows:** BA168–BA172 and the Run settings UI test. The "nearer the join" test proved redundant
  under mutation (a positive projected gap implies it) and was removed; every other filter is caught.
- **Measured:** on, seeds 42–81 of the four shipped projects reproduce D253's A4 outputs byte for
  byte; off, all four are byte-identical to D251, with the pin unchanged. Ir off: +0.16 % / +0.11 %
  / +0.00 % (four-leg, lane lab, M2.6).

## 2026-10-19 — Lane lab clamp traced (D252); join anticipation measured (D253, docs only)

The owner asked for both of NEXT's M3.2.8a.1 candidates, with the contract and numbers before any
code for the second.
- **D252** ([evidence](evidence/merge-clamps.md#d252-the-lane-change-labs-clamp)): the lane lab's
  only clamp (seed 74) is not a merge. A follower brakes at only 4.4–4.8 m/s² behind a leader
  stopping hard at its stub lane's dead end. That is D241's follower class, from the prototype
  braking's softness (D227).
- **D253** ([JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md), [evidence](evidence/join-anticipation.md)):
  eight reverted prototypes.
  - **Replacing the following leader** with the other stream's vehicle produced phantom leaders, side-by-side
    stops, and acceleration into the real leader: thousands of clamps, or 177 clamps and 12
    overlaps.
  - **A4** (a second following law, keeping the stricter acceleration) is clean, but it nets M2.6 only
    17 → 16 and moves T-junction delay from the minor road to the major road (7.069 → 7.545 s).
  - **Not recommended.** The line approach (D227's floor) is named as the better candidate.

No engine code is committed. `check` 179/179, Linux only.

## 2026-10-19 — Merge clamp switches implemented (D250, D251)

The owner chose D249's recommended package, as project switches that are on by default
([MERGE_CLAMPS](reference/MERGE_CLAMPS.md) §3, [evidence](evidence/merge-clamps.md#d251-the-package-implemented)).
- **Engine:** `ClampRules{leaderTravel, sideEntryAtLine, redLineStop}` on the definition.
  - `tickCap` relieves the hard cap only on a tick that would clamp.
  - `caps.cpp` holds B and M; the step's line-stop test takes Q.
  - `simulation.cpp` stays at 500 lines through a byte-identical refactor into `holdCap` and `leaderCap`.
- **File and editor:** `definition.clampRules` at schema 45, written only when a switch is off;
  three Run settings checkboxes; one Undo step.
- **Rows:** BA158–BA167, plus the Run settings UI test. D105's row keeps its clamp with
  `sideEntryAtLine` off. All 14 mutations are caught.
- **Measured:**
  - M2.6 41 → 17 clamps; delay 46.013 → 46.010 s; trips unchanged; no overlaps;
  - all off is byte-identical on all four projects; the pin and the TS rows are unchanged;
  - Ir +0.50 / +0.73 / +0.68 %.
- **A prototype bug:** D249's red-head stop braked with the wrong distance. Its measurement stands
  as recorded, and the implementation's numbers replace it.

`check` 179/179, Linux only.

## 2026-10-18 — Red running closed (D248); M3.2.8a.1's clamps traced and options measured (D249)

- **D248 (docs only):** the owner kept `redRunningTime` as an upper bound and a runner's committed
  crossing counted. RED_RUNNING now says that with `amberDeceleration` a run almost always starts at
  red onset.
- **D249 (docs only, [MERGE_CLAMPS](reference/MERGE_CLAMPS.md), [evidence](evidence/merge-clamps.md)):**
  - **Trace:** temporary probes traced all 41 shipped clamps (M2.6, seeds 42–81) to four causes:
    - 20: a minor creeping at its line, 1 m from the join, beside a major entering the shared lane;
    - 9: a minor that pulled out as committed;
    - 8: a major catching a minor still on its connector;
    - 4: a creep at a red head.
  - **Options:** seven, each a reverted prototype. B+M+Q removes 24 of the 41, with delay, trips,
    overlaps (0), the CLI pin and the TS reference rows unchanged. An unscoped red-head stop fails
    2 TS rows. A 3 m setback and "held is not committed" make things worse.
  - **No engine code is committed;** the owner chooses.

`check` 178/178 on the committed tree, Linux only.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
