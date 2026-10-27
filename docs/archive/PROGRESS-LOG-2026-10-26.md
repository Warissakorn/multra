# PROGRESS archive — 2026-10-26

Whole session entries moved from [PROGRESS](../PROGRESS.md).

## 2026-10-26 — Queue-counter and travel-time-section structural schemas (D362)

- **Changed:** shared metadata covers queue counters/measurement lines and travel-time sections/
  section lines/controlType. Native readers move to measurement_codec at the original parser
  stages; control points/paths share the right-of-way metadata. The composed partial schema/CLI
  uses 62 shared definitions and 14 version profiles; the parser is now 290 lines. The old
  incidental definition-count bound is replaced by an actual shared-ControlPoint reference check.
- **Preserved:** schema 16/23/24 gates, nullable network arrays and optional names/sources/types,
  empty-source decoding of non-object measurement lines, reference-before-point and section-
  lines-before-controlType diagnostics. Exactly-one-source, owner/reference and section bounds/
  order checks remain model validation; measurement/LOS algorithms and writer rules do not change.
  The envelope-only API, observed desktop response and existing serialized output are unchanged.
- **Verified:** 5,928 cases across schemas 1–57 retain byte-identical native outcomes/errors/
  canonical documents and pass independent Draft-2020-12 expectations. Four shipped projects
  validate structurally and keep byte-identical canonical output. Five new project tests cover
  gates/nulls, shared points, empty/ambiguous sources, diagnostics, save versions and semantic
  exclusions. Release headless build and all 161 CTest entries pass (16.64 s), including CLI,
  architecture, documentation and file-size checks. Linux headless evidence only; no Qt/Windows,
  scientific-validation, LOS-validation or owner gate is closed.

## 2026-10-26 — Right-of-way structural schema and preserved legacy reads (D361)

- **Changed:** shared metadata now covers waiting lines, control points/paths, conflict areas/
  sides, priority rules and Stop/Yield controls. Their native reads move to right_of_way_codec;
  queue counters reuse the same path decoder. The composed partial schema/CLI uses 56 shared
  definitions and 13 version profiles. The parser is now 319 lines; no geometry/engine changes.
- **Preserved:** schema 14/15 gates, optional nullable collections/names, legacy non-object
  right-of-way containers and path decoding, authored enums and generated:true-only from 54.
  Waiting-line point-before-id diagnostics and generated checks after each row keep their order.
  Semantic owner/path/line/area checks, bounds, positive gaps, priority cycles and run-readiness
  remain existing validators. The envelope-only API and desktop observed response are unchanged.
- **Verified:** 12,967 cases across schemas 1–57 retain byte-identical before/after native
  outcomes, errors and canonical documents and pass independent Draft-2020-12 expectations.
  All four shipped projects validate structurally and keep byte-identical canonical output.
  Five new project tests cover gates/nulls/enums, legacy paths, diagnostics/generated ordering,
  semantic exclusions and structural refusals; project tests pass 50/50. Release headless build
  and all 161 CTest entries pass (13.10 s), including docs, architecture, CLI and file-size checks.
  Evidence is Linux headless only; no Qt/Windows, Vissim, scientific or owner gate is closed.

## 2026-10-26 — Signal control structural schema and codec parity (D360)

- **Changed:** shared field metadata now covers definition signal-array presence, fixed-time
  controllers/groups and legacy programs/phases. Their readers move to signal_control_codec;
  the composed partial schema/CLI includes them using 41 definitions and 12 version profiles.
  The envelope-only API, writer version selection and desktop observed discovery stay unchanged.
- **Preserved:** controller/group keys stay strict at every version, program/phase keys stay
  additive, nullable names and integer-valued float groups retain existing conversion. The
  pre-54 generated guard, generated-value error order and pre-13 project migration keep their
  original stages; bare M0 programs remain legacy. Timing, uniqueness, reference resolution and
  run-readiness remain existing semantic checks. No engine or runtime signal algorithm changes.
