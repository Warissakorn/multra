# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the [decision log](decisions/RECORD.md) has its own indexed record. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long, and list each in
[`archive/README.md`](archive/README.md), which indexes every older entry.

---

## 2026-10-14 — M8.4a parking-lot runtime in the engine (D156)

M8.4 was two systems, so it was split (D156): this session the core runtime, next the authored
ParkingLot and its compiler. `src/core/parking_types.hpp` holds `ParkingLotSpec` (gate routes,
exits as route + queue input + weight, dwell curve, initial types by share); `SimState::lots`
holds parked vehicles, those leaving and those at the gate. `parking.cpp` runs it through five
hooks in `stepSimulation`: release after arrivals, `noteInserted`, admission from the snapshot,
the phase-1 hold (the dead-end mechanism) and drive-in at publish. Arrival at the line reuses the
Stop's test, now one function `atStopLine` in `conflicts.*`; `samplePending` was extracted from
`generateArrivals` with the same draw order so initial occupants are drawn like arrivals. Rows
PL1–PL7 and validation were written first on hand-built scenarios. A mutation dropping the
capacity check at publish survived them; PL2b (two vehicles driving in side by side for one space)
was added and kills it. Every hook returns at once without a lot and no draw is made: headless
86/86 (Linux, GCC 13.3), `trafficsim-cli 42` unchanged, frozen baselines green. No callgrind was
run; the lot-free cost is a few `empty()` checks per tick. Desktop not built (no Qt here).

## 2026-10-13 — M8.3 OD matrices into Micro demand (D155)

`withOdDemand` (`src/project/od_demand.*`) adds every matrix to a copy of the definition as
authored routes `od:<link>><link>` and inputs `od:<matrix>/<o>/<d>/<oc>><dc>`, at cell / period ·
3600 times both connector shares, split equally over the fewest-object drivable chains — the rule a
placed decision uses for a destination Link. The contract had said every complete path; that
would weight loops like the direct road, so §3 was rewritten (D155). Placed decisions were not used:
one per Link, and an author's own decision may sit on the same entry Link. `compileDocument` and
`runDiagnostics` read the expanded definition, so a matrix-only study runs; `odDemandIssues` is the
§6 Run tier (connector position, period beyond duration, unknown composition, no connector, no
path). Locale strings for every zone/OD code, English and Thai. Rows OD1–OD6 in `odexpand`.
OD3 first failed with seeds 42–81: 1,403 departures for 1,600 expected. The expansion was exact;
the first xorshift32 draw of a small seed is ≈0.003, so the first input's first arrival is 5–6
mean headways late. Not changed (frozen baselines); owner question B11, OD3 uses mixed seeds. A
400-trip fixture through `trafficsim-cli --seeds 42-45` completed ≈394 trips per run. Headless
85/85 (Linux, GCC 13.3); `trafficsim-cli 42` unchanged; desktop not built (no Qt here).

## 2026-10-13 — M8.2 zone layer codec and commands (D154)

`ProjectDocument::zones` holds Zones, zone connectors and OD matrices (types in
`src/model/demand/zones.hpp`, because M10/M11 compilers will read them). Three top-level arrays,
written only when non-empty, make the file schema 31; each is `UNSUPPORTED_FIELD` below 31 and a
malformed or empty one is `EDIT_ZONE_INVALID`. `zoneLayerIssues` is the contract's structural
tier, run by `validateDocument` on load and every commit; topology, the period against the run
and the composition are the Run tier, left to M8.3 (D154), because a half-drawn study is a legal
draft. `documentIds` was split out of `allocateId` so zone ids share the uniqueness rule, and new
ids skip zone-layer ids. Cascades: a Link delete drops its connectors, a split moves destination
connectors downstream, a Zone a matrix names cannot be deleted. Rows Z1–Z6 were written first
(`tests/zones_tests.cpp`, groups `zonescodec`, `zonescommands`). No editor surface yet (M8.6); the
compiler does not read the layer, so every run is unchanged. Eight older codec tests used
schema 31 as their "future" probe; they now use 32, as D151 moved them from 30. Headless 84/84
(Linux, GCC 13.3); `trafficsim-cli 42` unchanged. Qt is not installed here: desktop not built.

