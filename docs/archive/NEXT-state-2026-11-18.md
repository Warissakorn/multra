# NEXT state — 2026-11-18

Text cut from [NEXT](../NEXT.md) when it was compacted to open items (2026-11-18), moved verbatim.
Each block names the NEXT section it came from. Nothing here is current.

## From: Counted study Ratchadaphisek × Lat Phrao (D575–D578) — a failing case; wait for B50

- **Lane locks: done (D578–D582).** [LANE_LOCKS](../reference/LANE_LOCKS.md), ROADMAP M3.2.11 closed:
  LL1 (D579), A (`entryLanes`, D580; the study uses it), B (`missedExitAfter`, off by default, D581),
  the owner's defaults accepted (D582). C (zipper) goes with M3.4. The two new editor fields were
  compiled but never opened.
- **The study's last clamp: fixed (D583).** D258's floor now stops at the standstill gap behind a
  standing vehicle, and a cap hit there within the maximum is braking (LINE_APPROACH §8). Study,
  seed 42: no clamp. Shipped projects unchanged.

## From: Review of 2026-11-10 — queued proposals (D493)

  - **Split `EditorWindow` (D505): steps 1–5 done (D514 `BatchSession`, D517 `RunSession`, D518
    `RecoverySession`, D519 `AutomationHost`, D520 `ObjectTab`), and all 13 object tabs are panels
    (D521–D523), and the Macro/Meso runs' functions are on their panels (D524; header 396 lines).**
    D505's listed steps are done. The metadata-driven object table (D525,

## From: Review of 2026-11-10 — queued proposals (D493)

    owner's verdict (OWNER_SITTING B48). `src/project/`
    (codecs, runs, outputs; `src/report/` is reserved) waits until report work starts.

## From: Review of 2026-11-10 — queued proposals (D493)

  - **Random streams per input: done** (slices 1–4, D506, D510, D512; choice 3, the paired
    comparison under `perInput`, D526, BATCH §7 rows CMP11–CMP14).
    Slice 4's cost was not measured with callgrind (D512). A new project is `perInput` at schema 74; shipped projects stay
    `shared` until a recorded decision regenerates them.