- **Verified:** 6,669 cases across schemas 1–57 have byte-identical before/after native results,
  diagnostics and canonical documents, and pass independent Draft-2020-12 expectations. Four
  shipped projects pass structural validation and retain byte-identical canonical output.
  Five new project tests cover presence/strictness, numeric/null/error ordering, version guards,
  migration and semantic exclusions; project coverage is 45 tests. Release headless build and
  all 161 CTest entries pass (15.15 s), including CLI, docs, architecture and file-size checks.
  Evidence is Linux headless only; no Qt/Windows, scientific-validation or owner gate is closed.

## 2026-10-26 — SignalHead structural schema and conditional groups (D359)

- **Changed:** shared metadata supplies SignalHead known-key/type rules, native read conditions
  and schema if/then constraints. SignalHead decoding moves out of the parser (now 409 lines).
  The composed schema/CLI adds head rows; the envelope-only API and desktop observed discovery
  stay unchanged. Controller/program contents and semantic/model checks remain codec-owned.
- **Preserved:** absent/null controllerId ignores groupNumber; even an empty non-null controller
  requires a group. Integer-valued floats remain accepted in native int range, without inventing
  a schema-13 read gate. Optional text, pre-7 known-key policy, generated flags, diagnostic order,
  attachment conversion and pre-13 signal-program migration keep their existing behavior.
- **Verified:** 4,503 SignalHead cases have byte-identical before/after outcomes/errors/canonical
  documents and pass independent Draft-2020-12 expectations. Four shipped projects validate
  structurally and keep byte-identical canonical output. Five new regression cases cover defaults,
  conditional/ignored groups, numeric boundaries, version compatibility and legacy program
  migration; project tests pass 40/40. Release headless build and all 161 CTest entries pass,
  including architecture, docs, CLI and file-size checks. Linux headless evidence only; no
  engine, scientific-validation, owner or Qt/Windows gate is closed.

## 2026-10-26 — Connector and attachment-reference structural schema (D358)

- **Changed:** after reading CONNECTOR_PARITY_AUDIT, extended shared metadata and composed
  schema/CLI coverage to implemented Connector/from/to fields. Connector decoding has its own
  file; Link/Connector geometry-point decoding is shared. Native signal heads reuse the same
  LaneReference reader, with whole-network fraction conversion kept at its original stage.
  The parser is now 420 lines. No geometry algorithms or engine behavior change.
- **Preserved:** wrong fraction/station keys refuse even null/legacy inputs; optional name/side
  nulls, count/display defaults, pre-7 additive keys, side from 17, behaviour from 21 and
  generated:true-only from 54 keep their existing semantics and diagnostic paths. Physical
  lane ranges, attachment bounds, references and other signal/control/demand contents still
  require the codec; composed coverage remains explicitly partial.
- **Verified:** 4,275 before/after Connector cases have byte-identical native outcomes/errors/
  canonical documents and pass independent Draft-2020-12 expectations. All 3,078 prior Network
  cases retain native outcomes and pass the expanded schema. Four shipped projects validate
  structurally and keep byte-identical canonical output. Five new regression cases include
  migration/shared signal references, defaults, gates, diagnostics and native integer/array
  errors; project tests pass 35/35. Release headless build and all 161 CTest entries pass.
  Evidence is Linux headless only; no Qt/Windows/Vissim acceptance or milestone claim.

## 2026-10-26 — Versioned Network and Link/Lane structural schema (D357)

- **Changed:** shared field metadata drives Network/Link/Lane/Link-point known-key and type
  rules plus formal schema generation. Link reads move out of the full parser, now 476 lines,
  with the same defaults, ordering and diagnostics. Document-stage linkType/roadId checks stay
  in place. `projectDocumentSchema` composes the unchanged envelope-only API with network rules;
  the CLI emits it using 18 shared definitions and 9 version profiles for schemas 1–57.
- **Limits:** partial coverage stays explicit. Connector/control/demand/optional feature contents,
  geometric/model validity and reference resolution still require the codec. Desktop observed
  discovery and runtime geometry are unchanged; no milestone or owner gate is closed.
- **Verified:** 3,078 adversarial cases produce byte-identical native outcomes/errors/canonical
  documents before/after and pass independent Draft-2020-12 schema expectations. All four shipped
  projects validate structurally and keep byte-identical canonical output. Six new regression
  cases cover versioned definitions, defaults/nulls, schema-7 key paths, reference-stage gates,
  native integer tokens and semantic exclusions; project tests pass 30/30. Release headless
  build and all 161 CTest entries pass, including CLI, architecture, docs and file-size checks.
  Evidence is Linux headless only; no Qt/Windows acceptance claim.