## 2026-10-13 — M8.1 zones, OD matrices and parking lots contract (D153)

Docs only, the default slice NEXT named once the D143 queue was empty. New contract
[ZONES_AND_PARKING](reference/ZONES_AND_PARKING.md): schema 31 Zone, ZoneConnector, OdMatrix and
ParkingLot; OD cells compile to ordinary Poisson inputs and entry routing decisions with equal
weight over the existing `routelessChains` paths, so the core sees no zone; the lot is the one
core addition (`ParkingLotSpec`, `LotState`), held at the gate like a red head with Stop-style
service, admitted only below capacity, so a full lot queues and spills back and never deletes a
vehicle. Choices (D153): connectors at Link ends only, because inputs start only on entry Links
and internal insertion has no contract; a lot zone's row gives destinations while departure timing
comes from the dwell; a departing vehicle keeps its space until inserted, so a blocked exit fills
the lot. Rows Z1–Z6, OD1–OD7, PL1–PL11 are failure-first. Owner questions (dwell source, outbound
timing, lot trips in movement tables) are OWNER_SITTING B10. No code; checks: docs and file sizes.

## 2026-10-13 — Measured optimization pass: parallel batch seeds (D152)

Baseline (Linux, GCC 13.3, 4 cores, Release): a 40-seed M2.6 batch 13.6 s, one M2.6 run
≈350 ms (callgrind: step 80 %, observe 18 %, compile ≈2 % since D140), editor redraw 2.6 ms and
hit test 1.4 ms on 40 junctions, cold release build 363 s (test TUs 826 of 1,412 s compile CPU).
The batch was the one large item: `runSeedsParallel` (D152) runs seeds on worker threads; the
first attempt put the threads in `src/runner/` and `check_architecture` refused `<thread>` there,
so the scheduler moved to `src/project/` and the runner is unchanged. 13.6 → 3.66 s; CSV
byte-identical to the sequential run; new tests cover identity, progress order, cancel and a
worker's exception. Owner check: MSVC timing on P-cores, and an editor batch staying responsive.
Build: `trafficsim-tests` compiles at `-O1` in Release (GCC/Clang; libraries keep `-O3`):
its 88 objects rebuild in 101.5 s instead of 151.5 s (one run each, `-j4`); same 16 warnings.
Dropped after measuring: reusing the arrival snapshot as the tick's main one qualifies on
5,242 of 36,000 M2.6 ticks (≈1–1.5 %, under the 2 % bar); recorded in NEXT's engine-cost note.

## 2026-10-13 — Route destination markers and GUI interaction fixes (D151)

Implemented the owner's four GUI requests: the destination crossbar follows the
pointer within a road and re-reads the final click, then persists in schema 30 as
editor-only authoring metadata. Hover moves only the line, not the scene/path.
Undo/Open preserve it; deletion, route editing, reversal and split handle its references.
The deletion regression exposed a pre-existing missing Link id in the route cascade;
removal now includes the owning Link as well as its lanes. Runtime boundaries stay unchanged.
Wrapping labels keep height-for-width and natural hints capped to the current page,
not the old wrapped width. Screenshot review exposed over-wide form labels and
fields smaller than their font minimum; both now have resize/floating-dock regressions.
A shared application filter covers later dialogs: wheel events never
change spin-box/closed-combo values, panel wheels scroll, and canvas zoom remains.
Tooltips are plain text, wrap Thai/unbroken names, stay inside the owner/screen, and
scroll when long; modal ownership retains field focus and allows tooltip scrolling.

Verification: restored desktop build and all 123 CTest entries passed. Three
incomplete executables left by the interrupted build were rebuilt and rerun.
Architecture, file-size/docs and whitespace checks passed. GCC 13.3, Qt 6.4.2
and PROJ 9.4.0, Linux offscreen. Settings/reflow/tooltips cover
1x/1.5x/2x. The temporary toolchain needs its explicit PROJ_DATA path; no build
dependencies or machine paths were added to the repository. Windows CI and the
owner's look remain open. A transient workspace rollback required reconstruction
of the final corrections from this session before publishing; the earlier local
commit is not available in the restored clone.

