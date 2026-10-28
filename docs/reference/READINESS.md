# Readiness, *Fill defaults* and Node proposals (S4)

Written 2026-10-23 (D313) as the S4.1 contract, docs only. The owner answered its four choices the
same day, as recommended: S4-1a, S4-2a, S4-3a, S4-4a (§8), so §3–§6 state the chosen options. S4.2,
S4.3 and S4.4 are implemented (D314, D316, D317; §10–§12), so S4 is complete. It fixes, for step 3 and step 6 of the workflow
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
    (WORKFLOW §5): S5, which brings the study area ([STUDY_AREA](STUDY_AREA.md) §10);
  - calibration against counts and proposals from a failed calibration: M10's CountSet;
  - export marking while `generated` objects or stale results remain: S5's stale marking
    ([STUDY_AREA](STUDY_AREA.md) §9, §11).

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
  - **Micro:** `runDiagnostics` as today, `seamChecks(micro)`, and `MICRO_GENERATED_IN_AREA` per
    area Node holding generated objects ([STUDY_AREA](STUDY_AREA.md) §10, D332);
  - **Meso:** `mesoNetwork` issues and advisories, then `mesoDemand`'s, plus `seamChecks(meso)`;
  - **Macro:** `macroNetwork` issues and advisories, then `macroDemand`'s for the chosen period,
    plus `seamChecks(macro)`.
- **Severity:**
  - *refusal*: the run would not start (a compile or demand issue);
  - *warning*: a seam warning, or generated objects in the study area;
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
| S4.3 | *Fill defaults* F1–F3. **Implemented (D316)** | RS5–RS8 |
| S4.4 | Node proposals. **Implemented (D317)** | RS9, RS10 |
| S4.5 | F4, generating a coarse Node's Micro detail. **Contract written (D323, §13); S4.5a–c implemented (D323–D325), so S4.5 is complete** | RS11–RS19 |

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

## 11. S4.3 as implemented (D316)

- **`fillOffers(document, templates)`** and **`applyFills(document, chosen, templates)`**
  (`src/commands/fill_commands.*`). The templates are the resolved cross-sections, the list the
  Roads tab uses. Offers come in kind order, then in document order, and listing changes nothing.
  - **F1, a Link's type from its Road:** a Link with `roadId` and no type, whose Road's template
    names a type.
    - The value is that type; the source is "template <id>: <its source>".
    - `setLinkType` keeps the Link attached.
  - **F2, jam density:** a project-owned type without one.
    - The value is 118.0605 pc/km/ln, with the shipped catalog's HCM 7 Exhibit 10-7 source word for
      word (`hcmJamDensitySource`; RS6 compares both with `data/link-types/freeway-55.json`).
    - It is offered only when the value passes `linkTypeIssues`' jam-density rule for that type, so
      a fill never trades one refusal for another.
    - Installed types are never offered.
  - **F3, a Road's Links:** a Road without Links whose template resolves; *Apply template*.
  - **Applying:** only the chosen offers are applied, each checked against a fresh listing. An offer
    that no longer applies is `EDIT_FILL_STALE`.
- **Editor:** *Fill defaults…* sits beside the Problems tab's level selector. It opens a dialog with
  one row per offer: the fill, the object, the value, what it clears and the source, none ticked.
  - Confirm applies the ticked offers as one Undo step and checks the chosen level again.
  - With no offer, a note says so and Confirm is disabled.
- **Rows:**
  - RS5–RS8 in `tests/fill_defaults_tests.cpp` (`filldefaults`);
  - the editor part is in `road-ui`: F3 offered for an assigned Link-less Road, nothing changed
    while unticked, and the Links generated in one step once ticked.
- **Mutations caught:**
  - F1 detaching the Link;
  - F2 offered on an installed catalog;
  - F3 offered without a template;
  - stale offers applied.

## 12. S4.4 as implemented (D317)