## 2026-10-26 — Shared envelope/background metadata and formal schema (D356)

- **Changed:** envelope/background field metadata now supplies the existing native read and
  typed-validation stages plus a partial draft-2020-12 schema. `multra-project-schema` prints
  that schema without a document or Qt, keeping exact uint64 bounds. Required fields, nulls,
  open unknown keys, ranges and the 32 MiB image byte limit keep their existing codec behavior.
- **Limits:** partial coverage is explicit. Native integer tokens and UTF-8 byte lengths need
  codec checks beyond standard schema validation. Nested network/demand/optional feature rules
  remain codec-owned; desktop observed discovery and the engine are unchanged.
- **Verified:** project tests pass 24/24; Release headless build and all 161 CTest entries pass,
  including the new schema CLI contract. A local independent jsonschema 4.26.0 Draft-2020-12
  check passes 54 adversarial cases and all four shipped projects, with expected token/byte and
  partial-coverage differences. Before/after canonical output for those four projects is
  byte-identical. Evidence is Linux headless only; no Qt/Windows acceptance claim.

## 2026-10-26 — Project schema inventory and shared writer inspection (D355)

- **Mapped:** optional root fields, absence/null/empty rules, nested codec owners and writer
  feature floors 17–57 in PROJECT_SCHEMA. Corrected stale reader limits in the editor and
  automation references; nested formal field metadata remains separate from observed discovery.
- **Changed:** `documentSchemaVersion` exposes the writer's existing feature choice without
  serialization or validation. `documentJson` uses it; no second version calculation, retained
  input version or file-format change is introduced.
- **Verified:** before/after canonical output for all four shipped projects is byte-identical
  (combined SHA-256 `47cd924427e13e7d6e1a63215bde24938101b9cc079746c6393f22f9ecb35c64`).
  Four regression cases cover feature removal/precedence, invalid-draft inspection, all 57
  supported input versions and shipped-project round trips. Project tests pass 20/20; Release
  headless build and all 160 CTest entries pass, including architecture, documentation, file-size
  and reference checks. Linux only; no Qt/Windows acceptance claim.

## 2026-10-26 — Keep bounded schema discovery permissive (D354)

- **Fixed:** observed discovery could exhaust its visit budget inside an object, then restrict
  the enclosing array to objects despite unseen scalar siblings. Arrays with unvisited items
  now use unconstrained item schemas. Depth-limited containers stop before scanning children.
- **Changed:** extracted the structural generator into Qt-free project `observed_schema`, used
  by desktop discovery. The same 50,000-value/depth-32 limits, non-authoritative metadata and
  codec dry-validation path remain; this does not implement a complete formal document schema.
- **Verified:** a standalone reproduction using the previous generator rejects the unseen
  scalar shape; the revised helper accepts it. Four new headless tests cover mixed/empty/null
  samples, nested truncation, exact visit-budget and depth boundaries. Project tests pass 16/16;
  Release headless build and all 160 CTest entries pass, including documentation, architecture,
  file-size and reference checks. Qt tools are unavailable locally; no desktop/Windows claim.

## 2026-10-26 — Resolve authored merge paths once per grouping call (D353)

- **Found:** a sampling profile of the 16-Node benchmark put about 76 % of samples under
  `mergeGroups`, mainly resolving the same authored sides again for every receiving group.
- **Changed:** build the same ordered groups, then resolve each authored merge's pair once and
  mark groups containing both paths. Empty groups, unresolved/self pairs and crossing areas keep
  their existing semantics. No retained cache or change to network-wide conflict scope.
- **Measured:** isolated Release medians of three: 4 Nodes 41.86 → 26.94 ms; 16 Nodes
  2.193 → 0.609 s, about 72 % less wall time. Both generated documents are byte-identical.
  This meets the local 1.5 s performance target and closes no milestone or owner gate.
