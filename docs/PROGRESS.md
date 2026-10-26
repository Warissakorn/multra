# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