- **`macroProposals(document, run)`** and **`mesoProposals(document, run)`**
  (`src/project/node_proposals.*`) never throw and change nothing. A run whose revision is not the
  document's proposes nothing.
  - **Hot arcs:**
    - Macro: a `link` arc with capacity and load / capacity ≥ 1.0, `macroJson`'s V/C;
    - Meso: a link with spill-back in at least one slice (`mesoLinkSlices`), counted in slices.
  - **A Node's leg:** the arc's source names one of the Node's `nodeLinkLegs` or `nodeRoadLegs`,
    and its station range reaches that leg's crossing from the outside, within `roadEndTolerance`.
    Macro keeps a detailed Node's Links whole while Meso cuts them at the boundary; the range rule
    serves both, and a piece of the same Link away from the Node is not its leg.
  - **No Node:** a hot arc no Node claims gives one row per end that is a junction (a Macro node
    with a Connector arc in or out, a Meso node with turns and no Node), named `<source>@<station>`.
    An end at the network's edge gives no row.
  - **Order:** Node rows in Node order, then no-Node rows by end; legs in graph order.
- **Editor:** under the Nodes tab's table, a note and a table (level, Node or "No Node at <end>",
  legs with their v/c or spill-back slices, revision), Macro rows first.
  - It is derived again when the revision or either result changes.
  - The note and the table take the page's space without asking for more, so the docks keep their
    sizes (`object-groups-ui` and `properties-layout-ui` caught both before).
  - Selecting a Node row selects that Node. A no-Node row's hint names *Suggest nodes*, or a drawn
    Node: *Suggest nodes* finds junctions with conflicts, so a plain diverge needs a drawn Node.
- **Rows:**
  - RS9 and RS10 in `tests/node_proposal_tests.cpp` (`nodeproposals`), on a split-and-merge network
    with a detailed Node, and a coarse Node over two Roads;
  - the editor part is in `macro-ui`: the rows after *Run Macro*, the no-Node hint, none once a
    Node is drawn (stale), and the Node proposed and selectable after a new run.
- **Mutations caught:**
  - the trigger at > 1.0 instead of ≥ 1.0;
  - the stale check dropped;
  - any piece of a leg's Link matching;
  - boundary ends listed.

## 13. S4.5: F4, a coarse Node's Micro detail (contract, D323)

Written 2026-10-23, docs only, at the owner's pick of F4. The owner answered §13.7's six choices the
same day, as recommended (F4-1a to F4-6a, D323), so §13.1–§13.6 state the chosen options. WORKFLOW
§3: "turns propose Connectors, major/minor gives conflict areas with Yield/Stop, stages give a
program and heads". Rule 4 applies: generated detail runs; it is not a calibrated junction.

### 13.1 What is offered

- **F4** is a fourth fill kind, `nodeDetail`, listed and applied by `fillOffers` and `applyFills`
  like F1–F3: one offer per Node, none ticked, all chosen fills in one Undo step.
- **It clears** `NODE_NO_TURN` at `nodes[i]`, which Micro reports today for a Node without detail
  whose legs are Links (membership finds legs and no turn).
- **Offered** for a Node without detail (`nodeHasDetail` false) when all of these hold:
  - every leg is a Link leg (`nodeLinkLegs`), at least two arms, each arm a Road or a Link without
    one. A Link-less Road leg is not offered; its row names F3, which comes first;
  - control is `priority` or `signalised`. `allWayStop` is not offered: the Micro engine has no
    all-way stop (`FILL_NODE_ALL_WAY_STOP`);
  - at a priority Node of three arms or more, major and minor are determined (`major`, else
    `majorByRank`); otherwise `NODE_PRIORITY_UNDETERMINED` names *Coarse values…*;
  - every cut (§13.2) lies inside the polygon (`FILL_NODE_CUT_OUTSIDE`);
  - for a signal, §13.5's layout exists (`FILL_NODE_STAGES_SPLIT`, `FILL_NODE_INTERGREEN`).
- A Node not offered lists its reason code in the *Fill defaults…* dialog, unticked and disabled,
  so the user learns what to fix. Listing still changes nothing (RS1).
- **Turns** are the coarse allowed turns: every (in arm, out arm) pair of different arms minus
  `bannedTurns`, as `compileRoads` takes them. U-turns are never generated.

