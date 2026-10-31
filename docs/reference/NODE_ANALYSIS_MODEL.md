# Node analysis: Fill from model and Micro comparison (A9 second half)

**A9 second-half contract (D419–D421); Fill/provenance and model-window comparison implemented.** The owner approved J7 (`fe827c8`) on 2026-10-31
and selected the rest of [NODE_ANALYSIS §6/§12](NODE_ANALYSIS.md#6-workflow-in-the-editor).
The six choices below are [OWNER_SITTING B40](../plans/OWNER_SITTING.md#b--decisions-in-chat-one-line-each).
All six B40 choices are answered. B41 asks how manually entered counts acquire a comparison
window; that manual-count confirmation remains pending. Model-filled Nodes already carry an explicit window.
Status and the queue remain in [ROADMAP](../ROADMAP.md) and [NEXT](../NEXT.md).

## Boundaries already settled

- Preserve [J7's snapshot, CLI parity and Results contract](NODE_ANALYSIS_EDITOR.md), D418.
  Owner review confirms identical CLI/editor JSON and CSV for saved bytes and the unsaved
  Save-representation snapshot, with unchanged shipped projects/analysis except buildCommit.
- HCM control delay and Micro travel-time delay are different quantities. Keep both labels and
  the not-yet-validated marker. Reuse the method and [batch estimates](BATCH.md#2-aggregate),
  not new delay formulas, a seed or a clock in `src/analysis/`.
- Every stored edit is a Command with one Undo. Preview and comparison do not mutate the
  document. New stored fields require a feature-selected version, older reads and a refusal
  before the field's version at its own path; 64 is selected only when `analysis.modelSource` is present.
- Read actual Micro demand through its compiler/catalog resolution, with no second routing
  interpretation. Never infer an HCM heavy-vehicle classification from a type's name, length
  or PCU: the current catalog has no such classification.
- Macro/Meso result imports, timing proposals, coarse HCM and the UTDF writer are outside this
  request. U0 still has no answers. Existing other-level outputs and files keep their bytes.

## B40: answered choices

1. **Demand source.** NODE_ANALYSIS §6 says Micro routes/inputs; D379 also books volumes from
   Micro/Meso/Macro results. Recommended: expected demand from Micro's compiled routes, inputs,
   compositions and intervals, without sampling arrivals. Completed/passed counts can fall
   below demand under congestion. Alternative: Fill from a finished Micro run. Other result
   sources remain booked; do not silently choose between these meanings.
   **Owner, 2026-10-31:** “Expected demand จาก Micro routes/inputs (แนะนำ)”.
2. **Time and HCM inputs.** Recommended for expected demand: explicitly choose start/end, with
   a StudyPeriod shortcut placed using `clockStart`; integrate expected vehicles over that
   run-time interval and divide by its hours. Preview replacement of T with that duration and
   PHF with 1, avoiding a second peak adjustment. Alternative: replace volume only and retain
   T/PHF. The window, clock placement and values changed must be visible before Confirm.
   **Owner, 2026-10-31:** explicit start/end (StudyPeriod placed by clockStart), replace T with
   window duration and PHF with 1, shown before Confirm; recommended option selected.
3. **Micro comparison.** Recommended: reuse a completed, explicitly requested Run seeds batch
   (for example 42–51); compare only Node and approach rows from the same snapshot and
   evaluation window. Show HCM control delay beside Micro mean travel-time delay, seed n,
   confidence interval and unfinished vehicles. No subtraction, claim of equivalence or
   estimated turn/lane-group matching. Alternative: a separate ten-seed analysis action.
   **Owner, 2026-10-31:** reuse a completed, explicitly requested Run seeds batch; Node/approach
   rows only, n/CI/unfinished, no subtraction/equivalence/turn matching; recommended option selected.
4. **Heavy vehicles.** Recommended: an explicit user selection of HCM-heavy vehicle types,
   expected-count-weighted %HV per turn, recording that selection in provenance. Do not guess
   the selection. A zero-volume turn retains its existing optional %HV. Alternative: Fill
   volume only and retain every turn's existing %HV.
   **Owner, 2026-10-31:** “เลือก HV types ชัดเจน แล้วคำนวณ %HV (แนะนำ)”.
5. **Replacement and provenance.** Recommended: preview before/after, then replace volumes
   for every currently drawn turn, including explicit zero for no demand; keep adjustments
   and saturation. Save the source snapshot/hash, window and selected options through a
   feature-selected schema-64 field. Later manual volume/%HV/T/PHF edits clear that Fill's
   provenance rather than claiming the whole set still came from the model. Undo restores
   it; Fill can be repeated. Alternative: fill only missing volumes with per-turn provenance.
   **Owner, 2026-10-31:** “แทนทั้งชุด พร้อม provenance; แก้ค่าหลักเองแล้วล้างที่มา (แนะนำ)”.
6. **Demand that needs runtime choices.** Positioned decisions use the actual passage time;
   parking can release vehicles during the run. Recommended: support demand whose complete
   Node traversal is determined by compiled paths/shares, and refuse an affected unresolved
   runtime/parking contribution with a named, actionable source diagnostic. Do not invent an
   arrival time, drop that demand or publish a partial Fill. Alternative: require a simulated
   source for these cases. Revisit this boundary if choice 1 selects finished results.
   **Owner, 2026-10-31:** “รองรับ demand ที่แน่นอน; ปฏิเสธกรณีที่ยังต้องจำลอง (แนะนำ)”.

## Expected-demand preview

`previewNodeModelDemand` records/parses the same captured project bytes and calls the existing
Micro compiler with a manifest. Its mandatory start/end request is in run seconds within the
demand horizon, excluding cool-down. Runtime sections and Connector ownership map complete
traversals to derived turns; station coverage uses the existing reference-to-lane mapping.
An entry-lane stub follows every compiled lane-change target, recursively through further
stubs; all complete Node traversals must agree. Targets take precedence over a broader origin
family. Only a stub without targets uses the compiler's exact-origin fallback. Cycles are
visited once, and staged resolution is independent of route/edge order (D420). Expected demand
is not the throughput achieved after a lane change.
Each traversal counts once, not once for every lane/possible path. Inputs/composition shares,
legacy scheduled routing intervals and OD/StudyPeriod placement are the compiler's values.

The preview returns every current turn, including zero, an explicit count-weighted %HV for
positive demand, the source revision and exact project/catalog manifest. Zero retains the
old optional %HV; explicitly selecting no HV types gives 0 % on positive flows. The preview
only returns values; the staged form and Command below publish them.

Named refusals identify invalid windows/HV choices, omitted OD demand, out-of-area Nodes,
partial or ambiguous traversals, cycles and unresolved runtime contributions. A positioned
choice that changes no Node traversal is harmless; otherwise do not guess passage-time
weights. Potential parking release follows initial occupancy, gate demand and lot-to-lot
connections; a provably empty lot without gate demand does not block unrelated demand.
The preview refuses an affected release instead of counting zero-rate exit placeholders.

## B41: manual-count comparison window (pending)

A manually entered Node, or one whose Fill provenance was cleared by a demand edit, has no
start/end even when T matches a batch. Recommended: explicit session-only confirmation of
that count window before comparison, matching batch evaluation start/end and Node T, labelled
manual counts. Alternative: compare only Nodes retaining a Fill window. No answer is assumed.

## Staged Fill and provenance (D421)

Nodes → Analysis… → **Fill from model…** has explicit start/end in run seconds and a StudyPeriod
shortcut placed by subtracting `clockStart`. There is no chosen period or HV type by default;
reopening a sourced Fill offers its previous explicit selections. Preview captures exact saved
bytes when they still parse to the staged document, otherwise its Save representation. The
before/after table shows every current turn, explicit zero and old stale rows being removed,
plus T → window hours and PHF → 1. Named localized refusals disable Confirm; changed window or
HV choices invalidate the preview. No run is launched or sampled.

Child Confirm stages values in Analysis; only parent Confirm calls `setNodeAnalysis` through
History, in one Undo. Cancel at either level is unchanged. Site adjustments and sourced
saturation stay. Drawn turn/approach clear actions both keep the row visible for re-entry;
explicitly cleared stale rows disappear. Unchanged input creates no History entry.

`analysis.modelSource` is a historical claim, not a live demand binding. Schema 64 stores kind
`micro-demand`, captured revision/project SHA-256, start/end, explicit HV ids, manifest file
logical paths/hashes and optional fallbacks. Closed codec rules reject unknown/missing/bad
values and refuse the field before 64 at its own path (including null). Earlier files keep
their feature-selected schema/bytes. Stored results or copied geometry/timing are not added.
Demand-row, %HV, T or PHF edits with the same old claim clear it in the Command/History boundary;
site-only edits retain it. Undo restores the whole claim. Re-Fill records a new snapshot.
The Analysis source line, results settings, JSON and CSV expose retained provenance; outputs
without it keep their old bytes. Existing manual count entry remains available.

## Micro side by side (D420/D421)

Run seeds explicitly, wait for completion, then Analyse and open Results → Node analysis →
**Micro side by side**. The page reuses the completed batch; it launches no job. Captured Node
ids and ordered in-Link ids map batch positions, including duplicate Node names. Its source
must equal Analyse's decoded document, and the confirmed count window and T must match batch
evaluation start/end. Model-filled Nodes use their retained window. Without one,
`NODE_MICRO_NO_WINDOW` names the Node while B41 awaits an answer; no window is inferred from T.

Rows show Node/approach HCM control delay and LOS beside Micro simulated section/travel-time
delay, seed n, 95% CI half-width, vehicles and unfinished. Missing estimates stay blank.
No turn matching, subtraction, new delay formula or equivalence claim is made. Distinct measure
labels and the not-yet-validated marker stay visible. Source revision/hash, evaluation window
and seed list are shown. Mismatched/unfinished sources produce localized named refusals;
`NODE_ANALYSIS_PARTIAL` remains visible under D383, without changing B29(d).

Editing, Undo, seed replacement, cancellation and New/reopen discard the existing batch under
its normal lifecycle. Undo restores inputs/provenance but does not resurrect a discarded run.
Opening another document also clears the analysis capture, including revision-zero files.
The active Micro subtab exports its shared comparison CSV, with C-locale numbers, quoted cells,
marker and generated/stale marking; no comparable rows means Export/Copy is disabled. Analytical
subtabs retain their CLI-identical CSV and the report JSON remains analytical in every subtab.

## Acceptance coverage

NA24–NA30 are **passing for the answered B40 scope**, on both driving sides; UI cases cover
English and Thai. B41's manual-window confirmation remains unimplemented and A9 awaits review.
Every new feature row failed before implementation; forcing setup is asserted first.

| Row | Passing coverage |
|---|---|
| NA24 | Hand-counted complete routes, lane paths/compositions, intervals, one count per traversal, explicit zero; deterministic compiled lane-change stubs and ambiguous control |
| NA25 | Explicit window, StudyPeriod/clock placement, HV weighting/zero policy, T/PHF replacement; invalid/unresolved sources refuse without mutation or dropped demand |
| NA26 | Provenance schema-64 round-trip and early-field paths, unchanged older files, exact raw source SHA, demand-edit clear/site-only retain, Undo and reopen |
| NA27 | Both sides/locales: before/after, Cancel/no-op, one Undo/Redo, Save/reopen, retained adjustments/saturation, consistent drawn-row clearing |
| NA28 | Real completed batches: exact Node/approach HCM and Micro values including n/CI/unfinished/blanks, duplicate-name id mapping, distinct measure labels |
| NA29 | Snapshot/window mismatch, unconfirmed window, stale edit/Undo, reopen, cancellation/incomplete/no batch; no stale comparison or implicit run |
| NA30 | Shared-format marker/CSV, real CLI JSON/CSV parity with sourced inputs, unchanged source-absent fixture/project/level outputs |

Native tests: `node_model_demand_tests.cpp`, `node_model_demand_edge_tests.cpp`,
`node_model_stub_tests.cpp`, `node_analysis_source_tests.cpp`, `node_analysis_comparison_tests.cpp`.
Qt: `node_analysis_fill_ui_tests.cpp` and `node_analysis_micro_ui_tests.cpp`, retaining JT12's
`node_analysis_ui_tests.cpp` real-CLI coverage. Six D420 cases include shipped m2.6 plus Suggest
nodes, the 1200 veh/h pocket's 720/240/240 hand count, and differing complete exits, both sides.
Full verification is in PROGRESS; Windows/high-DPI/manual appearance remains OWNER_SITTING E32.

## Code entry points

`project/compileDocument`, `demand_preview`, `demand_paths` and `evaluationSpec` own compilation
and clock rules. `node_model_demand` owns preview, `node_analysis_fill` its replacement,
`node_analysis_source_codec` the stored claim, and `node_analysis_comparison` report matching/CSV.
`runner/batch` and `project/parallel_seeds` own existing runs/estimates. The free
`shell/node_analysis_fill_dialog`, Analysis form, self-owned `node_analysis_panel` and
`editor_batch` stage/present them; `commands/node_analysis_commands` owns edits. EditorWindow's
header remains at 500 lines. No generic job subsystem or new per-turn simulation statistic.
