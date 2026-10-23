# Readiness, *Fill defaults* and Node proposals (S4)

Written 2026-10-23 (D313) as the S4.1 contract, docs only. The owner answered its four choices the
same day, as recommended: S4-1a, S4-2a, S4-3a, S4-4a (§8), so §3–§6 state the chosen options. S4.2
is implemented (D314, §10). It fixes, for step 3 and step 6 of the workflow
loop ([MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) §5):
- the readiness panel: one list per level of what would stop or weaken a run;
- *Fill defaults*: which absent values a command may fill, and from which cited source;
- Node proposals: which results point at a Node to refine.

Rule 4 applies: every result carries the not-validated marker. Readiness says a run will start,
not that its results are right.

## 1. Scope

- **In S4:**
  - the readiness panel for Micro, Meso and Macro (per study period), with the seam checks (S3.7);
  - *Fill defaults*, each fill listed with its source and applied only when chosen;
  - Node proposals from a Macro or Meso result.
- **Out:**
  - the study area and "`generated` objects inside the study area count as missing for Micro"
    (WORKFLOW §5): S5, which brings the study area;
  - calibration against counts and proposals from a failed calibration: M10's CountSet;
  - export marking while `generated` objects or stale results remain: S5's stale marking.

## 2. Today (measured from the code on D312's commit)

| Level | Before a run | Where |
|---|---|---|
| Micro | `runDiagnostics`: draft, runtime and advisory rows; *Recheck* in the Problems tab | `src/model/network/diagnostics.*`, `editor_diagnostics.cpp` |
| Meso | Refusals only when *Run Meso* fails: `mesoNetwork` then `mesoDemand` issues, by stage | `src/project/meso_input.*`, `meso_run.*` |
| Macro | Refusals only when *Run Macro* fails, per period: `macroNetwork` then `macroDemand` | `src/project/macro_input.*`, `macro_run.*` |
| Seams | *Seam checks…* on the Roads tab, by level (D312) | `src/model/demand/seam_checks.*` |

So a Meso or Macro study learns what is missing one refusal at a time, by running.

## 3. The readiness panel

- **One list per level** (Micro, Meso, Macro; Macro also per study period). Each row is
  `{severity, code, path, object}`, translated as the Problems tab's rows are, and selecting a row
  selects the object on the canvas.
- **Rows:**
  - **Micro:** `runDiagnostics` as today, plus `seamChecks(micro)`;
  - **Meso:** `mesoNetwork` issues and advisories, then `mesoDemand`'s, plus `seamChecks(meso)`;
  - **Macro:** `macroNetwork` issues and advisories, then `macroDemand`'s for the chosen period,
    plus `seamChecks(macro)`.
- **Severity:**
  - *refusal*: the run would not start (a compile or demand issue);
  - *warning*: a seam warning;
  - *advisory*: everything else.
- **Agreement:** the panel reads the same functions the runs call, in the same order. So "no
  refusal" holds exactly when the run starts. This is a row (RS2), not a promise.
- **On demand:** like *Recheck*, the panel is computed when asked and is marked stale after any
  edit. Compiling three levels on every mouse-up would be wasteful (the Problems tab's reason).
- **Fills offered:** a row that a fill in §4 can clear shows that fill.

## 4. *Fill defaults*

- **The principle** (WORKFLOW §1): a default is an absent field, and no default is uncited.
  - A fill writes a present field, so each fill names its source and is chosen by the user.
  - All chosen fills apply in one Undo step.
  - Nothing is filled during a run or without a click.
- **Candidates** (S4-2a: F1–F3 in S4.3; F4 is its own later slice):

  | Fill | Clears | Value and source |
  |---|---|---|
  | **F1** Link type from its Road | `MACRO_LINK_TYPE_MISSING` on a Link with `roadId` | The Road's template `linkType`, so the source is the template's `source` |
  | **F2** Jam density | `MESO_JAM_DENSITY_MISSING` on a project-owned type | 190 pc/mi/ln = 118.06 pc/km/ln, HCM 7th ed. Exhibit 10-7 (ROADS §8), source written with it |
  | **F3** Generate a Road's Links | `SEAM_ROAD_NOT_GENERATED` (Micro) | *Apply template* (D305); the Links carry `roadId` |
  | **F4** Generate a coarse Node's Micro detail | A coarse Node in a Micro run | One Connector per allowed turn, heads from its stages, conflict areas from major and minor, all `generated` |

  - **F1–F3** are small and use values the project already cites.
  - **F4** is WORKFLOW §3's "turns propose Connectors, major/minor gives conflict areas with
    Yield/Stop, stages give a program and heads". It is a slice of its own: lane mapping, heads and
    priority rules each have open questions.
- **Not fillable:** a vehicle type's PCU (D267: no PCU ships, `MODE_PCU_MISSING` stays the user's)
  and a signal's cycle or stages (NODES §5: a signal needs user-entered values).

## 5. Node proposals

- **Step 6** (WORKFLOW §5): after a run, the program lists Nodes worth refining. It never refines
  one itself.
- **Triggers** (S4-3a):
  - **Macro:** a Node with a leg whose link has v/c ≥ 1.0 in the chosen period's result. 1.0 is
    capacity by definition, so it takes no number of its own.
  - **Meso:** a Node with a leg whose link reports spill-back in any slice (`spillBack`, D300).
- **What a row holds:** the Node, the level, the result's revision, the leg links and their v/c or
  spill-back slices. A stale result (an older revision) proposes nothing.
- **Where no Node covers the link's end,** the row names the link end instead, with *Suggest
  nodes* (S0.5) offered there.

## 6. Editor