### 13.2 Where the legs end (F4-1)

- Today a generated Link runs to its Road's end, near the Node's centre, so the legs overlap inside.
- **Cut (F4-1a):** each in-leg Link ends where its lane surface first meets another arm's leg
  surface, its stop line in a drawn junction; each out-leg Link starts where its surface last meets
  one. The opposite Link of the same Road is not another arm. A leg meeting no other arm keeps its
  end. The cut takes no number.
- **The cut keeps `roadId`:** the program made the Link and edits it. *Apply template* on a Road
  ending in a Node with detail cuts its Links the same way, so a re-apply after F4 changes no byte.

### 13.3 Connectors and lanes (F4-2)

- **One Connector per allowed turn**, from the in-leg's cut end to the out-leg's cut start, with
  `connectorEquation`'s default curve and the in-leg's level.
- **Lanes (F4-2a):** the turns from one in-leg are ordered by their sweep from the kerb side
  (`drivingSide`): kerb turn first, far turn last. With *k* turns over *n* lanes counted from the
  kerb, turn *i* takes lanes ⌊*i*·*n*/*k*⌋ to ⌈(*i*+1)·*n*/*k*⌉ − 1. Each turn has at least one lane,
  lanes are contiguous and shared when *n* < *k*. Two lanes and three turns give the kerb lane to
  the kerb turn and through, and the far lane to through and the far turn.
- **The out end** takes as many lanes as the in end, capped at the out-leg's count: from the kerb
  for every turn but the far one, which takes them from the far side.
- No number is taken; the lanes a study needs (pockets, exclusive turns) are the user's to draw.

### 13.4 Priority (F4-3, F4-4)

- **Areas:** each pair of generated Connectors whose surfaces overlap gets authored areas, as
  `addCrossingAreas` makes them (one per lane pair, a waiting line 1 m short, D63); merges into one
  out-leg are taken over as `takeOverMerge` does.
- **Who gives way (F4-3a):** HCM 7th ed., Ch. 20 §2, "Relative Priority of Various Movements at
  the Intersection" (owner's copy, PDF p. 1590; read for S4.5b, D324). Rank 1: major through and
  major kerb turn; 2: major far turn and minor kerb turn; 3: minor through, and the minor far turn
  at a three-leg junction; 4: the minor far turn at a four-leg one ("Rank 4 movements occur only at
  four-leg" junctions, which corrects this section's first draft). The HCM writes right-hand
  traffic, so its right turn is the kerb turn here and its left the far turn. The higher rank
  yields; an equal rank leaves the pair automatic (the drawing-order fallback for a merge).
- **Control:** a minor arm's yielding areas get *Stop* at their waiting line (F4-3a, two-way stop
  control as Ch. 20 models it).
- **Gap time and headway (F4-4):** `data/priority-rules/default.json`'s 3.0 s and 7.0 m, which
  every derived rule uses today and which carry their own "not calibrated" note (F4-4a).

### 13.5 Signals (F4-5)

- **A signalised Node** gets one controller named after the Node, with the coarse cycle and offset
  0, and one signal group per turn; each path of a turn's Connector gets a head at its start.
- **Layout (F4-5a):** stages run in their stored order, each followed by an equal intergreen
  *I* = (cycle − Σ green) / stages. Stage *s* starts at the sum, over the stages *j* before it, of
  greenⱼ + *I*. A turn's group is green over the stages that serve it, which must be consecutive,
  the last wrapping to the first (`FILL_NODE_STAGES_SPLIT` otherwise). Amber is the group default, 3 s, and an *I* below it is
  `FILL_NODE_INTERGREEN`.
- **Inside a stage,** turns green together whose Connectors cross get §13.4's areas and ranks
  (a permitted far turn gives way to the opposite through), with no Stop. So a signalised Node of
  three arms or more also needs its two major arms (owner, D325). Turns never green together get no
  crossing area.
