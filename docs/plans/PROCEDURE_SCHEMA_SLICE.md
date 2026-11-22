# PROCEDURE_SCHEMA_SLICE — the next schema slice: `procedures` (schema 73, feedback from 75)

The brief for the slice after D573–D574, moved verbatim from [NEXT](../NEXT.md) on 2026-11-22. It waits for the
owner's review of D573–D574; NEXT says when it is due. Contracts: [PROJECT_SCHEMA](../reference/PROJECT_SCHEMA.md),
[PROJECT_SCHEMA_FEATURES](../reference/PROJECT_SCHEMA_FEATURES.md).

- **Following slice (`procedures`, schema 73, feedback from 75), after owner review of D573–D574.**
  Stop before starting it. Read D571–D574's diffs and copy the pins-first pattern as D574 left it:
  `networkCodec::onlyKnownKeys` for a reader with no version, named floor constants (add
  `procedureVersion=73`, `feedbackVersion=75` and use them in `document.cpp`'s guards and save
  chain), `tests/schema_refusal.hpp` for refusal pins, and the shared `citedSource` row if a source
  appears. Contracts: PROCEDURE_SEQUENCE §2 and
  PROCEDURE_FEEDBACK §2; inventory the implemented reader rather than inferring from the writer.
  1. **Inventory (read, change nothing):** procedure_codec.cpp's parseProcedures, step,
     feedback, feedbackIssues and procedureIssues; document.cpp's guards and save selection.
     Root procedures is refused below 73 at its path, even null. Feedback's pre-75 guard runs
     AFTER parseProcedures: malformed feedback can fail INVALID_PROCEDURE before the guard;
     valid feedback below 75 gives EDIT_UNSUPPORTED_FIELD at procedures[i].feedback. Keep this
     order. Other codes are INVALID_ID and DUPLICATE_ID. Inspect kKinds/procedureStepKindName.
  2. **Pins first (PRS1):** tests/procedure_schema_tests.cpp and _cases.hpp, registered in CMake,
     built/passing on unchanged readers before metadata/schema/codec edits. Pin kind/type/key
     paths, active checked before keys, missing ids, assignment's exclusive presence, feedback
     methods/bounds/native integer tokens and complete issue order, including post-parse guards.
  3. **Rows as read:** root false below 73, then a nonempty array of closed rows requiring text
     id/name and nonempty steps; feedback false below 75. Steps are four closed rows tagged by
     required kind; reuse the existing kind formatter for tag strings. Optional active is boolean
     (true is accepted even though only false is written). Generation requires text rateSet;
     distribution requires text deterrenceSet, with optional text skimMode/costMatrix; modeChoice
     requires text modeChoiceSet, with optional nonempty text periods; assignment needs exactly
     one of text period or macroRun (exclusive presence, unlike D573's inclusive sources).
     Feedback is closed {method, weight?, maxIterations, tolerance}: method msa or constantWeight;
     constantWeight requires numeric weight in [0, 1), msa forbids weight; maxIterations is a
     native integer token in [2, 1000], tolerance numeric ≥ 0. Blank/unique ids remain codec
     checks; dependency/known-reference/run rules remain with the existing run. No step-order
     restriction or uniqueness inferred from example procedures.
  4. **One key source:** replace only onlyKeys lists with shared field tables through
     `onlyKnownKeys` (reader has no version; it checks at kLatestSchema). Keep kind-before-active, active-before-keys, field order,
     int64 conversion/clamping, feedbackIssues and the document's post-parse feedback guard.
     Do not change writer bytes, save floors, step formatting, catalogs or runs.
  5. **Coverage:** add -procedures to the value and update $comment, C++/CLI pins, help, tools
     README and DESKTOP_AUTOMATION's sentence; keep x-multra-complete false.
  6. **Labels in `check` (D574):** add the new cases namespace to
     `tests/schema_labels_dump_tests.cpp`; ctest `schema-labels` then checks every label and every
     shipped project against the emitted schema. Still by hand (temporary, then deleted): writer
     output at 73 (all step kinds, both assignment forms, active false) and 75 (MSA and
     constantWeight, numeric boundaries), and about 20 probes outside cases(). Native-token floats
     such as 2.0 may be labelled schema-accepted/codec-refused; schema-refused/codec-accepted must
     be empty.
  - **Pitfalls:** filter by group (multra-tests project), not test name; optional-only closed rows
    emit required: []; parseDocument calls validateDocument. The feedback floor is post-parse;
    do not move it earlier. Use PROJECT_SCHEMA_FEATURES for the new section and fix moved-section
    links if splitting again. Keep every file ≤500 lines; network_fields.cpp is 433 at D573.
    Headless is sufficient; report desktop/no-PCH/Windows honestly. The local linker-only
    -Wl,--strip-debug cache flag fits the small disk; repository presets remain unchanged.
  - **Session end:** headless build and cmake --build build/headless --target check green;
    append the next decision ID and index entry; PROJECT_SCHEMA_FEATURES PRS1–PRS3,
    PROJECT_SCHEMA/reference README updated; PROGRESS entry with the oldest moved whole to
    archive and its links rewritten; rewrite this brief and Review waiting; commit/push the
    same branch, without PR or merge, then stop for owner review.
