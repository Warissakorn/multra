# Node analysis: Fill from model and Micro comparison (A9 second half)

**A9 second-half contract (D419/D420); native preview implemented, UI in progress.** The owner approved J7 (`fe827c8`) on 2026-10-31
and selected the rest of [NODE_ANALYSIS §6/§12](NODE_ANALYSIS.md#6-workflow-in-the-editor).
The six choices below are [OWNER_SITTING B40](../plans/OWNER_SITTING.md#b--decisions-in-chat-one-line-each).
All six B40 choices are answered. B41 asks how manually entered counts acquire a comparison
window; implement independent Fill/provenance work while that comparison-only choice is pending.
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
  before the field's version at its own path; the next available step is 64.
- Read actual Micro demand through its compiler/catalog resolution, with no second routing
  interpretation. Never infer an HCM heavy-vehicle classification from a type's name, length
  or PCU: the current catalog has no such classification.
- Macro/Meso result imports, timing proposals, coarse HCM and the UTDF writer are outside this
  request. U0 still has no answers. Existing other-level outputs and files keep their bytes.

## B40: choices awaiting the owner

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

## Native preview implemented at this checkpoint

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
old optional %HV; explicitly selecting no HV types gives 0 % on positive flows. No stored
field, schema step, mutation, dialog, T/PHF adjustment or comparison is implemented yet.

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

## Draft acceptance coverage

NA24's native coverage and the independent native guards of NA25 are implemented; the remaining
rows are **planned**, not passing. Finalize unanswered choices and write failing tests before
their implementation, asserting forcing setup first. Both driving sides are covered natively;
English/Thai UI cases still await implementation and are not claimed by this checkpoint.

| Row | Coverage to settle against B40 |
|---|---|
| NA24 | Hand-counted complete routes through a Node, multiple lane paths/compositions, time-varying input shares, and no double counting of one traversal; volume zero differs from missing |
| NA25 | Explicit window, StudyPeriod/clock placement, %HV policy and T/PHF conversion; invalid/unresolved sources refuse without any mutation or silently omitted demand |
| NA26 | Provenance round-trip/version floor, unchanged older files, source identity and the chosen manual-edit/re-Fill rule; existing D383 stale rows remain explicit |
| NA27 | Both sides/locales: staged before/after, Cancel/no-op, exact one Undo/Redo, Save/reopen and retained adjustments/saturation; drawn turn/approach clear controls behave consistently |
| NA28 | A real completed batch: Node/approach side-by-side values equal existing HCM and batch reports, including n/CI/unfinished/missing values and distinct measure labels; no approximate movement matching |
| NA29 | Snapshot/window mismatch, stale edits, Undo, New/reopen, cancelled/incomplete batch and no result cannot masquerade as a fresh comparison; source and seed list remain visible |
| NA30 | CSV uses shared formatters/marker; analytical CLI parity, shipped projects and Micro/Meso/Macro output bytes remain unchanged when the new feature is absent |

The native cases in `tests/node_model_demand_tests.cpp` and
`tests/node_model_demand_edge_tests.cpp` cover direct interval/lane/stub counts, clock-placed
OD/compositions, scheduled decision shares, invalid requests/types, deferred routing, partial
routes and parking (including the empty-lot control), on both driving sides. D420 adds six
rows in `tests/node_model_stub_tests.cpp`: the shipped m2.6 study with a suggested Node, a
hand-counted m2.6-style pocket, and a positive-demand stub with different complete Node exits,
on both sides. UI/provenance,
schema-64 version tests and completed-batch comparison remain unfinished.

## Code entry points and next dialog touch

`project/compileDocument`, `demand_preview`, `demand_paths` and `evaluationSpec` own compiled
demand and evaluation windows. `runner/batch` and `project/parallel_seeds` already own Micro
runs/estimates. `shell/node_analysis_dialog`, `node_analysis_panel`, `editor_nodes` and
`editor_batch` are the existing UI surfaces; `commands/node_analysis_commands` owns edits.
Keep `EditorWindow` at its existing 500-line header limit by using free dialogs/self-owned pages.

J7's minor E31 finding is booked for the next touch of the dialog: removing a drawn approach
currently hides the row until reopen, while removing a drawn turn leaves cleared fields.
Make drawn rows consistent cheaply, preserving explicit stale-row removal and one Undo; this
does not widen the approved J7 slice.
