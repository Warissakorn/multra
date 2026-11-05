# PROGRESS archive — 2026-11-03

Whole entries moved from the live PROGRESS log.

## 2026-11-04 — The yielding partner brakes as cooperative braking does (D454)

- **Owner:** asked for a recommendation, then "do D454".
- **Cause** ([weaving-yield-cap](../evidence/weaving-yield-cap.md)): a front stub reaching its dead end
  fast stops harder than comfortable. Its rear partner, capped at comfortable braking, stood inside
  the standstill distance, and the pair locked. This was P2b, and the off ×1.0 remainder.
- **D454 (M3_8_CONTRACT §2, D450 (a) revised):** once the front one closes in, the rear partner
  brakes up to `maxDecelerationCooperativeBraking` (M3.2.8c's key, 3 m/s²). No new parameter.
- **Rows:** BA192 and BA193, both failure-first.
- **Results:**
  - Product runs are identical except the lane-change lab on (delay −0.00005 s, clamps 0).
  - Lab on: one lock is left from ×0.5 to ×1.0 (seed 50, 500 m).
  - Lab off at ×1.0: 0/2/1 (from 0/3/2).
  - Cost: a 205 s wait on at ×1.0.
- **Track:** the weave-end lock track is closed by the recommendation. The leftovers are in NEXT.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — A change under way that may not land claims nothing (D453)

- **Owner:** "trace P2a first", then "do B+C+E as D453".
- **Traced** ([weaving-held-change](../evidence/weaving-held-change.md)):
  - A two-change stub's crossing was held at the lane edge for 170 m beside lane-1 traffic.
  - It then landed beside a waiting partner, because D450 (b) was checked only at the start.
  - Checking (b) on every step was not enough. The held change still made its partner yield short
    of the dead end, where (b) lapsed, and it still reserved its target place (D206).
- **D453 (M3_8_CONTRACT §2):** a change under way that (b) refuses takes no outward step, is no
  partner for D451 (c), and holds no target place. Switched on only.
  - Every subset was measured; only B with E, and B with C and E, cut the locks, to 3 each.
- **Rows:** BA190 and BA191, both failure-first.
- **Results:**
  - Product runs are identical except the lane-change lab switched on (delay +0.003 s, clamps
    unchanged).
  - Lab switched on: locked seeds at ×0.6 0/1/0, at ×0.8 0/0/1, at ×1.0 0/1/0. The 237 s wait is
    gone.
  - Lab switched off: identical.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — No courtesy for a change that may not land (D452)

- **Owner:** "trace the remaining switched-on locks first".
- **Traced** ([weaving-lock](../evidence/weaving-lock.md#d452-no-courtesy-for-a-change-that-may-not-land)):
  three patterns.
  - **(P1) the courtesy cycle:** a lane-1 follower held back for a two-change stub whose landing
    D450 (b) refuses, and stood short of a waiting lane-0 stub's place, so neither went.
  - **(P2a):** a held crossing resumes beside a partner.
  - **(P2b):** a late hard stop at the dead end beats the comfortable yield.
- **D452 (M3_8_CONTRACT §2):** no courtesy for a changer whose landing (b) refuses. It is one line in
  `courtesyHolds`, with no parameter and no state.
  - **Tried and dropped:** re-checking (b) as a held crossing resumes (P2a). It added locks at ×0.8.
- **Rows:** BA188 and BA189, both failure-first.
- **Results:**
  - Product runs are identical to D451, off and on.
  - Locked seeds on: ×0.5 0/0/0, ×0.8 1/0/0, ×1.0 0/2/0 (from 1/1/0, 4/5/0 and 1/4/3).
  - Locked seeds off at ×1.0: 0/3/2 (from 6/6/5).
  - Longest waits: off ×1.0 down from 510 s to 28 s.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Switched-on weave-end lock: a change under way hid the pair (D451)

- **Owner:** "trace the switched-on case first".
- **Traced** ([weaving-lock](../evidence/weaving-lock.md#d451-switched-on-a-change-under-way-hid-the-pair)):
  a lateral crossing (about 3.5 s) left the changer no one's partner until it crossed, so an
  approaching partner drove on and arrived too fast to yield. Two more traps were found on the way:
  a dead end at a join (D444), and a landing stub that drives on to its own dead end.
- **D451 (M3_8_CONTRACT §2):**
  - (b) revised: a partner is taken where it will wait, and the landing stub takes the lane up to
    its dead end;
  - (c) new: a change under way counts as on its target route for D207 (`yieldMutualUnderWay`).
  - **Deviation from the plan:** I had rejected (c) on its own, but the trace showed it is needed
    together with (b).
  - **Dropped:** a "could not stop comfortably" condition; it added a lab clamp.
- **Rows:** BA186 and BA187, both failure-first.
- **Results:**
  - Product runs are unchanged but for the lane-change lab's Diverge (clamps 0, delay ±0.01 s).
  - The weaving lab off is lock-free through ×0.8. On, its locks at ×1.0 fall from 10/10/10 to
    1/4/3 seeds.
  - Longer waits: up to 237 s on at ×0.6, and 510 s off at ×1.0.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — The weave-end lock, partly fixed (D450)

- **Owner:** "fix the weave-end lock first".
- **Traced** ([weaving-lock](../evidence/weaving-lock.md)). Two ways a pair at a shared dead end locks
  for good:
  - D207 dropped the rear partner whenever it fell back, and it drove up into the standstill
    distance once the front one stood;
  - a two-change stub moved into the place a waiting partner needed.
- **D450 (M3_8_CONTRACT §2):** (a) the pair is kept while the front one brakes for or waits at its
  dead end; (b) `landsInWaitingPlace` refuses such a landing. No parameter, no state, and both
  switch modes.
- **Rows:** BA184 and BA185 were written failure-first, and BA183 turned from pinned-open to
  passing. Each rule is needed.
- **Results:**
  - All four product projects are identical apart from the build commit.
  - The lab switched off is lock-free through ×0.6 (×0.4 before). Locks remain at ×0.8+ and
    switched on.
  - Two experiments, not kept, are in the evidence. Harder yield braking is the owner's choice.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Weaving lab, step 1: fixture and off/on sweep (D449)

- **Owner:** asked how other tools test weaving, then agreed to step 1: a synthetic fixture and an
  off/on measurement, contract first ([WEAVING_FIXTURE](../reference/WEAVING_FIXTURE.md)).
- **Built:**
  - `data/projects/weaving-lab.traffic.json`, from `tools/weaving_network.hpp`, regenerated with
    `multra-weaving-fixture`: three Type A weaves of 150, 300 and 500 m;
  - `multra-weaving-sweep`;
  - rows BA179–BA183 in `weavinglab`.
  No engine or schema change.
- **Found** ([evidence](../evidence/weaving-lab.md)):
  - A weave with two entry Links does not compile its crossing routes, because a change is
    compiled only on the entry Link. So the lab starts the weave at the network edge.
  - **The weave end locks for good** once the two lane heads stand short of each other's gap, since
    nothing is removed (M3 §4). The weave is lock-free through ×0.4. Locks begin at ×0.5. At ×1.0
    every seed of the 150 and 300 m weaves locks, off and on. Switched on locks earlier.
  - BA183 pins the lock as open.
- **Also:** the shipped-project tests count five files (SP1), and N1, MA1, NA8 and the Meso check
  include the lab.
- **Verified:** `ctest --preset headless` passes 175/175; `check` passes.

## 2026-11-04 — Lateral track closed; owner checks prepared

- **Owner:** chose to close the lateral track and turn to the pending owner checks. Gap seeking is
  deferred to M6 calibration or an explicit request (NEXT).
- **Prepared and verified** (Linux; no Windows desktop build in this container):
  - **UT10:** `tests/data/utdf/u2-u0-junction-geometric.csv` is byte-identical to today's
    `multra-cli --export-utdf` of `u2-u0-junction.traffic.json`, and `utdf` passes.
  - **A9:** the only Node-analysis code change since `086a4fd` is `32ab14a`, which moved the
    peak-hour-factor default into the shared `nodeAnalysisPeakHourFactor` for UTDF, with the same
    formula. `nodeanalysis` passes.
  - **Canvas:** the five `canvas-labels-after` images CANVAS_STYLE links are present.
- **Verdicts:** the owner's, on OWNER_SITTING I.5 and E32 and on the canvas review in NEXT.

## 2026-11-04 — Probe: the long m2.6 lane-change waits (no code change)

- **Owner:** "probe การรอนานใน m2.6 ก่อน", before deciding on gap seeking.
- **Method:** scratch probes, not committed.
  - `stubwait`: time on a route that still needs a change, a metric both runs share. Off and on,
    seeds 42–51; its arrival totals equal the CLI batches.
  - `refusal`: per-tick reasons for the three long waits.
- **Result** ([m26-long-waits](../evidence/m26-long-waits.md)):
  - The long waits exist switched off (max 110.9 s; 61 over 30 s). They are signal-queue
    congestion on the two `path-4` approaches.
  - Switching on adds ≈ 2 s per change (the crossing time) and 13 more waits over 30 s.
  - The three long waits are mostly queued with the target place occupied (21–74 s). Being refused
    while moving, with the target lane's leader too close, is only 4–18 s; that is the part gap
    seeking would reach.
- **Next:** the owner's choice on gap seeking (NEXT). No decision ID and no engine change.

## 2026-11-04 — The start's target-lane filter at the window's end (D448)

- **Owner:** "ทำข้อถัดไปต่อเลย": NEXT's last code copy left from D444.
- **Cause:** the start's `sideways` filter took `frontPart` on the target route. At a join at the
  window's end that is the downstream Connector, so every span on the target lane counted. This was
  conservative: a start a correct filter accepts was refused.
- **Change:**
  - `targetPartAt` returns D444's part, and `targetSectionAt` wraps it.
  - `followerFilterOn` takes the part. `followerFilter` wraps it, so other callers are unchanged.
  - Both start sites read the target lane through these.
- **Row:** BA162 (control accepted at 299 m; refused at the 300 m join before the fix, accepted after).
- **Result:** every audit batch, off and on, and the duration probe are byte-identical to D447's.
- **Verified (Linux, headless only):** `ctest --preset headless` 174/174 and `check` pass.

## 2026-11-04 — A blocked body over the line steps back (D447)

- **Owner:** chose D208's rule ("never waits straddling the line") over D206's BA53, which kept
  `lateral` over the line when a car alongside blocked the move.
- **Change:** `moveLaterally` has one step-back path, used when the move is blocked in 2D and when
  D208 refuses the cover. A body inside its lane waits; one over the line steps back by up to one step
  and keeps its target. This closes the review's path (b).
- **Row:** BA53 revised failure-first. It failed on the old code (`lateral` kept) and passes after.
- **Result:** every audit batch, off and on, is byte-identical to D446's, and so is the
  change-duration probe. No product run reaches the case now.
- **Verified (Linux, headless only):** `ctest --preset headless` 174/174 and `check` pass.

## 2026-11-04 — Swap lock: the rear partner too close to accept yields (D446)

- **Owner:** approved the plan for lateral audit Finding 3.
- **Cause (lab seed 49, vehicles 83/88):** a swapping pair. Both starts were accepted at low speed,
  then D208's covering test refused each body over the line because the other was too close. D207's
  partner test ends at the standstill distance (2 m), and the rear one was just beyond it, so nobody
  yielded. The pair rode side by side for about 50 s. Refusing the second start would not have
  helped.
- **Change:**
  - While a change is under way, the D207 partner also covers a rear stub that would not accept the
    changer as its leader.
  - `acceptsAsLeader` is the one follower test, shared by D208 and D207.
  - `yieldMutual` moves to the new `lane_swap.cpp`: `lanes.cpp` was at 496 lines.
- **Row:** BA161 failed before the fix (the pair crossed only at 14.7 s) and passes after.
- **Result:**
  - Switched off, every batch is unchanged.
  - Switched on, completed trips and clamps are unchanged.
  - Lab changes over 8 s fall from 36 to 27, and the lab pair now crosses 1.7 s after starting.
  - Residue: a changer alongside a car that is not changing, at a similar speed, still waits (lab
    seed 49, vehicle 580, 36 s). That is the gap-seeking case, not a swap.
- **Verified (Linux, headless only):** `ctest --preset headless` 174/174 and `check` pass.

## 2026-11-04 — Straddler searched a covered lane's next section from its start (D445)

- **Owner:** "ทำเลย": lateral audit Finding 2, reproduced before any change.
- **Cause (m2.6 seed 42, vehicle 1133):** `followerFilter` listed both sections of lane-17 as covered.
  The second starts 25 m ahead of the front; its mapped station was clamped to 0, so the D210 search
  started there and found a standing car at gap 0. The car-following and the hard cap shared that one
  search, so the hard cap held 1133 and it was clamped 13 times. The review's suspects were not
  the cause.
- **Change:** a covered lane is listed only when the mapped front falls within that section (LATERAL §6).
- **Row:** BA160 failed before the fix and passes after.
- **Result, switched on:** m2.6 clamps fall from 28 to 1, the switched-off count, in every seed. Seed 51
  completes 1965, as it does off. Seed 42's longest straddle falls from 5.8 s to 2.4 s. Every
  switched-off batch is unchanged.
- **Verified (Linux, headless only):** `ctest --preset headless` 174/174 and `check` pass.

## 2026-11-03 — Lateral crossing at the window end (D444)

- **Owner:** agreed to the smallest fix first, after the review of the lateral audit: reproduce
  Finding 1, unify the crossing test and clamp the body. The lane-change state machine waits.
- **Cause (m2.6 seed 45, vehicle 613):** the span window [240, 277] ends at a join on the target route.
  At the dead end, `frontPart` put the mapped front on `connector-53`, so `crossingOn` found no crossing
  and the centre clamp let the body stand on the line. `lateralStart` read the same part, which gives
  a target of 0 to a change started there.
- **Change:**
  - `targetSectionAt` (`lateral.cpp`) is the one target-lane lookup for the start and the crossing.
  - `spanHolds` (`lanes.hpp`) replaces four copies of the coverage test.
  - On a side no span covers, the whole body stays in its lane (LATERAL §7; BA44 now pins this).
- **Rows:** BA158 and BA159 failed before the fix and pass after.
- **Result, switched on:** m2.6 seed 45 completes 1531 → 1903 (off: 1908). No vehicle straddles at the
  end of any seed. m2.6 clamps fall from 33 to 28. The lab, four-leg and T-junction runs on equal their
  off runs.
- **Open:** Finding 2 (seed 42: 13 clamps) and Finding 3 (the swap lock) are not addressed. The
  `followerFilter` window-end lookup is recorded in the evidence.
- **Verified (Linux, headless only):** `ctest --preset headless` 174/174 and `check` pass. There was
  no desktop build here: Qt is not installed in this container.

## 2026-11-03 — UTDF U2b: desktop export, Number Nodes, UTM

- **Numbering:** template Apply numbers an unnumbered Node (`generateJunction`), and re-Apply keeps
  the number. `numberNodes` and `setNodeNumber` are in `node_commands`.
- **Nodes page:** a Number column, a Number field in *Rename*, and *Number Nodes* (one Undo step).
- **Export:** *File → UTDF for Synchro/Vistro…* (`utdf_export_dialog.*`).
  - It shows the report, with the mode and the NEMA opt-in, before the file dialog.
  - It writes atomically and refuses the open project.
  - It writes UTM through `GeoProjection::worldToGrid` when the project has a map reference.
- **Translations:** en/th strings for every `UTDF_*` code and the dialog.
- **Tests:**
  - `utdf-export-ui` (UT9): the desktop writes the CLI's expected files byte for byte in both
    modes. It also covers cancel, protection, Number Nodes and Thai.
  - `map-geo` checks `worldToGrid`.
  - `node-ui` now expects the 9th column.
  - The jt6 fill now writes schema 65.
- **Verified:** headless `utdf`, `junctionfill` and `nodeanalysis` pass, and so do desktop
  `utdf-export-ui`, `map-geo`, `node-ui` and `object-export-ui`. `multra-check-docs` passes.

## 2026-11-03 — UTDF U2a: phases, volumes and timing (D443)

- **Owner:**
  - U2a, then U2b, in one session;
  - a Node whose controller fails NEMA is left out unless the user opts to export it unsignalised;
  - the Vistro median patch stays a report step until Vistro's link numbering is confirmed.
- **Writer:** `utdf_signals.*` reads the Node's controller through its heads and checks it with
  `nemaCheck`.
  - **[Timeplans]/[Phases]:** every Synchro 10.1 record. All-red is the ring gap, and the D439
    values fill the records MULTRA does not author.
  - **Lanes:** `Phase1`/`PermPhase1` per UTDF_PROBE §5. Volumes, PHF, HV, ideal flow and bus stops
    come from `analysis`, with A1's HCM defaults listed. A1 and UTDF now share one PHF rule
    (`nodeAnalysisPeakHourFactor`).
  - **Report steps:** median (`UTDF_MEDIAN_STEP`) and HCM 7th. Mirrored mode keeps the group
    numbers, which are already right-hand NEMA numbers.
- **CLI:** `--utdf-unsignalised`.
- **Tests:** the U0 junction now carries the §2 volumes and the §3 controller.
  - It reproduces `u0-handwritten-v3.csv` except `[Network]`'s values and the external ends'
    `Grade`. `[Timeplans]`/`[Phases]` match byte for byte.
  - P1–P4 cover both driving sides (UT7).
  - The four-leg A1 fixture covers the NEMA refusal and the opt-in (UT8).
  - Mirrored mode is covered (UT11).
  - The CLI fixture is renamed `u2-u0-junction*`.
- **Verified:** headless `check` passes 174/174; `multra-check-docs` passes.

## 2026-11-03 — UTDF U1: the geometry writer (D442)

- **Owner:** chose world metres for U1, with UTM in U2 on the desktop, because PROJ is not in the
  headless build. Mirrored mode is in U1. Recorded as D442; the contract is UTDF_EXPORT §7.
- **Schema 65:** Node `number`, an optional positive integer. A repeat is `NODE_NUMBER_DUPLICATE`.
- **Writer:** `utdfDocument(document, mode)` (`utdf_export.*`, with the leg walk in `utdf_legs.*`)
  writes [Network], [Nodes], [Links] and [Lanes]:
  - legs walked to the next Node or to an external end;
  - D440 lane counts, with pockets on the owning turn;
  - every empty authored field listed in the report.
- **CLI:** `multra-cli --project P --export-utdf FILE [--utdf-mode geometric|mirrored]`. It refuses
  an existing file and writes nothing when no Node can be exported.
- **Tests:** group `utdf` covers UT0–UT5 and UT11's geometry. The U0 junction rebuilt in MULTRA
  reproduces every Node 1 `[Links]` line and the ten `[Lanes]` geometry lines of
  `u0-handwritten-v3.csv`. `cli-utdf` compares the CLI's files byte for byte.
- **Verified:** headless `ctest` 174/174 and `check` pass; `multra-check-docs` passes.
- **Next:** the owner imports `u1-u0-junction-geometric.csv` into Vistro (UT10). After that comes U2.

## 2026-11-03 — Vistro limits closed (D441)

- **Owner:** both whole-network merges (`u0-full-lh-a/-b`) create a second, overlapping junction and
  stay right-hand; only `a` gets the median. The owner stopped probing.
- **D441:** Vistro gets the UTDF file plus a `.vistromerge` median patch. Mirrored mode (D437)
  gives a correct far-turn HCM. The report covers HCM 7th and the right-hand result of geometric mode.
- **Updated:** UTDF_PROBE row 21, UTDF_EXPORT §6/§9 UX-6/UT12, the README, NEXT and OWNER_SITTING I.4.
- **Open for U1:** how the patch learns Vistro's post-import link numbers. `multra-check-docs` passes.

## 2026-11-03 — UTDF cannot be left-hand in Vistro; whole-network probe

- **Owner (patch v2):**
  - The median merged.
  - Left-hand and HCM 7th did not merge.
  - Vistro refuses to switch to left-hand while a right-hand junction exists.
- **Owner choice:** try the whole network in Vistro's own format, merged into an empty left-hand project.
- **Added:** `u0-full-lh-a/-b.vistromerge`, generated by a script from the structure of the owner's
  post-import export (not committed) and holding U0 sheet data only. `b` mirrors lane indices.
  A script check finds every lane-turn and signal-group index in `$LANE`.
- **Updated:** UTDF_PROBE rows 20–21, UTDF_EXPORT §6, the README and NEXT. `multra-check-docs` passes.

## 2026-11-03 — Merge Data patch v1 keeps right-hand; v2

- **Owner:** merging `u0-patch-v1.vistromerge` after the v3 import left the project right-hand.
  They also sent Vistro's post-import network export; it is not committed (owner).
- **Recorded (structure only):**
  - Vistro renumbers the end nodes (100100…) and numbers the links itself (227 N, 318 E, 409 S, 500 W).
  - Default values are omitted from the export.
  - Pockets are `$LANE` rows with a length.
  - The signal groups match §3. The offset is restated as 44 s LAGEND.
- **Added:** `u0-patch-v2.vistromerge`, in the export's own shape, with HCM 7th and a 2 m median on
  legs 318/500. UTDF_PROBE rows 18–20, utdf-real-exports, the README and NEXT are updated.
  `multra-check-docs` passes.

## 2026-11-03 — U0 limits: a Vistro Merge Data patch

- **Owner:** asked how to handle UTDF's limits: the import switches to right-hand, there is no
  median, and Vistro opens imports as HCM 2010. Vistro has *File → Import → Merge Data*.
- **Found:** the owner's Vistro network export (Visum net format 19) has `$NETWORK
  LEFTHANDTRAFFIC`, `$NODE ICAMETHODSIGNALIZED`/`USELOCALICAMETHOD` and `$LEG HASCISLAND`/
  `CISLANDWIDTH`/`CISLANDLEN`.
- **Added:** `u0-patch-v1.vistromerge`, which sets left-hand and HCM 7th for node 1. `.gitattributes`
  now covers `*.vistromerge`. UTDF_PROBE rows 18–19, UTDF_EXPORT §6 (proposed companion file),
  the README and NEXT are updated. The median patch waits on Vistro's link numbers after import (row 19).
- **Check:** `multra-check-docs` passes.

## 2026-11-03 — U0 golden file committed

- **Owner:** exported again from Vistro and accepted it as the golden file, `u0-vistro.csv`
  (committed byte for byte). It matches §1–§2 with a 100 s cycle and protected-only far turns:
  EBR is phase 3 and WBR phase 7. The phase numbers, the lagging order and the splits are
  Vistro's own, not §3's, so UT0 compares fields and codes, not timing values.
- **Updated:** UTDF_PROBE §6 row 16, UTDF_EXPORT UT0, the utdf README, OWNER_SITTING I and NEXT.
- **Row 17:** `u0-handwritten-v3.csv` (Links `Lanes` 3) imports with the pockets as pockets and the
  same HCM 2010 result as v2-filled: 186.72 s, LOS F. D440 holds, and U0's probe questions are answered.

## 2026-11-03 — Vistro draft 2 of U0; Links Lanes counts pockets (D440)

- **Owner:** re-exported the U0 junction from Vistro. Draft 2 matches §1–§2: lanes, pockets,
  names, volumes, PHF 0.92 and HV 5 %. The timing is still 60 s with four phases; the owner will
  set §3 and export again.
- **Owner choice (D440):** `[Links] Lanes` counts the pocket, 3 at the U0 junction. Vistro's own
  writer counts only full lanes (2) and puts pocket storage on the through column. MULTRA keeps
  storage on the turn column, which Vistro reads.
- **Committed:** `u0-vistro-draft2.csv` byte for byte, and the probe `u0-handwritten-v3.csv`, which
  is v2-filled with the junction legs' `Lanes` 3 (UTDF_PROBE §6 row 17).
- **Fixed:** UTDF_EXPORT §3 still said MULTRA never writes unauthored values; it now cites D439.
  Rows 15–16 of UTDF_PROBE §6 had text outside their last cell.
- **Check:** `multra-check-docs` passes.

## 2026-11-03 — Vistro switches an import to right-hand; no median

- **Owner:**
  - The project was set left-hand before import; Vistro switched it to right-hand itself.
  - No median is shown.
  - The redone `u0-vistro.csv` arrived empty after the first column of every section, so it was not committed and a re-export is pending.
- **Recorded:**
  - UTDF_PROBE §6 rows 14–16.
  - UTDF_EXPORT §3/§6: in geometric mode the report must tell the user to set left-hand again after import, and that Vistro drops the median.
  - OWNER_SITTING I.3 and NEXT.
- **Check:** `multra-check-docs` passes.

## 2026-11-03 — Vistro imports the U0 timing; neutral values for unauthored records (D439)

- **Owner imports into Vistro 2026:**
  - `u0-handwritten-v2-empty.csv` is refused: `VehExt … D1 is invalid`. Empty cells are not allowed.
  - `u0-handwritten-v2-filled.csv` imports the timing exactly: controller 1, cycle 100, Ring 1 =
    1·2|4, Ring 2 = 5·6|8, splits 14/44/42/14/44/42. HCM 2010 then gives 186.72 s, LOS F, v/c 0.988.
- **Owner choice, D439:** the timing records MULTRA does not author get v2-filled's neutral
  pretimed values: constants and copies of authored values, each listed in the report. UT4 is amended.
- **Noticed:** the imports look drawn right-hand (row 15, to confirm).
- **Updated:** UTDF_PROBE §6 rows 13/15, UTDF_EXPORT §3/UT4, the utdf README, OWNER_SITTING I.3
  and NEXT. `multra-check-docs` passes.

## 2026-11-03 — U0: the refused timing leaves no cycle; HCM method is not in UTDF

- **Owner screenshot** of the original `u0-handwritten.csv` imported into Vistro: no cycle, empty
  rings, *ICA error*, all delays 0. This confirms the refused timing; the v2 files are not yet tried.
- **HCM method:** Vistro opens an import as HCM 2010. No UTDF record carries the method, and the
  owner sets HCM 7th after import. The export report should say so.
- **Updated:** UTDF_PROBE §6 row 6, UTDF_EXPORT §3 and utdf-real-exports. `multra-check-docs` passes.

## 2026-11-03 — Vistro draft 1 of the U0 junction

- **Owner:** exported the U0 sheet from Vistro 2026 (metric, left-hand on), allowed the commit, and
  will redo it to match the sheet.
- **Committed** as `tests/data/utdf/u0-vistro-draft1.csv`, byte for byte. It is a draft, not the golden file:
  cycle 60 with phases 2/4/6/8, PHF 1.00, HV 2 %, and different NB/SB lanes.
- **Settled:**
  - Vistro writes UTDF in feet even for a metric project, but reads `Metric,1`.
  - With left-hand on, Vistro writes the far turn under `EBR` (geometric, as D437's default).
  - Its 4-phase numbering is NB 6, SB 2, EB 8, WB 4.
  - Its `End` for the phase that ends the cycle is the cycle length; Synchro writes 0.
- **Updated:** UTDF_PROBE §6 rows 9/10/12/16, utdf-real-exports, UTDF_EXPORT §4/§6,
  OWNER_SITTING I.1 and NEXT. `multra-check-docs` passes.

## 2026-11-03 — Vistro imports u0-handwritten.csv; timing probe v2

- **Owner:** found Vistro's UTDF menus: *File → Export / Import → Synchro…*. They imported
  `u0-handwritten.csv`.
- **Came through as sheeted:** geometry, speeds, 3.25 m lanes, all four pockets, shared lanes,
  volumes and PHF.
- **Not shown:** the intersection median.
- **Timing refused:** `CellNotFound: Row not found: Row = 3, Column = 1`.
- **Added:** `u0-handwritten-v2-empty.csv` and `-v2-filled.csv`. Their `[Timeplans]` and `[Phases]`
  carry every record Synchro 10.1 writes, in its order; one leaves the unauthored rows empty, the
  other fills them. The import tells whether UT4 ("no unauthored values") survives.
- **Updated:** UTDF_PROBE §4/§6 rows 6 and 13–15, UTDF_EXPORT §3, OWNER_SITTING I.3 and NEXT.
- **Status:** U1 stays blocked. `multra-check-docs` passes.

## 2026-11-03 — U0 golden file moves to Vistro (D438)

- **Owner:** Synchro cannot create UTDF. They asked for another route and chose Vistro as the
  reference (D438).
- **Changed:**
  - UTDF_PROBE §4 now has Vistro steps: draw left-hand and metric, run HCM 7, export `u0-vistro.csv`, find the far-turn column, import the hand-written file, then the licence question.
  - UTDF_PROBE §6: rows 2/4/5/8 are closed as not possible; rows 9–12 are added.
  - UTDF_EXPORT §8, UT0 and UT10, OWNER_SITTING I.1–I.3, `tests/data/utdf/README.md` and NEXT are updated to match.
- **Kept:** Synchro's dialect stays recorded from the real 10.1 export. Synchro only checks the
  import, if it can import UTDF. U1 stays blocked. `multra-check-docs` passes.

## 2026-11-03 — Synchro is right-hand only; left-hand UTDF export modes (D437)

- **Owner answer (U0 item 4):** Synchro builds right-hand traffic only. The upload `2569-P01.csv`
  was an encrypted Synchro 12 project, not UTDF; nothing was read from it or committed.
- **Owner choice (UX-5, D437):** the user picks at export.
  - Geometric (default): real map and legs, far turn in `R`, and the report says the network is left-hand.
  - Mirrored: reflected to right-hand, far turn in `L`, right-hand NEMA numbers.
- **Updated:** UTDF_EXPORT §6/§9/UT11, UTDF_PROBE §6 rows 1 and 7, OWNER_SITTING I.4,
  utdf-real-exports and NEXT.
- **Still open:** the §1 export, both imports of the hand-written file, and whether Links `Lanes`
  counts pockets. U1 stays blocked. `multra-check-docs` passes.

## 2026-11-03 — U0 partly answered from real Synchro/Vistro exports

- **Owner supplied:** UTDF exports and project files from two real studies: Synchro 10.1, Vistro 2026
  (with its `.vistromerge`) and an encrypted Synchro 12.0 `.syn`. None is the UTDF_PROBE §1 junction.
- **Recorded, not committed:** licence and project data are unknown, so only section and field names
  and codes went into [utdf-real-exports](../evidence/utdf-real-exports.md). UTDF_PROBE §6 rows 3/7/8,
  OWNER_SITTING I and NEXT now say what is settled.
- **Findings:**
  - Node TYPE 3 is unsignalised.
  - A phase that ends at the cycle wrap is written `End 0`.
  - `Yield` = End − yellow − all-red.
  - Vistro's dialect adds U/L2/R2 movements, `D1…D32` phases and `Phase1…4`.
  - Vistro has a left-hand setting, but UTDF does not carry it.
- **Still open:** the §1 export, the far-turn column, Synchro's left-hand setting, both imports of
  the hand-written file, and whether Links `Lanes` counts pockets. U1 stays blocked; UTDF_EXPORT
  is unchanged.

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