- **Derive back** returns the same stages when no turn spans two (RS15). A turn served by two
  consecutive stages stays green through the intergreen between them, and derive back reports
  that span as it runs, so the stages gain it (owner, D325: each turn's g/C stays exact).

### 13.6 `generated`

- WORKFLOW §1.3: every object F4 creates carries `"generated": true` until a person edits it.
  Connectors, conflict areas, waiting lines, priority rules, Stop controls, heads and the controller
  gain the flag, written only when true, at schema 54; an older file carrying it is
  `EDIT_UNSUPPORTED_FIELD`. Any edit of the object clears it.
- **Each slice adds the flag to the objects it creates** (F4-6a), so no field is stored before
  something writes it: S4.5a Connectors, S4.5b the right-of-way objects, S4.5c heads and the
  controller.
- **S5 reads it** ("`generated` objects inside the study area count as missing for Micro", export
  marking); F4 only writes it.
- `settleNodes` refines the Node in the same Command: its `coarse` goes as the detail appears.

### 13.7 Owner choices (answered 2026-10-23, D323: F4-1a to F4-6a)

1. **F4-1 Where the legs end.**
   - **(a) Cut where a leg's surface meets another arm's (recommended).** No number, and the cut
     is where a drawn junction's stop line stands.
   - (b) Cut at the polygon boundary. It needs an inside tolerance, which has no source.
   - (c) No cut: Connectors leave from the boundary station mid-Link, and the stubs inside overlap
     and give conflicts no drawn junction has.
2. **F4-2 Lanes per turn.**
   - **(a) The kerb-to-far share of §13.3 (recommended).**
   - (b) Every turn from every lane: no rule, but turns from one approach cross one another.
3. **F4-3 Who gives way, and how.**
   - **(a) HCM 7 Ch. 20's ranks, Stop on minor arms (recommended).**
   - (b) The same ranks, Yield on minor arms.
   - (c) Major over minor only; turns on the same rank left automatic.
4. **F4-4 Gap time and headway.**
   - **(a) `default.json`'s 3.0 s / 7.0 m (recommended),** the derived rules' values.
   - (b) Ch. 20's base critical headway per movement as gap time.
5. **F4-5 Signal layout.**
   - **(a) Stored order, equal intergreens, one group per turn (recommended).**
   - (b) No signals in S4.5: a signalised Node is not offered.
6. **F4-6 Slices.**
   - **(a) S4.5a cuts, Connectors and `generated` (schema 54); S4.5b priority; S4.5c signals
     (recommended).** Each lands with its rows.
   - (b) One slice.

### 13.8 Acceptance rows (planned)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| RS11 | Offers | A four-arm coarse Node over F3's Links is offered, clearing `NODE_NO_TURN`; a Node with detail, a Link-less Road leg, `allWayStop`, undetermined priority, a cut outside, split stages and a short intergreen are listed with their codes and not applicable |
| RS12 | Cuts | Each leg ends or starts where its surface meets another arm's; `roadId` kept; *Apply template* again changes no byte |
| RS13 | Turns | One Connector per allowed turn, none for a banned turn or a U-turn; a two-lane approach shares by §13.3; `nodeMembership` then gives exactly the coarse turns, with no issue |
| RS14 | Priority | Minor arms yield with Stop to major through; equal ranks stay automatic; deleting the generated detail derives back the same `major` and `bannedTurns` |
| RS15 | Signal | The controller's cycle and each group's green follow §13.5; derive back gives the same stages |
| RS16 | `generated` | Written only when true; editing a Connector clears its flag; schema 54 gated |
| RS17 | One step | Unticked changes nothing; Undo restores every byte |
| RS18 | Runs | The four-arm fixture runs at Micro (seed 42) with no refusal, and Node evaluation has a row for every turn |
| RS19 | Arm outside (D382) | A Node narrower than its arms' carriageways, whose Roads end inside while their Links pass beside it, is `FILL_NODE_ARM_OUTSIDE`; the same arms under a wider Node still fill |

## 14. S4.5a as implemented (D323)

- **`Connector::generated`** (schema 54), written only when true; at 53 the key is
  `EDIT_UNSUPPORTED_FIELD`, and anything but `true` is `EDIT_CONNECTOR_INVALID`.
  - `settleGenerated(after, before)` runs in `History::execute`: a generated Connector the Command
    changed is the person's from then on. A copied Connector never carries the flag.
- **The plan** (`src/model/network/node_detail.*`, pure):
  - `nodeLegCuts(network, node, full)` measures every leg on its uncut geometry, lane by lane with
    `surfaceOverlaps`, against the legs of the other arms; only an overlap reaching the Node's side
    of the leg's crossing counts. A cut at or outside the crossing is `FILL_NODE_CUT_OUTSIDE`, and an
    overlap the resolver cannot measure `FILL_NODE_CUT_UNMEASURED`;
  - `nodeTurnConnectors(network, node)` gives §13.3's turns and lane ranges.
- **The command** (`src/commands/node_detail_commands.*`): `generateNodeDetail` cuts the legs
  (keeping `roadId`) and adds the Connectors through `addConnectorRange`, so D75's fit narrows a
  range more than two lanes wider than its out end. `settleNodes` then refines the Node.
  - **The uncut geometry is the template's** (`templateLinks`, extracted from *Apply template*).
    So a leg is always cut from the same shape, and *Apply template* on a Road ending in a Node that holds a
    generated Connector cuts its Links there again (`generatedCuts`, taken before the Links change): a re-apply changes no byte (RS12).
- **Readings taken:**
  - **Only generated arms in S4.5a.** §13.1 allowed an arm that is a Link without a Road. Its uncut
    shape is lost once it is cut, so a re-apply could not reproduce the cut; it is
    `FILL_NODE_ARM_NOT_GENERATED` until a need is shown.
  - **Signalised Nodes** are `FILL_NODE_SIGNALISED` until S4.5c. **Undetermined priority** is not
    checked yet: S4.5a writes no priority, so S4.5b brings that refusal with the areas.
  - **Listing:** a Node with detail, or with fewer than two arms, gives no row; every other Node
    gives one, with its refusal when it has one. A refused row cannot be ticked, and applying it
    throws the refusal.
- **Until S4.5b** the generated Connectors' crossings were automatic, as an undrawn junction's
  are: the detail ran, but nobody gave way at it (S4.5b, §15).
- **Editor:** *Fill defaults…* lists the Node with the Connector count, the source and what it
  clears; a refused row shows why and has no check box.
- **Rows:** RS11–RS13, RS16 and RS17 are in `tests/node_detail_tests.cpp` (`nodedetail`). RS14,
  RS15 and RS18 wait for S4.5b and S4.5c.

- **Amended by S5.2a (D327):** a leg keeps its current end away from the Node. A leg that
  `splitRoad` had split there is 0.1 m shorter than its template, and rebuilding it from the
  template stranded the split's Connector 0.1 m short of the end (`UNSUPPORTED_CONNECTOR_POSITION`
  at Run). Found by SA1's corridor, two F4 junctions joined by a split Road.