- **Verified:** automatic-conflict tests pass 10/10, including both driving sides, unchanged
  group/arrival ordering, unrelated groups and invalid/self/crossing/cross-group pairs. Release
  headless build and all 160 CTest entries pass, including documentation, architecture, file-size
  and reference checks. Linux evidence only; no Windows/UI acceptance claim.

## 2026-10-26 — Candidate ID checks without rebuilding a set (D352)

- **Changed:** `allocateId` checks candidates directly against the document, without collecting
  every existing ID into a new set. A shared short-circuit traversal preserves `documentIds`'s
  namespace and Macro-run switch; allocation also checks the existing zone/mode namespaces.
  No retained index, public API or saved format changes; counters and limit errors stay the same.
- **Measured:** Release, GCC 14.2.0, no concurrent builds: 4 Nodes 48.39 → 45.43 ms (median of 3);
  16 Nodes 2.516 → 2.312 s (median of 6, two batches with order reversed), about 8 % less wall
  time. Full 4/16-Node documents are byte-identical. The ≤ 1.5 s target remains open.
- **Verified:** focused project tests pass 12/12, including collisions across 25 collections,
  Connector lane-path suffixes, enumeration scope and counter exhaustion. Release headless build
  and all 160 CTest entries pass, including architecture, documentation, file-size and reference
  checks. Evidence is Linux only; no Qt UI or Windows claim.

## 2026-10-26 — Call-local Connector surfaces for Node detail (D351)

- **Changed:** `automaticConflicts` shares one Connector surface cache through overlap
  classification and mouth/display polygon construction. It lives for that immutable call only;
  normal geometry queries remain uncached. Existing entry points retain their signatures.
- **Measured:** isolated Release medians, 4 Nodes 74.49 → 60.96 ms, 16 Nodes 3.059 → 2.728 s;
  complete generated documents byte-identical. The 1.5 s target remains open. A checked-in
  `multra-node-detail-benchmark` retains the recipe and optional JSON output for the next slice.
- **Verified:** the focused automatic-conflict suite passes 9/9, including exact cached/uncached
  stations, classifications and polygons on both driving sides, and a geometry edit between calls.
  Release headless build and all 160 CTest entries pass (including architecture, file-size,
  documentation, reference and group-registration checks). The retained benchmark also produces
  the baseline 4-Node bytes. Toolchain: CMake 4.4.4, Ninja 1.13.2, GCC 14.2.0, JSON 3.11.3,
  installed under the workspace without system changes. No Qt UI or Windows verification.

## 2026-10-26 — Codec discovery, dry validation and run-until (D350)

- **Asked:** continue with schema discovery and run-until while keeping the GUI responsive.
- **Changed:** bounded observed-shape discovery reports its non-authoritative coverage and
  codec versions. Dry validation uses real parsing/History without editing live work; patch
  protection is shared with real commits. A guarded run-until uses the existing timer/observers
  and run ID, fixed-grid targets, bounded fast stepping and deadline outcomes; Pause/Cancel,
  Step, reset and invalidation stop it. GUI text identifies the target and disables the ordinary
  playback speed selector while it is active. No new worker, job registry or polling timer.
- **Validation:** Debug desktop build and full `check` passed, all 239 CTest entries (164.07 s).
  After the final fast-to-playback clock reset, rebuild and all five affected automation/run
  suites passed again (14.60 s); documentation, file-size and architecture checks passed.
  Tests cover codec/dry-run isolation/errors, target acceptance before stepping, exact replay,
  GUI heartbeat, timeout, duplicate/conflicting targets, cancellation and playback transition.
  The compiled desktop/control pair discovers/validates and runs to completion before export.
  Full formal schemas are not claimed; preparation remains synchronous and Windows is unrun.

## 2026-10-26 — Single-run identity and explicit CSV export (D349)

- **Asked:** continue with run identity and reporting independent of the selected Results tab.
- **Changed:** successful GUI preparation assigns a single-run ID/source revision; pause/resume,
  reconnect and completion preserve it. Existing invalidation paths retain the latest ID/reason.
  Optional runId rejects stale simulation/results requests. Explicit movements export requires
  a current finished single run and reuses CSV/provenance formatting plus atomic writes; legacy
  unqualified export retains tab-based behavior. No job registry, engine RNG change or timer.
