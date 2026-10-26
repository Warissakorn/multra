# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