---

## 2026-10-12 — OSM detailed implementation review (D150)

The owner asked for another detailed review. Seven focused cases failed on the
first implementation: corrupt cache sent obsolete validators, 16-bit PNGs broke
the decoded-byte assumption, Fit without a centre change kept old tile levels,
date-line views became world-sized, accepted anchors exceeded Qt scrolling,
quoted max-age received fallback freshness, and mismatched 304 ETags reused bytes.
Each now has a passing regression. Additional source-review fixes bound 512-pixel
wanted sets, preserve long Retry-After delays, use 64-bit candidate counts and locate
the shipped proj.db even with a custom content directory. HTTP policy is factored
into one helper; schema/dialog share the reachable anchor bound. Core/eval unchanged.
The Expires/Age regression also exposed Qt 6.4's rejection of HTTP GMT timestamps;
HTTP dates now use explicit UTC normalization, including legacy forms.

Local map-ui passed, including deterministic cache policy/bootstrap/budget cases,
both date-line hemispheres, rotation and high latitude.
Headless full build/check passed 82/82 CTest tests. The original commit passed
all five native CI jobs, including Windows desktop/core (run 38198105807).
Desktop full build/check passed 120/120, architecture/file-size/docs passed, and
installed-database projection tests passed. Latest-commit CI is recorded in PR #138; those checks
do not close the owner's real-study alignment/responsiveness review.
Detailed evidence: [OSM_REVIEW](audits/OSM_REVIEW.md). Pending work: [NEXT](NEXT.md).

---

## 2026-10-12 — OSM map background (D150, M1.28)

Implemented the owner's OSM background plan: optional schema-29 map reference/settings,
strict scalar/key/zone validation, one atomic History command and old map-free bytes.
The desktop's Qt-free PROJ module uses one fixed WGS84 UTM CRS and translated/rotated
anchor; the existing Canvas camera owns both map and roads. Tiles are adaptive triangle
paint beneath image/roads, not scene items or Fit network bounds. Tile arrival repaints
background only. Providers are data; localized Map settings/navigation/show/remove
commands, status and visible attribution complete the UI.

Qt Network supplies bounded viewport-only requests and manual QNetworkDiskCache
freshness/validators, decoded LRU, timeout, 429/backoff, invalid-image rejection and
cancellation. Explicit cache handling was needed because Qt's automatic cache served
an expired undated max-age fixture without contacting the server. Local fixtures now
pin fresh disk hits, expired ETag/304, seven-day fallback and no-store. PROJ stays out
of headless persistence/core/eval; desktop packaging copies proj.db, Windows workflows
install/copy projection DLLs and Qt deployment handles Network/TLS. Future-schema tests
now use 30 rather than the newly supported 29; frozen trajectory fixtures are untouched.

Verification: Linux GCC 13.3, Qt 6.4.2 and PROJ 9.4.0. Baseline native build/tests passed;
mapproject (5 cases), map-geo and local-server/render/editor map-ui passed. Headless CLI
build and seed-42 baseline passed without Qt/PROJ linkage; install staging contains the
projection database. Full desktop build and `check` passed: 120/120 CTest tests, architecture, file-size
and documentation checks. Windows CI and
MAP10 owner alignment/responsiveness remain pending; no engine/survey gate was closed.
Contract: [MAP_BACKGROUND](reference/MAP_BACKGROUND.md). Next review: [NEXT](NEXT.md).

---

## 2026-10-10 — M5.8b scenario comparison in the editor (D149)

Results → *Compare with…* runs the open project as the **base** against a chosen
`*.traffic.json` as the **alternative**, over the Seeds field: the roles of `--project` and
`--compare`, so Export is byte for byte the CLI's `--compare --csv` for the same saved files and
seeds (checked once by hand with `cmp`, and by row EC2). Copy is `csvToTsv` of the same text. The
table shows base and alternative n and mean, the difference and its ±95 % half-width per movement,
section and approach; the note carries the marker, both names, each side's warnings, the unmatched
names and the network delay difference. A difference gets no LOS letter.