- **Validation:** Debug desktop build and full `check` passed, all 239 CTest entries (162.59 s).
  Six targeted automation/run suites passed. Coverage includes lifecycle/reconnect, reset/seed/
  edit/Open/batch invalidation, stale/partial export refusing an existing destination, CSV byte
  equality with Macro selected, and the compiled desktop/control pair exporting a named run
  to a Unicode path and rejecting its old ID after restart. Windows remains unrun; explicit
  other-engine/batch exports and run-until are separate work.

## 2026-10-26 — Connector proposals and candidate diagnostics (D348)

- **Asked:** continue with geometry suggestions and diagnostics, preserving existing geometry.
- **Changed:** connector.suggest runs UI authoring on a copy and returns a proposed patch,
  provisional ID and explicit requested/actual ranges. Read-only requests leave History,
  selection and run unchanged. Rejected patch attachment issues report the candidate's
  expected/actual positions, distance, tolerance and geometry pointer. The unchanged 0.01 m
  validation threshold and measurements share one model helper; core types were not expanded.
- **Validation:** Debug desktop build and full `check` passed, all 239 CTest entries (142.46 s).
  Tests cover preview/apply/Undo/Redo, stale guards after an intervening edit, interior stations,
  invalid inputs/duplicate connections, driving sides, range narrowing, exact JSON allocator
  limits and candidate endpoint/reference measurements. The existing attachment tolerance
  remains unchanged. Windows runtime/appearance remains unrun.

## 2026-10-26 — AI map commands and tile readiness (D347)

- **Asked:** continue with map settings and tile-loading state through commands.
- **Changed:** map.get/status expose installed providers/settings and current viewport counts;
  map.set takes coordinates, derives UTM and uses the same apply path as Map settings; remove
  is idempotent, retry uses existing view-only backoff. Parameter failures include path/expected.
  Ready requires a settled visible viewport and complete wanted tiles; read requests create
  no fetches or new polling timers. Existing History and run invalidation remain.
- **Validation:** Debug desktop build and full `check` passed, all 239 CTest entries (135.03 s).
  Eleven targeted automation/map suites passed. A local HTTP tile fixture covers held loading,
  complete tiles, failed images/retry, pending-pan readiness, hidden retry, unavailable providers,
  invalid fields, no-op set/remove and Undo/Redo. External OSM and Windows remain unverified.

## 2026-10-26 — Deferred recovery commands (D346)

- **Asked:** continue the AI workflow with recovery management that avoids blocking startup
  while preserving the normal user recovery prompt and live editors' drafts.
- **Changed:** --recovery ask/defer (default ask), startup policy in state, explicit metadata-only
  recovery list, guarded restore with dirty-work permission, and confirmed single-draft discard.
  Basename-only IDs reject paths and symlinks; locks are rechecked at use. Restore retains the
  existing untitled/dirty semantics and adopts the draft lock; autosave/manual Recover stay on.
  No background scan or default deletion was added.
- **Validation:** Debug desktop build and full `check` passed, all 239 CTest entries (137.91 s).
  Seven targeted automation/storage suites passed. Tests cover both startup policies with
  real pending files, live/current ownership, list-to-use lock races, invalid/missing IDs,
  corrupt recovery, dirty replacement, adopted locks, and save/discard isolation. A separate
  production-desktop smoke test with isolated workspace XDG storage waited for real autosave,
  intentionally crashed only its test-owned process, restarted with defer, listed/restored the
  preserved draft, verified its edited content, saved and quit normally. Invalid CLI policy
  was rejected with exit 2 and no window. Windows runtime remains unrun.

## 2026-10-26 — Guarded New and intentional Quit (D345)

- **Asked:** continue the reviewed AI workflow with New in the same window and intentional
  Quit that preserves unsaved work unless explicitly discarded.
- **Changed:** File → New and `file.new` share one reset path. New/Quit keep document/revision
  guards and require boolean discard permission for dirty work. Accepted Quit freezes edits,
  refuses subsequent mutations and closes only after transport handoff of its response. A
  failed/stalled handoff cancels closing and restores editing; no process kill is introduced.
  The drain monitor is allocated only for an intentional Quit reply, with a 5 s deadline.
