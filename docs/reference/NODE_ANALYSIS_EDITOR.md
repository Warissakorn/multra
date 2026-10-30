# Node analysis editor (M14 J7, A9 first half)

Contract: [JUNCTION_TEMPLATES §9/JT12](JUNCTION_TEMPLATES.md#9-posted-speed-and-the-analysis-panel),
[NODE_ANALYSIS](NODE_ANALYSIS.md) and D383. B39's snapshot choice is answered in D418.
This adds authoring/results UI over the existing method, without a formula, field or schema step.

## Inputs and Commands

Nodes → **Analysis…** stages the selected Node's existing `analysis` fields, with a scrolling
body and fixed footer. Turn rows use §6's compass + geometric L/T/R names and retain in/out
Link ids. Hourly volume and optional %HV are per turn; PHF and analysis period are per Node,
as the existing stored model specifies. Blank volume means no row; entered zero is a real row.
Blank optional fields retain the method's existing defaults without storing those defaults.
Approach rows edit grade, parking manoeuvres and stopping buses. The Link's posted speed is
read-only here. Node settings include CBD and a sourced base-saturation override.

Cancel changes nothing. Confirm calls `setNodeAnalysis` once through History, with one Undo;
unchanged/default-only input creates no edit. Actual input edits keep setNodeAnalysis’s existing
manual-Node ownership rule (clear generated); Undo restores it. A saturation override requires
both value and source text in the form; older empty-source files remain readable. Stored shape/sign validation is the existing codec;
method-range refusals stay with Analyse. Lane groups/geometry/timing remain derived. Stale turn
and approach rows are kept and flagged with D383's codes; only explicit removal clears them.
No *Fill from model*, counted intervals, timing proposal or Micro side-by-side is included.

## Snapshot and byte parity (B39/D418)

**Analyse** runs every Node carrying analysis inputs, as `--node-analysis` does, synchronously
on the UI thread. If the saved file still parses to the current document, capture its exact
bytes. Otherwise capture the current document's Save bytes and label the result as an unsaved
snapshot. Record and parse those same bytes for the manifest and calculation. Analyse writes
no project and creates no History entry. Saving that unchanged snapshot then running the CLI
must give identical JSON/CSV, including manifest, marking and build metadata. Preserve the CLI's
existing outputs and platform text-stream line endings; locale affects UI strings only.

Shared project report functions own analysis, metadata and export marking; existing HCM,
Micro/Meso/Macro and bare `nodeAnalysisJson`/`nodeAnalysisCsv` bytes stay unchanged. The UI owns
only presentation and a captured result. Results are session-only, revision-stale after edits;
Undo back to the source revision restores freshness. Opening/new document clears the result
even if its revision happens to match. Stale exports carry the existing `project` marking.

## Results

Results → **Node analysis** shows the analytical/not-simulated, not-yet-validated marker,
source revision/snapshot state, defaults used and sourced saturation flow. Separate read-only
tables show lane groups (v, s, c, X, g/C, d, LOS, mean/95th back of queue in veh/ln), approaches
and intersections (flow, d, LOS; critical X where available), and all localized refusals with
their paths/codes. Missing/refused numbers stay blank. D383's existing partial totals remain
visible with `NODE_ANALYSIS_PARTIAL`; B29(d) stays an owner choice, not a method change here.
CSV Export/Copy use the shared formatter and marker; figures are never re-computed from cells.

## JT12 acceptance

- Both driving sides and English/Thai: per-turn compass names, missing versus zero, settings
  and approach edits, invalid stored input, stale-row retention/removal, Cancel, exact one-Undo
  and reopen. No change to lane/signal geometry or other levels' stored inputs.
- Real Analyse: every configured Node, all requested figures, defaults/refusals (including
  D383), blank unavailable queues, no History mutation, revision-stale/Undo/reset behaviour.
- Fresh saved and unsaved-then-saved snapshots match actual CLI JSON and exported CSV bytes;
  include generated marking, decimal/quoted names and source-file changes. Old CLI fixture
  bytes remain identical except buildCommit. Windows/manual look is OWNER_SITTING E31.

JT12 is **passing** in `tests/node_analysis_ui_tests.cpp`: 8 form/result cases across both
driving sides and both locales, including real CLI processes and raw export byte comparisons.
Final desktop build/CTest/check evidence is recorded in PROGRESS; Windows appearance remains E31.