## 15. S4.5b as implemented (D324)

- **Ranks** (`movementRank`, `node_detail.*`): through is a turn to the other arm of the same class,
  major to major or minor to minor; otherwise the turn's sweep sign makes it a kerb or a far turn.
- **Major arms:** the Node's `major`, else `majorByRank` over each arm's link type (its first leg
  Link's, else its Road template's). At three arms or more, F4 is refused:
  - `FILL_NODE_PRIORITY_UNDETERMINED` when neither decides;
  - `FILL_NODE_MAJOR_ARMS` unless exactly two arms are major and there are at most four, the
    two-way stop shape Ch. 20's ranks are written for (a reading taken: §13.1 did not name it).
- **Areas:** for every automatic conflict between generated turns from different arms and of
  different ranks, `detail::authorCurrentConflict` makes the authored areas from one list (a merge taken whole).
  Then every new area between two such turns gets the priority by rank, with `default.json`'s gap
  time and headway through `setConflictControl`, and an area a minor arm yields in gets *Stop*
  (`setAreaControl`). An area of equal ranks that a merge takeover made kept the fallback's
  order; S4.5c orders every pair of a generated merge by rank and then drawing order (§16).
- **`generated`** on waiting lines, conflict areas, priority rules and Stop controls (schema 54), each
  cleared by its first edit (`settleGenerated`) and never copied. A bad value is
  `EDIT_GENERATED_INVALID`, which replaces S4.5a's `EDIT_CONNECTOR_INVALID` for every object.