- **Validation:** Debug desktop build and full `check` passed, all 239 CTest entries (139.64 s).
  Lifecycle coverage includes recovery isolation and pipelined mutation refusal;
  compiled-process coverage checks ten clean/discard Quit cycles,
  full responses and normal exits; transport coverage forces backpressure, disconnect and timeout.
  Windows runtime/appearance acceptance is unrun.

## 2026-10-26 — Live AI endpoint and reconnect status (D344)

- **Asked:** implement stages 1–2 of the reviewed plan: keep the shared window open, enable
  automation during work and report useful state without AI polling in normal use.
- **Changed:** Help enables/disables a lazily allocated listener and displays its endpoint.
  Disable closes clients/releases the name without touching the document, Undo, selection,
  run or autosave. Interactive bind failure keeps the window; CLI exit-2 compatibility stays.
  State adds instance identity, listener readiness, startup completion, structured busy reason,
  and optional compact status; document/revision guards and existing selection defaults stay.
- **Validation:** Debug desktop build and `check` passed, all 239 CTest entries (127.25 s),
  including all four automation suites. The new lifecycle test uses real local sockets
  and the menu dialog, including dirty document preservation, reconnect during playback,
  incomplete requests on disable, occupied endpoint refusal and stale document rejection.
  Windows runtime acceptance is unrun; Linux offscreen is not cross-platform evidence.
- **Resource sample:** Linux offscreen automation test host, after 1.5 s settling, 2 s idle per
  mode: disabled 61.6 MiB RSS, listener idle 61.9 MiB; both 0 CPU ticks observed. This short
  single-process sample is not a benchmark or a zero-cost guarantee. No AI polling timer was added.

## 2026-10-26 — Node detail authors from one conflict list (D343)

- **Asked:** measure-first optimization of the engine, build, docs and design (owner: "ทั้งหมด",
  then "ทำตามแนะนำ"). The engine (0.09–0.14 ms per tick at 491–637 vehicles), docs and desktop start
  (0.26 s) needed nothing; `types.hpp` stays the owner's deferred split (506 objects rebuilt).
- **Found:** `generateNodeDetail` and `authorConflictGroup` authored each pair through
  `authorAutomaticConflict`, which rebuilt `automaticConflicts` for the whole network per pair to
  refuse a stale click: 92 % of generating a Node (callgrind). Release, one Node 65 → 12 ms; 16
  unconnected Nodes 15.5 → 3.1 s (median of 3). `studyarea` (Debug) 50 → 16 s.
- **Change:** both loops call `detail::authorCurrentConflict` with the entry of the list they took
  before the first area; the canvas click keeps its check. A probe on every pair (954 node-detail,
  19 group) found the rebuilt entry equal to the loop's; the generated documents hash identical.
- **Verification:** Linux, GCC 13.3, desktop Debug offscreen: `check` passes (238/238).
  No Windows run here.

## 2026-10-26 — Review fixes to D339–D341 (D342)

- **Asked:** review every open PR, then improve all findings and merge (owner: "ปรับปรุงทั้งหมด และ Merge").
  PRs 193–196 merged as reviewed; this follow-up carries the fixes.
- **Meso OD rule per matrix:** D341 switched every OD matrix to graph paths once any Road
  connector existed. A Road connector in an unrelated zone thus turned a Link-only matrix from
  running into `MESO_OD_NO_PATH` and changed its loading (probe: 0 → 900 trips, refused). Now a
  matrix takes the graph rule only when one of its zones has a Road connector; the rest run the
  legacy expansion on the document less those matrices and Road connectors. The parking refusal
  and endpoint checks are limited to the graph-rule matrices' zones.
- **Ties:** equal-time paths compare within a 1e-12 relative tolerance. Summing the same link
  times in another order differed by an ulp in about 18 % of realistic detour pairs, so one of two
  equal detours took all the demand; MR1's unit-speed integers could not show it.
- **Macro side file:** a named run's period records `runDefinition {id, name}`, so a study-area cut
  from the side file can name the definition behind its flows; older files still read.