## From: Review of 2026-11-10 — queued proposals (D493)

  - **Analytic rows against HCM: §6 written (D515), tolerance set (D516: S1 ±5 %, S2/U ±15 %).**
    HCM 7 values read (D527: `s0` 1,900; `g` = displayed green; d1/d2 and Eq. 20-18 forms; U-a's
    `t_c` 6.2 s, `t_f` 3.3 s; references computed in [§6](../evidence/m6-benchmark-options.md)).
    The priority-line counter is built (D528, [PRIORITY_LINES](../reference/PRIORITY_LINES.md),
    `--priority-lines`). D529: S1-f added (Akçelik & Besley 2002); the S2-c script is done
    (`multra-fctl-reference`, [FCTL_REFERENCE](../reference/FCTL_REFERENCE.md) FC1–FC6) and its
    references are in §6 (10.98, 13.49, 19.57 s at `x` = 0.5, 0.7, 0.85). S1-d's references are read (Bonneson 1992, D532; Lee & Chen 1986, D533).
    U-c's effective `t_c`: Siegloch's regression in `--priority-lines` (D534, PRIORITY_LINES §5,
    PL7–PL9). Every source the first comparable run needs is read and computed.
    **D535: the first comparable run, kind S1, made** ([evidence](../evidence/m6-s1-first-run.md)):
    S1-a 1,944 veh/h passes (1,900 ±5 %); S1-f misses (−6.7 % against 2,083); S1-d passes ranks 3, 7, 8
    and misses 2, 4, 5, 6, 9 against Lee & Chen. **Ask** the owner whether S1's gate is S1-a alone
    or needs S1-f and S1-d too. D536: the D530 diagnostic contract and observer; its rerun measured
    nothing (the engine's queue creeps at ~0.1 m/s). D537 (owner): read at Go, at 5 km/h. **Done**
    ([evidence](../evidence/m6-s1-diagnostic.md)): `L_hj` 7.95 m against 6.9 (+15 %, about two thirds of
    S1-f's gap), `t_x` 1.195 s against 1.17, `v_S` 43.5 against 45.1 km/h; the queue creeps through the red.
    **Ask** the owner whether any of these is to change (it would move S1-a too).
    **D538: S2-c passes** ([evidence](../evidence/m6-s2c-first-run.md)): 11.83, 14.13 and 19.79 s against
    10.98, 13.49 and 19.57 s (+7.7, +4.7, +1.1 %). **D539: S2-b and S2-a run** ([evidence](../evidence/m6-s2ab-first-run.md)): pass only at
    `x` = 0.5; −16 % to −53 % below HCM `d1 + d2` from 0.7 to 1.05, the gap in `d2`. **D540** (owner asked for a
    recommendation): arrival process measured ([evidence](../evidence/m6-s2-arrivals.md)): at the line arrivals are
    bunched (IDC ≈ 1.2, CV ≈ 1.5), not smoothed, so the S2-b/S2-a gap is on the queue/discharge side.
    **D541** ([evidence](../evidence/m6-s2-green.md)): the engine's capacity is 913 veh/h (HCM 844): effective
    green 42.3 s, lost time 0.73 s (HCM 3 s). At its own values HCM `d1 + d2` is within ±15 % at every
    level, so S2-b/S2-a miss on capacity, not queueing. The held calibration question (D540) now covers
    jam spacing, start-up response and amber use. **D542: U-c and U-a run** ([evidence](../evidence/m6-u-first-run.md)):
    the Stop-controlled rank-2 turn carries 228.5 … 81.6 veh/h at `v_c` 200 … 1,000 (HCM 846 … 298); both miss
    at every level; the engine's follow-up headway is 13.7 s (HCM 3.3 s), so the Stop service, not the gap
    test, limits the movement. Every kind now has its first run. **D543** ([evidence](../evidence/m6-u-split.md)):
    of the 13.7 s, the move-up to the line takes 10.7–12.2 s (the last 0.5 m alone 3.75 s, the creep), the
    service 0.1 s, the release 1.6 s, almost never blocked. **D544** (owner): no calibration on M6; the
    misses stand as known deviations until independent data (U-e inD, R7; Thai data, Q4 (d)); D540's hold is closed.
    **D545 sweep phase 1** ([evidence](../evidence/m6-sweep-phase1.md)): B and C move S1 as estimated, raise S2
    capacity (misses grow) and cut U's `t_f` only 0.7–0.8 s; nothing adopted. **D546 sweep phase 2** ([evidence](../evidence/m6-sweep-phase2.md)):
    runtime flags off by default; D (standstill gap behind a standing target) halves U's `t_f` (6.91 s; DE 5.82)
    and gives S1 6.5 m spacing but 1,910 veh/h (`t_x` 1.35 s); E alone changes nothing; nothing adopted. The
    sweep is complete. **Next on M6:** nothing until independent data (R7 inD, Q4 (d)); the gate stays open.
    D530: the S1 diagnostic (`t_x`, `L_hj`, `v_S`; Murat & Cetin 2019) needs a measurement contract
    before it is used.
    D531: S1 keeps the mean headway; the median and skewness beside it need a `--discharge`
    contract.

## From: Review of 2026-11-10 — queued proposals (D493)

  - D497–D504 are done; the owner's look is OWNER_SITTING B45.

## From: Optimisation pass of 2026-11-07 (D487–D489) — follow-ups only

  insertion's snapshot (oversaturated study −6.6 % Ir); what is left there is listed in the notes.

## From: AI automation follow-up (D344–D350, D558–D573)

- **Done:** owned `definition.compositions` with type-share and interval rows (D558, CS1–CS4), owned
  `definition.modes` (D559, MS1–MS4) and owned `definition.vehicleTypes` (D560, VT1–VT4).
- **Item 49 answered (D562):** the speed bounds use `value()` sentences; `desiredSpeed` and curve
  points close with the type row from 21.
- **Behaviours (the owner agreed, "เห็นด้วย", 2026-11-14):** the inventory is done (D561,
  [PROJECT_SCHEMA_DEMAND](../reference/PROJECT_SCHEMA_DEMAND.md#behaviour-extension-d561-inventory-d563-b1-d564-b2)).
  **B1 and B2 done (D563, D564, BH1–BH6); B3 needed nothing.** Owned behaviours are fully mapped.
- **Done (D565, BC1–BC3):** `vehicleClasses` and `linkBehaviourTypes`.
- **Done (D566, ZL1–ZL3):** the root zone layer (`zones`, `zoneConnectors`, `odMatrices`,
  `parkingLots`); either-or keys use the field attribute `unless`.
- **Done (D567, SP1–SP3):** root `studyPeriods`, `definition.clockStart` and `studyArea`;
  their codecs and the zone codec share `networkCodec::exactFields`.
- **Done (D568, MP1–MP3):** the root `map`, with the codec's ranges; the root key table is `rootFeatures`.
- **Done (D569, MR1–MR3):** `definition.macroSettings` and the root `macroRuns`.
- **Done (D570, TR1–TR3):** the root `tripRateSets`; its codec checks keys with `knownKey`.
- **Done (D571, DS1–DS3/CM1–CM3):** root `deterrenceSets` and `costMatrices` at 67, with
  `constraint` from 68 and `destination` from 72; both readers use the shared key tables.
- **Done (D572, PF1–PF3):** root `periodFactors` at 69 uses a shared closed required-field
  table via `exactFields`; share bounds are structural, while blank text, sum ≤ 1, known periods
  and unique pairs retain their existing codec checks and order.
- **Done (D573, MCS1–MCS3):** root modeChoiceSets 70, nests 71 and logsum 76 use shared
  closed key tables through knownKey at 76; alternative cost sources use an inclusive anyOf.
  Feature extensions D566–D573 are in PROJECT_SCHEMA_FEATURES after the whole-section split.

## From: M11.5 Meso assignment — S1 done (D550); stop for the owner

- **A2b done (D551):** placed and positioned decisions join the loop (DYNAMIC_ASSIGNMENT §15–§16, DA10–DA13; MA9 as
  amended in MESO_ASSIGNMENT §10.1). Positioned decisions are refused for Meso (neutral weights in its loading); a placed
  decision cannot choose between routes to one destination (shared family).
- **A4 done (D552):** the *Dynamic assignment* Results page at the Micro level (DYNAMIC_ASSIGNMENT §17–§18, DA14–DA17):
  Run/Cancel on a worker, δ and cell tables, *Apply weights* as one Undo step, *Save route costs…*. **Owner:** C17.

## From: M9 dynamic assignment — A1–A3 done (D549); stop for the owner

- **A1 done:** `src/project/dynamic_assignment.*`, the pure loop; Example 10.6 reproduced (DA1–DA3, group `dynassign`).
- **A2 notes (read 2026-11-13):** a positioned decision emits `RoutingEvent` (time, decision, to-route); an
  unplaced M2.4 decision is split at compile into inputs `<input>/route-<routeId>` and a placed one into
  `<input>/path-k` (`withRoutingDecisions`, `expandRouteless`), so no event names it. `DepartedEvent` carries
  `scheduledTime`. Mapping a vehicle to its entry needs the compiled route's families or link sequence.
- **A2 done (unplaced decisions only, §11):** `dynamic_assignment_run.*`, `route_costs.*`, `--route-costs` (DA4–DA6, DA9, group `dynassignrun`). Placed and positioned decisions are refused as `DYNAMIC_ASSIGNMENT_UNSUPPORTED` until A2b.
- **A3 done:** `runDynamicAssignment`, `--dynamic-assignment OUT [--iterations N] [--gap G]`, `fixture::twoRoute()`
  (DA7–DA8, group `dynassignloop`). Amended after the first run (§13.1, figures recorded): start 19 : 1, since 1 : 1
  was already inside the gap; the 5 % cost criterion dropped as inconsistent with δ. Converged at n = 2 (δ 0.0108).
- **Owner:** whether the M9 gate's rows (DYNAMIC_ASSIGNMENT §14) close it (rule 8). **Next on the owner's pick:**
  A2b (placed and positioned decisions), A4 (editor, weights into the project), or M11.5 reusing the loop.

## From: M10 four-step — G1–G2, D1–D4, E1–E2, N1, P1–P2, F1–F2, L1–L2 done (D470–D485, D547, D548); stop for the owner

- **Feedback (D547, done):** a procedure's `feedback` (schema 75) iterates distribution and mode choice on the
  assignment's congested times, averaging the OD-mode matrices by CW (book `w`, 0.25 proposed) or MSA, stopping on
  TMF. Ortúzar Example 11.3 reproduces the hand fixed point (CW 999.80 / 999.07 against 999.79 / 999.08). Naive
  feedback converged at λ = 0.34 here, against the book's Figure 11.3 (FB7 amended to λ = 0.6, §5). **Owner:** the
  dialog and page on Windows.
- **Logsum (D548, done, [MODE_CHOICE_LOGSUM](../reference/MODE_CHOICE_LOGSUM.md)):** a mode-choice purpose's
  `distributionCost: "logsum"` (schema 76) gives its distribution Ortúzar Eq. 6.10's composite (G/D/MS/A); β > λ is
  refused (Eq. 6.11); one shared cost coefficient. Example 6.3's C_B 49.23 and a hand table reproduce. Switched in the
  *Nests* dialog. **Owner:** that dialog on Windows. G/MS/D/A is not built (D548 choice 1).

## From: Next session — the owner's pick

  - **Results (D458, [evidence](../evidence/gap-seeking-changer.md)):**
    - No seed of 10 locks at any factor (×0.3–×1.2), off or on.
    - Clamps are 0 at every factor, off and on.
    - Switched on, every vehicle is served through ×1.0. Mean delay is 15/12/14 s at ×0.8 and
      87/44/35 s at ×1.0.
    - Switched off, results are byte-identical to D456.

## From: Next session — the owner's pick

- **Clamps:** 5 left on M2.6 (pull-out 1, late catch-up 4), the lane lab's 1 (D252) and source
  clearance (D108); `joinAnticipation` ships off (D255); D95 stays off.

## From: Next session — the owner's pick

- **Engine spread costs, not booked:** `closestVehicle` 8 %, per-tick `Vehicle` moves 6 % (D283).