- **The command refines the Node itself:** `generateNodeDetail` deletes `coarse` as it draws, so a
  caller outside `History` never holds both.
- **Derive back amended** (`deriveCoarse`): an arm is minor when each of its turns that meets a
  decided conflict yields in one; an arm with a turn that meets some and never yields is major.
  The earlier rule, "never yields", made both major arms minor, since a major far turn yields to the
  opposite through; a minor kerb turn also has priority over the other minor arm's through, so
  "has priority somewhere" does not identify a major arm either.
- **Editor:** *Fill defaults…* passes the resolved link types and the priority defaults.
- **Rows:** RS14 and RS18 are in `nodedetail`. RS14: every generated area decided and generated,
  rules at 3.0 s / 7.0 m, Stop only on minor arms' lines, a minor arm yielding to a major one, and
  deleting the detail deriving back the same major arms with no banned turn. RS18: the four-arm
  fixture over the shipped T-junction's catalogs runs at Micro (seed 42) with no refusal, its Node
  has a row for each of the 12 turns, vehicles and no issue. Rule 4 applies: a run, not a validated
  junction. RS15 waits for S4.5c.

## 16. S4.5c as implemented (D325)

S4.5 is complete with this slice.

- **Layout** (`signalLayout`, `turnGreen`, `node_detail.*`, pure): stages in their stored order,
  each followed by an intergreen of (cycle − Σ green) / stages. Signal times must lie on the run's
  time step (`INVALID_SIGNAL_TIMING` otherwise), so each intergreen is that share rounded down to the
  step and the last takes what remains (20/3 s on 0.1 s: 6.6, 6.6 and 6.8). A turn's green runs from
  its first stage's start to its last stage's end and wraps past the cycle's end where the run does.
- **Refusals,** after the cuts:
  - `FILL_NODE_SIGNAL_GRID`: the cycle or a green is off the step;
  - `FILL_NODE_INTERGREEN`: the shortest intergreen is below a group's 3 s amber;
  - `FILL_NODE_TURN_UNSERVED`: an allowed turn is in no stage (a reading taken: a group needs a
    green, and banning the turn is the user's call);
  - `FILL_NODE_STAGES_SPLIT`.

  `FILL_NODE_SIGNALISED` is gone. The major-arm refusals of §15 apply at a signal too.
- **The command:** one controller named after the Node (else its id), with the coarse cycle,
  offset 0 and group *k* for turn *k* (named "in -> out", amber 3 s). Each path of each turn's
  Connector gets a head at station 0 showing its turn's group. Both are `generated` (schema 54;
  a controller below 54 carrying the key is `EDIT_UNSUPPORTED_FIELD`). `settleGenerated` covers both,
  and a copied head never carries the flag.
- **Priority at a signal:** two turns decide only when green in a common stage, and nobody
  Stops. **Found while building:** a merge is taken over whole, so its areas join turns that may
  never be green together. Rank priorities on some of its pairs, mixed with the takeover's order on
  the rest, made `CONFLICT_PRIORITY_CYCLE`. Every pair of a generated merge is therefore ordered by
  one total order, rank and then drawing order, at priority Nodes as well. Stop still goes only
  where ranks differ and a minor arm yields.
- **Locale:** the refusals have English and Thai text, S4.5b's two included.
- **Rows** (`nodedetail`):
  - RS15: the controller, its 12 groups at [0, 40) or [45, 85), one head per path, control
    signalised, no Stop, and crossings only within a stage. Derive back gives the two stages; a turn
    carried across an intergreen derives back with that 6.6 s span as a stage of its own;
  - RS11 gains the signal refusals; RS16 the head and controller flags;
  - RS18 runs the signalised four-arm fixture at Micro (seed 42), signalised with 12 turns and
    vehicles.

  Rule 4 applies.