- **Tests:** `kLatestSchema` names the newest readable schema, and the 16 "next schema refuses"
  tests use `kLatestSchema + 1` instead of a literal edited at every bump. MR7 covers the
  per-matrix rule, parking scope and reordered ties; MA20 the side-file definition.
- **Verification:** Linux, GCC, headless preset: full `multra-tests` pass (Qt/desktop and Windows
  untested here; CI covers them on the PR).

## 2026-10-26 — Meso OD demand on directed Roads (D341)

- **Asked:** continue development; followed NEXT's Meso Road-OD seam after D340.
- **Changed:** the existing schema-57 Road targets opt OD matrices into minimum free-flow-time
  paths on the compiled Meso graph, with equal shares over tied paths. Connector shares, named
  modes, PCU and clock/period windows are resolved from the original document/catalogs; authored
  and routeless paths remain alongside graph OD. No LTM engine or project-schema change.
- **Refusals:** unavailable directions, any unserved positive connector pair, unknown mode/type
  references, missing PCU, invalid windows and path counts above 4096 block Run. Parking-lot
  dynamics remain Micro's and refuse in this new reader. Micro still requires generated Links.
- **Results:** Road runs identify `minimum-free-flow-time` in JSON/CSV and the English/Thai Meso
  note. The existing worker and Road dialog now support both Macro and Meso. Source bytes and
  ownership stay unchanged; paths keep collision-safe matrix/zone/connector identities.
- **Tests:** nine model cases cover directional endpoints, time/turn costs, modes, all shortest
  paths, 5000-link routes, the path cap, shares (including overflow-sized totals), windows,
  mixed/authored/routeless demand, refusals, fluid count conservation, replay and provenance.
  The existing `od:` path namespace preserves study-area zone identity and live/side-file parity.
  Real CLI and editor tests run both levels and retain the Micro refusal.
- **Compatibility:** legacy CLI CSV is byte-identical; JSON differs only in `buildCommit`.
- **Verification:** Linux, GCC 14.2 and Qt 6.8.2 offscreen: full desktop build and final `check`
  pass, **238/238 CTest cases passed**. All nine focused model cases, the expanded real CLI
  regression and the Road editor harness also pass. Fontconfig uses bundled Thai fonts with
  DejaVu Sans fallback. Windows untested for this change; no milestone gate closed.

## 2026-10-26 — Road-only Macro OD demand (D340)

- **Asked:** continue development; followed NEXT's Road-only demand seam booked by D310.
- **Changed:** schema-57 zone connectors can target a Link-less Road in an explicit travel
  direction. The Macro graph reads those endpoints, so the existing OD shares, PCU, periods,
  assignment, named runs and exports work without generating Micro Links. The Zones tab adds a
  Road connector dialog with English/Thai text and endpoint drawing.
- **Lifecycle:** Apply template remaps every connector to its generated Link in one Undo step;
  split preserves the physical end and delete cascades connectors while keeping matrix counts.
  A two-leg merge remaps external endpoints and keeps Roads separate if a connector uses their
  shared end. Commands retain id values across Road-vector mutation, including aliased arguments.
- **Boundary:** Micro and Meso explicitly refuse direct Road targets, preventing a successful
  empty or partial-demand run. Meso needs its own graph-path expansion; the Micro study-area cut
  keeps its existing contract. Existing Link JSON and shipped project bytes remain unchanged.
- **Tests:** RD30–RD34 cover codec/version refusals, exact directional flows, share splitting,
  periods, mixed Link/Road paths, one-way/banned turns, lifecycle/Undo, editor worker and dialogs,
  save/reopen, original CLI input provenance and Micro/Meso refusal.
- **Verification:** Linux, GCC 14.2 and Qt 6.8.2 offscreen: full desktop build and `check`
  pass, **237/237 CTest cases passed**. Six focused model cases, the new UI harness and the
  real CLI regression also pass independently. Fontconfig uses bundled Thai fonts with DejaVu
  Sans fallback. Windows untested for this change; no milestone gate closed.

- **CI follow-up (2026-10-26):** [run 642](https://github.com/Warissakorn/multra/actions/runs/40253582320)
  passed Linux headless/sanitizer/release/desktop and Windows core/desktop. Automated evidence
  for D340; no owner appearance gate closed.

