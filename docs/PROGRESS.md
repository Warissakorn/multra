# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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