- The Problems tab gains a level selector (Micro, Meso, Macro with its period), *Recheck* for the
  chosen level, and a *Fill defaults…* button. That button lists the fills that would clear the
  current rows, each with its source and a check box.
- Node proposals appear after a Macro or Meso run as a list on the Nodes tab, and selecting one
  selects the Node.

## 7. Slices

| Slice | Content | Rows |
|---|---|---|
| S4.1 | This contract. **Written (D313)** | — |
| S4.2 | The readiness panel by level, with the seam checks. **Implemented (D314)** | RS1–RS4 |
| S4.3 | *Fill defaults* F1–F3 | RS5–RS8 |
| S4.4 | Node proposals | RS9, RS10 |
| (S4.5) | F4, generating a coarse Node's Micro detail, if chosen | its own contract section |

## 8. Owner choices (answered 2026-10-23, D313: S4-1a, S4-2a, S4-3a, S4-4a)

1. **S4-1 The panel's place.**
   - **(a) The Problems tab with a level selector (recommended).** One place for every level, and
     the Micro rows stay where users know them.
   - (b) A separate *Readiness* dialog.
2. **S4-2 *Fill defaults*' scope.**
   - **(a) F1–F3 now; F4 as its own later slice (recommended).** Each uses a value the project
     already cites. F4 needs its own contract.
   - (b) F1–F4 in S4.
   - (c) No fills; the panel only.
3. **S4-3 Node proposal triggers.**
   - **(a) Macro v/c ≥ 1.0 and Meso spill-back, after a run (recommended).** Neither needs a new
     number.
   - (b) A user threshold, for example v/c ≥ 0.85, set in the panel.
   - (c) Defer proposals until calibration exists (M10).
4. **S4-4 Slice order.**
   - **(a) S4.2 panel → S4.3 fills → S4.4 proposals (recommended).** The panel is what the fills
     and proposals hang on.
   - (b) Proposals first.

## 9. Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| RS1 | Byte identity | Opening the panel or listing fills changes no byte and no revision |
| RS2 | Agreement | For each level, on the shipped projects and a Road-only fixture, the panel has a refusal row exactly when the run is refused, with the same codes and paths |
| RS3 | Seams in the panel | A lane-count seam shows at Micro as a warning and at Meso as an advisory, as `seamChecks` gives |
| RS4 | Stale | An edit after *Recheck* marks the panel stale; *Recheck* renews it |
| RS5 | F1 | A generated Link without a type gets its Road template's type, and Macro's refusal for it goes, in one Undo step; the Link stays attached |
| RS6 | F2 | An owned type without jam density gets 118.06 with the HCM source, and Meso's refusal goes; an installed type is not offered |
| RS7 | F3 | A Link-less Road gets its Links, and Micro's not-generated row goes |
| RS8 | Chosen only | Unticked fills change nothing, and Undo removes every ticked one at once |
| RS9 | Proposals | A Macro result with v/c 1.2 on a Node's leg proposes that Node; v/c 0.9 does not; a Meso result with spill-back on a leg proposes it; an edit makes both stale |
| RS10 | No Node | A link end with v/c ≥ 1 and no Node is listed by its end, with *Suggest nodes* offered |

## 10. S4.2 as implemented (D314)

- **`readiness(document, data, level, period)`** (`src/project/readiness.*`) never throws and changes
  nothing. It returns rows `{code, path, objectId, selectId, severity}`, where severity is
  *refusal*, *warning* or *advisory*.
  - **Micro:** `runDiagnostics`; its draft and runtime rows are refusals.
  - **Meso:** `mesoNetwork`'s issues; when the graph compiles, `mesoDemand`'s, as `runMeso`'s stages
    go.
  - **Macro:** `macroNetwork`'s issues, then the chosen period's `macroDemand` (an empty or unknown
    name takes the first period). No period at all is `MACRO_PERIOD_UNKNOWN`.
  - **Then** each level's advisories, then `seamChecks` for the level.
  - **Exceptions:** an unreadable catalog is one refusal row named by the exception (for example
    `EDIT_CATALOG_READ`).
  - **Objects:** paths under `network.` resolve to an object id (Links, lanes, Connectors, heads,
    Roads and Nodes). Links and Connectors are selectable on the canvas.
- **Agreement (RS2)** is checked against the runs themselves. The fixtures are the shipped projects,
  a Road-only fixture, a typed two-route OD network with and without PCUs, and four-leg with an
  empty Node. Each run is stopped by an observer once past its refusals.
  - **Meso and Macro:** the panel's refusals equal the run's, by code and path.
  - **Micro:** the panel has a refusal exactly when `compileDocument` refuses. Micro's rows carry
    their diagnostics paths, not the compile's.
- **Editor** (the Problems tab):
  - a level selector (Micro, Meso, Macro) and, for Macro, the period. Choosing either is a check,
    and *Recheck* checks the chosen level;
  - Micro shows its diagnostics as before, plus its seam rows; Meso and Macro show the readiness
    rows, with *Refusal*, *Warning* or *Advisory* as severity;
  - an edit after a check leaves one note, "Edited since the last check: Recheck to renew this
    list". Before, the list emptied silently;
  - the tab's page now holds the selectors above the table, so code that reveals the tab reveals
    the page.
- **Rows:**
  - RS1–RS3 in `tests/readiness_tests.cpp` (`readiness`);
  - RS4 in `tables-ui`: the stale note, Meso's refusals, and Macro without a period.
- **Mutations caught:** the Meso demand stage skipped, the Macro demand stage skipped, seam warnings
  shown as advisories, and Micro advisories treated as refusals.