How it is built and why:
- **One worker.** M5.6's thread plumbing was factored into `startWorker(total, comparing, job)`,
  where the job runs on the worker over value copies and returns what installs its result. The
  batch and the comparison share the generation number and Cancel, so M5.6's guarantee holds for
  both: no table shows fewer runs than its n. A comparison counts 2N runs, base first.
- **Refuse before clearing.** A bad list, an alternative that cannot be read or compiled, a base
  that does not compile and different evaluation periods are refused before any run and before
  the results on show are cleared. An alternative's issues are one line naming its file, never
  rows in the open project's Problems tab, whose paths they are not.
- **Honest names.** A dirty base is named `NAME (unsaved edits)` and a fileless one
  `unsaved project`, so the CSV never names bytes that did not produce its side.
- **A bug the new test caught.** `startWorker(seeds.size(), ..., [seeds=std::move(seeds)]...)`
  left the order of the size read and the move to the compiler; GCC moved first and the batch
  started with a total of 0, so `batch-run-ui` failed at once. The total is now read before the call.

Contract and rows EC1–EC7: [BATCH](reference/BATCH.md) §7, *Editor comparison*; test
`compare-run-ui` (`tests/compare_run_ui_tests.cpp`, 25 s in a Debug build on Linux). The batch table helpers
moved to `src/shell/editor_batch_cells.hpp` for both views. Verified on Linux only (Ubuntu 24.04,
GCC 13, Qt 6.4.2, offscreen): full desktop build, every ctest and `check`. Windows rests on
`native.yml`. The owner's look is [OWNER_SITTING](plans/OWNER_SITTING.md) C7. The D130 and D131
entries moved to the archive.

---

## 2026-10-10 — M5.8a scenario comparison, CLI (D148)

`trafficsim-cli --project BASE --compare ALT --seeds LIST [--csv FILE]` answers PROBLEM §1 step 6.
It runs both projects as batches over the same seeds and reports the alternative minus the base
per movement, section, approach and the network. Each row has a Welch 95 % interval: the
Welch–Satterthwaite df, then `tQuantile975(⌊ν⌋)`, the batch's own conservative table rule.

Why Welch, not paired: one random stream per run diverges as soon as the projects differ, so
equal seeds are not common random numbers, and the output says so.

What the command refuses or reports:
- **Matching.** Rows are matched by name; base-only, alternative-only and duplicated names are
  listed in JSON and on a `# Unmatched` CSV line, never dropped.
- **Refusals.** Evaluation periods that differ in warm-up, end or cool-down are refused before
  any run; different seed lists before any output.
- **No LOS.** A difference never gets an LOS letter.

Contract and rows CMP1–CMP7 are in [BATCH](reference/BATCH.md) §7, with
`tests/compare_tests.cpp` (group `compare`) and `cli-compare`/`cli-compare-conflict` in ctest. The
CLI's batch path was factored into `prepare`/`runAll` for both commands. `--seeds` output is
byte-identical before and after (`cmp`, seeds 42–44 on four-leg and 42–43 on M2.6 and the
T-junction; only `buildCommit` differs). The editor is M5.8b (NEXT).

An independent review of the M6.0 sheet also ran this session, and its 25 findings were fixed in
a separate commit. The run-cost entry of 2026-10-08 moved whole to `archive/`.

---

## 2026-10-10 — M6.0 benchmark option sheet (Q4)

Docs only, the D143 queue's item after M4.2. The [option sheet](evidence/m6-benchmark-options.md)
prepares Q4, open since 2026-09-10, for the owner to answer by letter
([OWNER_SITTING](plans/OWNER_SITTING.md) B9). It covers three kinds: S1 saturation flow and
capacity, S2 delay, and U minor-movement capacity with the gap-acceptance calibration R7 moved
to M6. Each kind has five published options, each giving its quantity, conditions and limits,
and the command that would produce TrafficSim's comparable figure or what is missing for it. No
TrafficSim figure was produced for it, and no tolerance is proposed, so the choice stays blind.

What the sheet found for the next M6 session:
- **Unsignalised capacity has no command yet.** The engine's rule is a gap time plus a distance
  headway, occupancy and area reservation, so a published critical headway must be translated,
  not copied into `gapTime`. A comparison also needs a saturated-minor project per major-flow
  level, and a follow-up headway measured at a priority line, which `--discharge` cannot give.
