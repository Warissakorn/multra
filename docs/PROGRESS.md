# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-10-27 — Authored object table CSV export (M1.23.1, D365)

- **Asked:** switch from schema/AI work to the recommended M1.23 native object-table CSV slice.
- **Changed:** File → Export object table (CSV), also in the palette, exports Links, Connectors,
  inputs, routes and decisions from the current document without Run/catalog resolution. A
  Qt-free formatter shares persisted serialization on table-only projections, keeps full numeric
  precision, UTF-8 BOM/CRLF, authored row order and nested JSON cells. The shell selects Unicode
  paths, writes atomically and protects project/recovery destinations without changing History,
  revision, dirty/save state or results. A UI test caught nonexistent paths comparing equal in
  QFileInfo; protection now compares cleaned absolute names and nonempty canonical paths.
- **Verified:** five native regressions cover known 3–4–5 geometry, precise values, quoting/Thai,
  optional attachments, nested demand/counts, empty tables/order and locale independence; project
  tests pass 70/70. Twenty sample CSVs from all four shipped projects pass independent Python
  CSV/JSON checks. Release headless build and all 161 CTest entries pass (14.09 s). A local Qt
  6.8.2 SDK enabled a Release desktop build and object-export-ui (all five actions, unsaved edits,
  Unicode paths, cancellation, failed writes, protection and translation) plus results-tabs-ui pass.
- **Limit:** workspace-ui fails its existing numeric-font/tabular-digit assertion under this
  Qt environment; a separately linked pre-D365 workspace reproduces the same failure. A direct
  Qt font probe also disagrees with the bundled TTF's equal digit metrics. No test was weakened;
  the issue is recorded in NEXT. Windows/packaged acceptance is OWNER_SITTING G1. This slice
  closes no full M1.23, owner, runtime-model or scientific gate; other interchange formats remain.

## 2026-10-27 — RoutingDecision structural schema and unchanged counted synchronization (D364)

- **Changed:** shared metadata now covers RoutingDecision rows, route shares, periods and
  type-rule numeric matrices. Their native decoder moves to routing_codec; the parser is now
  222 lines. The composed partial schema/CLI uses 73 shared definitions and 16 version profiles.
  The schema and document precheck share the position version floor. Envelope-only schema and
  desktop observed discovery stay unchanged.
- **Preserved:** always-strict decision/share/type keys, open period keys, nullable optionals
  and contains-based routingDecisions/typeRules null refusal. The document-only pre-20 position
  guard still rejects null before normal decoding; bare/standalone reads retain support.
  Numeric-vector, matrix-row and matrix-cell diagnostics retain their original order/paths.
  Counted synchronization stays after complete definition decoding; model validators retain
  matrix dimensions, weights, time windows and destination/run-readiness checks.
- **Verified:** 5,073 cases across schemas 1–57 retain byte-identical native outcomes/errors/
  canonical documents and pass independent Draft-2020-12 expectations. Four shipped projects
  validate structurally and keep byte-identical canonical output. Five new regression tests
  cover strict/open shapes, nulls, position guards, matrix errors/semantics and counted sync;
  an independent review found no compatibility defect. Release headless build and all 161
  CTest entries pass (16.23 s), including CLI, docs, architecture and file-size checks. Linux
  headless evidence only; no engine algorithm, Qt/Windows, scientific or owner gate change.

## 2026-10-26 — Base Route/VehicleInput schema and preserved demand derivation (D363)

- **Changed:** shared metadata now covers definition duration/timeStep, Routes, VehicleInputs
  and volume intervals. Demand readers move to demand_codec; the parser is now 263 lines.
  The composed partial schema/CLI uses 67 shared definitions and 15 version profiles. Schema
  and document prechecks share the counted-volume version floor; the envelope-only API and
  desktop observed response stay unchanged.
- **Preserved:** open Route/input/interval keys, contains-based null refusal, scalar-before-
  interval diagnostics, interval-derived totals and empty-period behavior. Route migration and
  counted-volume synchronization retain their stages after full definition decoding; bare
  standalone reads retain their existing behavior. No catalog ownership, demand compilation,
  simulation dynamics, writer version-selection or serialized-output change.
- **Verified:** 6,156 cases across schemas 1–57 have byte-identical before/after native outcomes,
  errors and canonical documents and pass independent Draft-2020-12 expectations. Four shipped
  projects validate structurally and retain byte-identical canonical output. Five new tests
  cover required/open fields, nulls, integer-independent numeric types, scalar derivation,
  counted synchronization/version guards, route migration and semantic exclusions. Release
  headless build and all 161 CTest entries pass (11.59 s), including project, CLI, docs,
  architecture and file-size checks. Linux headless evidence only; no Qt/Windows, scientific
  validation or owner gate is closed.

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

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