- **Effective green is not measured.** Amber ends the discharge window, so capacity needs a
  declared green or a saturated count.
- **Poisson generation** makes a random-arrival delay formula (Webster) the closer match, not an
  exact one: insertion and car-following reshape the stream before the stop line. HCM's uniform
  term alone describes arrivals the engine cannot generate.
- **No verifiable Thai study.** No published Thai study of these quantities could be verified,
  so a local benchmark would be the owner's own measurement.

**How the citations were checked.** Agents researched them with web search, an adversarial pass
checked them, and a third pass re-checked only the printed details. The HCM and HBS are paid and
were not read: their values come from agency and software documentation and are marked to be
read in the manual. Unconfirmed exhibit numbers are left out.

PR #135 (M5.9, M4.2) merged, with native CI green on Linux and Windows. NEXT now points at M5.8.
The D129 entry moved whole to `archive/`.

---

## 2026-10-09 — M4.2 amber stop-or-go (D147)

An optional behaviour `amberDeceleration` (catalog 3.0 m/s², ITE) turns amber into a continuous
check: go when `speed² > 2·a·gap`, else stop. With it, a vehicle that cannot stop at its type's
maximum deceleration goes on amber or red, as at a priority rule. That second rule was the owner's
choice after the first measurement left 44 physically impossible stops at red onset. One comparison
in the core head loop, no vehicle state. M0 scenarios (no `schemaVersion`) resolve the catalog
without the field through a never-written, never-compared provenance flag. So the frozen TS
fixtures, `trafficsim-cli 42` and the editor's `crossing.json` run did not move; a save and reopen
round-trips exactly. An owned behaviour with the field is schema 28. Capturing the catalog now owns
it, which exposed a latent defect: owned catalogs without a library were written without `model`
at schema 22+ and could not be reopened. The writer now tags every owned behaviour from schema 21.

Contract and rows AM1–AM8 in [AMBER](reference/AMBER.md); `tests/amber_tests.cpp` (group `amber`)
plus a regression for the untagged catalogs. Four-leg over seeds 42–81: clamps 220 → 1, every
movement's delay down 1.4–7.5 s; M2.6: clamps 844 → 35 (not classified), delay −1.1 to −12.3 s
([evidence](evidence/m4.2-amber.md)). Changed tests, each for a stated reason:
- The four-leg clamp diagnosis is pinned on the run with the field removed (D36), and a new row
  pins zero clamps with it.
- The archived M3.2.7 sweep inputs drop the field, noted in that evidence as D36-era.
- `results-tabs-ui` uses a copy owning the D36 behaviour so its clamp page still has clamps.
- Fixtures that test schemas 18–25 strip the field.
- Future-schema probes move to 29.

Linux only: desktop preset (Qt 6.4.2 offscreen, GCC 13.3), ctest 113/113 and `check` green.
Windows is the PR's native CI. The BA14/BA17 and D128 entries moved whole to `archive/`.

---

## 2026-10-09 — M5.9 evaluation cool-down (D146)

`definition.evaluation.cooldown` (schema 27) runs the simulation that many seconds past `duration`
with no new demand (inputs still end by it; `compileDocument` extends the compiled duration after
every expansion, so core is untouched). With a cool-down the window selects trips by release
(`scheduledTime` in `[warmup, end]`, `end` defaulting to `duration`) whenever they finish, in
movement and section rows, and only window vehicles left at the end are unfinished; without one
every D132 rule stands. New projects get 900 s; the run-settings dialog edits it in the same
History step as the warm-up; JSON, CSV and both Results notes name it. Contract and rows CD1–CD8 in
[BATCH](reference/BATCH.md) §5, written first; `tests/cooldown_tests.cpp` (group `cooldown`).
On `four-leg-signalised` seeds 42–51 the warning goes from 11 of 12 movements to none at 120 s and
every window trip finishes at 300 s ([evidence](evidence/m5.9-cooldown.md)); the example itself
stays schema 17 (owner's choice, sitting B8). Nine CLI outputs of the committed examples are
byte-identical before and after. Future-schema tests now probe 28. Linux only: desktop preset
(Qt 6.4 offscreen, GCC 13.3), ctest and `check` green; Windows is the PR's native CI. The D126/D127
entries moved whole to `archive/`.

---

## 2026-10-09 — Three modelling levels designed and booked (D145)

The owner asked for more capability chosen by what studies need — parking lots, dynamic
assignment, then Micro, Meso and Macro levels, Macro as a full four-step model, after the D143
queue. Docs only, no code: [design](plans/MULTI_LEVEL_MODELLING.md) (study → output matrix, one
authoring network compiled to three views, shared Zone/OD demand, parking lots as zones with
capacity, between-run dynamic assignment, pure `src/meso/` and `src/macro/` engines, subarea
cut), ROADMAP M8–M12, PROBLEM §1–§6 amended (the assignment non-goal replaced), ARCHITECTURE
planned rows, OWNER_SITTING B7 for the design's open parameters. Parking and regional modelling
left *Later*. Reasoning in [D145](decisions/RECORD.md#d145). The D124/D125 entries moved whole
to `archive/`. Checks: `trafficsim-check-docs` and `trafficsim-check-file-sizes` built directly
with GCC 13.3 (the headless preset lacks nlohmann/json on this machine); no engine build claimed.

---

## 2026-10-09 — Branch audit; Results discharge and clamp pages recovered (D144)

Every origin branch ahead of `main` was checked. `main` was re-rooted on 2026-09-28 (four root
snapshots), so the 38 older branches share no history with it and git's "ahead" counts mean
nothing; by content, every decision they name is in `main`, and the only files they hold that
`main` lacks are the retired M0 window (D24) and an unreferenced `data/fonts/NotoSansThai.ttf`.
PR #128 (`claude/dazzling-gates-df64jc`, closed unmerged) held the one piece of lost work:
Results inner pages for queue discharge and safety clamps (`9b81b17`, `cbfff89`). Its W74 and
M5 commits were already replayed or superseded. The two commits were cherry-picked onto the D142
branch, merged by hand with M5.6 (the batch view sits in the Movements page; `startBatch` selects
it), renumbered D135 → D144 (and a stray D134 → D139 in DRIVING_BEHAVIOUR), and `section-ui`
now finds the Results page as the other suites do. `check`: 111/111 (Linux, Qt 6.4.2 offscreen).

---

## 2026-10-09 — Owner's answers to the roadmap review (D143)

The owner accepted every recommendation, R1–R8. Documents only, no code: D143; ROADMAP closes M2
(gate D53, sub-milestones carried), relabels M1.26, narrows M1.22 to what `src/` lacks (checked:
no spline/arc, extend/merge, layer locks or per-Link driving side; reversal refuses referenced
Links), defines M1.23's CSV, moves M5.1's transit/crosswalks and scenario management beyond M5.8
to *Later*, and adds M3.4 (the M3.2.8c remainder), M4.2 (amber) and M6.0 (benchmark sheet, which
now owns calibrated gap acceptance). M5_PLAN gains M5.8 and M5.9 and the order after M5.7.
PROBLEM gains §8 (motorcycles). `plans/OWNER_SITTING.md` is now the single list of owner items
(gates, chat decisions, 22 desktop looks, each dated with a yes/no failure question); NEXT keeps
only the session queue and points there. `check`: docs and file sizes clean.

---

## 2026-10-09 — Volume from turning counts (D142, schema 26)

The owner answered ROADMAP O10's "count sheet typed once vs D46": an input may take its volume
from its entry decision's turning counts. `VehicleInput.volumeFromCounts` is opt-in, so D46 and
every existing file are untouched. `src/project/counted_volumes.cpp` holds the one rule:
`countedDecision` (the decision named, or the placed one on the entry Link), `countedVolumes`
(summed base-route counts × 3600 / length per decision interval) and `syncCountedVolumes`, called
on read, in `putInput`/`putRoutingDecision`, in `History::execute` and at the top of
`withRoutingDecisions`, so no path sees a stale volume. The input's intervals are not written for
such an input. Issues `INPUT_COUNTS_NO_DECISION`, `INPUT_COUNTS_EMPTY` and
`INPUT_COUNTS_POSITIONED` (a D119 decision is a downstream split, not a source). The input dialog
gains the checkbox, which locks the typed volume, counts and Periods and shows the counted total;
the input and decision rows say which feeds which. `field<bool>` now refuses a non-boolean.

Evidence (Linux, GCC, Qt 6.4.2 offscreen, Debug): `demandcounts` 8/8 (VC1–VC7, each movement's
vehicles equal its counts for unplaced and placed decisions), `counted-volume-ui` (VC8), and the
D46 test unchanged. CLI `42 --project` output for the four shipped projects equals `origin/main`'s
(`c7628e8`, built in a scratch worktree) byte for byte. Native Windows CI and the owner look are owed.

---

## 2026-10-09 — M5.6: the editor's Run seeds, Copy and Export (D141)

The editor now runs the CLI's batch. **Run seeds** parses the *Seeds* field with `parseSeedList`
(default `42-51`, O9), compiles the document on the UI thread and hands a `Scenario` and an
`EvaluationSpec` by value to one `std::thread` that calls `runSeeds` and `aggregate`
(`src/shell/editor_batch.cpp`). Progress and the result come back through queued
`QMetaObject::invokeMethod` calls tagged with a generation number; `cancelBatch()` (called from
`clearRun()`, so every edit, Undo, Open, Reset and a single Run reach it) bumps the number and sets
the worker's cancel flag, so whatever a stale worker posts is dropped. The destructor cancels and
joins. No mutable statics were found in `src/core`, `src/eval`, `src/model` or `src/runner`.

The batch table replaces the single-run tables: movements and sections with n, mean delay, ±95 %,
vehicles, travel time, unfinished, control type and LOS; approaches with n, mean ±95 % and max; a
note with the marker, n, seeds, evaluation period, overloaded seeds, movements with unfinished trips
and the LOS pack. Export writes `batchCsv`; Copy writes `csvToTsv(batchCsv(...))` (new in
`csv_format.hpp`: comment lines kept, unquoted commas become tabs, quotes dropped), and a finished
single run copies `movementCsv` the same way. Contract and rows EB1–EB6 in BATCH §6.

Evidence (Linux, GCC, Qt 6.4.2 offscreen, Debug): `batch-run-ui` compares the editor's
`BatchReport` with `aggregate(runSeeds(...))` on four-leg plus two signalised sections, the export
bytes with `batchCsv` and the clipboard with its TSV; cancel, an edit, a bad list, a batch after a
cancel and closing mid-batch are exercised; 13–14 s, three repeats stable. Its first version left an
autosave draft (an unsaved rename outlived the 15 s timer) that blocked the next run on the recovery
dialog; the test now saves after the edit. `check`: 108/108 tests, architecture, file sizes and
docs clean. Native Windows CI and the owner's look are separate and still owed.

---

## 2026-10-09 — Optimization pass: lld, D140, validate benchmark, NEXT

Measured first (Linux, 4 cores, Qt offscreen): the engine (0.15–0.30 µs per vehicle-tick, linear
to 96 crossings), the run view (~3.3 ms per Step) and redraw (3.6 ms at 40 crossings) are not
bottlenecks and were left alone. **Link with lld when the compiler accepts it**
(`TRAFFICSIM_LLD`, default ON, MSVC untouched): touching one source relinks some 40
executables, and GNU ld was most of an incremental Debug build. `trafficsim-tests` links in
4.5 s with ld and 0.57 s with lld; touching `src/shell/editor_demand.cpp` rebuilt in 31.3 s
before and 13.0–13.5 s after, `src/model/network/routing.cpp` 28.7 s → 10.7–11.1 s. CI's Linux
apt line installs `lld`; without it the default linker is kept silently.

**D140: Connector paths once per expansion.** `History` validates every command, and validating
the M2.6 study template took 21.6 ms against 1.0 ms for four-leg on the same network: callgrind
put 85 % of it in `connectorPaths`, recomputed by every `routeShortestChains`, `routeLaneFamily`
and `routelessChains` call inside one `expandRouteless`. `ConnectorPathTable` (lazy, local, never
stored) is now built once by `expandRouteless` and `routelessIssues` and passed down; the old
signatures build their own, so editor and compile callers are unchanged. Validation 21.6 → 5.0 ms
(median of 15; four-leg 1.03 → 0.91 ms). `--seeds 1-3` output of all four shipped projects is
identical apart from the build stamp; the ten-seed study batch and the engine benchmark are
unchanged within noise (an interleaved ld/lld engine run ruled out the linker).
`tools/validate_benchmark.cpp` (`trafficsim-validate-benchmark [project] [repetitions]`) keeps
that number measurable; like the other benchmarks it prints and is not in `check`.

**Route continuations find their tail once.** After D140, 43 % of study-template validation was
`continuations` rescanning every route object to find the tail, once per candidate. It now finds
it once (the last match, as before). Validation 4.97 → 2.75 ms (four-leg 0.85 → 0.72 ms); the
editor's route hover and gesture use the same function. CLI output identical.

**Destination chains once per expansion (D140's walk scope).** `placedDecisions` runs once per
time slice × vehicle type and resolved the same `routeShortestChains(decision Link, destination)`
each time, about 86 calls per study-template validation. A `DestinationChainMemo` beside the
table resolves each pair once. Same-session A/B: 3.32 → 2.46 ms median (five runs of 31 each);
four-leg unchanged (no destination decisions). CLI output identical.

**`appendStationRouting` takes the expansion's `runtimeSections`.** It rebuilt the table on every
call (≈5 per study-template validation, 23 % of the remaining instructions); `expandRouteless`
already holds that table and passes it to `appendLaneChanges` beside it. A/B: 2.60 → 1.72 ms
median (five runs of 31); four-leg unchanged (no positioned decisions). CLI output identical.
Validation overall this session: 21.6 → 1.7 ms. Left: `routelessChains` still walks once per
input × slice; at under 2 ms per edit it was not started.

**NEXT.md slimmed** (31.4 → 26.8 KB, −15 %): the five 2026-10-05/06 owner-instruction sections narrated
finished slices already recorded in ROADMAP (M2.8, M3.2.4, M3.3 status lines), PROGRESS and
RECORD (D115–D129, D135–D139, each checked before removal). One section keeps every open item
verbatim — the D128/D119/D116/D115–D118 Windows reviews, the Demand PR review order, W74's next
steps, BA18, the M3.2.4e contract rule and pending native CI. §1–§5 are live owner gates and
working notes and were not cut, so the planned −40 % was not reached.

---

## 2026-10-09 — W74 replayed on top of M5: D135–D139, schema 25

W74 (M3.3.3a) and M5 were built in parallel from the same `main` and both used D130–D134 and
schema 22. By owner instruction the M5 branch is the base and the W74 slices are replayed on it:
D130–D134 of the W74 branch are now D135–D139 everywhere (record, index, code comments,
evidence), and `w74` is written in schema 25 (22–24 are M5's evaluation period, sections and
control type); a file below 25 carrying `"w74"` is refused, and every "future schema" probe is 26.
The entries below keep their original dates and test counts. D139 (the dialog editing `w74`
keys, with no code defaults) had no entry of its own; its reasoning is in the record. Not carried
over from the W74 branch: its own `--seeds`, editor Run N seeds and evaluation period (M5.2–M5.3
there, superseded by D131–D132 here; the editor batch is a reference for M5.6) and its Results
tabs (discharge and clamps). The decision record's blank lines between rows are removed again.

---


## Backlog (M0, in order)

The [historical checklist](archive/PROGRESS-M0-backlog-and-questions.md#backlog-m0-in-order)
is archived. Current work lives in [NEXT](NEXT.md); milestone gates live in [ROADMAP](ROADMAP.md).

## Open questions

The [historical register](archive/PROGRESS-M0-backlog-and-questions.md#open-questions)
is archived. Use [the owner sitting, §B](plans/OWNER_SITTING.md#b--decisions-in-chat-one-line-each) for current questions.

## Decisions

The complete [decision record](decisions/RECORD.md) now lives beside its
[topic index](decisions/README.md). This heading remains as a compatibility target
for existing `PROGRESS.md#decisions` links. Add new decisions to RECORD, not here.
