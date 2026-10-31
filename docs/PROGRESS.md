# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


## 2026-10-31 — A9 native expected-demand preview checkpoint (D419)

- **Owner:** J7 fe827c8 approved: real CLI/editor JSON/CSV parity for saved and unsaved
  Save-representation snapshots; shipped projects/node analysis unchanged except buildCommit.
  M14 junction slices are complete; U0 remains unanswered and no writer starts.
- **B40:** asked six contract choices before code. Owner chose expected Micro demand, explicit
  HV types, whole-set replacement/provenance cleared by later manual demand edits, and named
  unresolved-demand refusals (1/4/5/6). T/PHF and the comparison action (2/3) remain unanswered;
  their recommendations are not assumed. Schema 64 is booked for provenance, not consumed.
- **Independent implementation:** read-only expected-demand preview captures/parses the same
  bytes, reuses the compiler, integrates requested input intervals and maps complete runtime
  traversals to derived turns. Use reference-to-lane stations and exact compiler origins for
  entry-lane stubs; never multiply a turn by possible lane paths. Explicit HV weighting, zero
  rows/retained optional %HV and source revision/manifest; no sampled traffic or achieved counts.
  Refuse partial/unresolved traversals and affected potential parking releases, including
  lot-to-lot sources. A provably empty lot without gate demand stays harmless.
- **Tests:** NA24's direct interval/lane/stub rows failed against the empty interface on both
  sides before implementation. Extended guards exposed a parking fixture without its required
  gate; fixed the forcing setup. The empty-lot control then failed on an overbroad refusal,
  before its guard fix. Eight new native cases cover those paths, clock-placed OD/compositions,
  scheduled decision intervals, invalid requests/HV types, partial routes and runtime choices.
  Native nodeanalysis now passes 32/32, including existing method rows.
- **Unfinished:** no stored field/codec/version, Fill dialog/Command, T/PHF policy or Micro
  comparison. Both-locale UI rows and NA26–NA30 are still planned, not claimed. E31 records the
  cheap drawn-approach clear inconsistency for the next dialog touch; J7 approval stays intact.
- **Verification:** desktop build, CTest 258/258 (280.29 s) and check 258/258 (283.04 s) pass.
  A separate non-PCH header compile caught a missing complete Json include; fixed it, then
  passed standalone compilation, another desktop build and native nodeanalysis 32/32.
  Architecture/file-size/docs guards pass. Linux offscreen only, including existing JT12
  en/th cases; this is not a claim of the still-unimplemented new UI rows.
  The configured four-leg fixture and four shipped projects retain project bytes, and their
  before/after --node-analysis JSON bytes differ only in buildCommit. No Windows/manual
  verdict; headless is unconfigured. NEXT names the two pending answers and exact remaining work.
  B39 and the oldest PROGRESS entry moved whole to indexed archives; ten live entries retained.

## 2026-10-31 — J7: Node analysis form, snapshot and Results (D418)

- **Owner:** J4T approved, including natural polygons, both entry paths/sides, 200/90/60/40 m
  corridors, correct short-pocket refusal and unchanged shipped/analysis bytes. J7 is A9 first
  half only. B39 answered: current snapshot, exact saved bytes when equal; otherwise exact Save
  bytes labelled unsaved at Analyse, with CLI parity after saving the unchanged snapshot.
- **Implemented:** staged compass L/T/R turn volumes/%HV, Node PHF/period/CBD/sourced saturation
  and approach adjustments; missing differs from zero. Existing codec checks shape/sign, HCM
  checks its ranges on Analyse. Stale/extra rows are preserved and explicitly clearable.
  setNodeAnalysis is one Command/Undo, Cancel/no-op unchanged. No lane/timing copy or new schema.
- **Results:** shared project report/metadata/marking for CLI and GUI; synchronous calculation
  over exactly recorded/parsed bytes. Separate group, approach, intersection, defaults/source
  and localized refusal tables; unavailable figures blank. Partial totals retain D383's existing
  warning. Source revision, captured-snapshot note, stale/Undo freshness, generation reset and
  marked CSV/Copy; protect active project/recovery from export. No Window-header growth.
- **Tests first:** initial fixture incorrectly assumed the template switch kept generated;
  corrected the forcing assert before proving missing actions on all four side/locale pairs.
  Extended form tests then exposed the missing source requirement for a saturation override;
  fixed in UI without tightening older-file decoding. All 8 JT12 form/result cases pass.
  The 28 actual CLI subprocess runs match JSON/CSV for non-canonical saved bytes, unsaved-then-saved
  snapshots, external-file changes, method/queue refusals, multiple/empty configured Nodes;
  plus defaults/zero, Cancel, exact Undo/reopen, explicit stale-row clearing and export guards.
- **Verification:** the first full CTest run was 253/258: the old seven-tab expectation and
  four layout cases failed. Updated the tab inventory and made the hidden page ignore minimum
  size, with reflow labels; retained canvas/layout thresholds. Those six targeted suites then
  passed (25.31 s), including JT12. Desktop build and CTest **258/258** (290.45 s, parallel 5)
  pass; check **258/258** (297.74 s) also passes, with architecture/file-size/docs guards.
  The one-Node before/final CLI bytes match except buildCommit.
  The first build exhausted local
  disk on debug executables; stripped debug sections and used a local-only strip-debug linker
  cache flag, retaining debug objects and source/FP flags. No tracked build flag changed.
  Linux offscreen only; headless unconfigured, Windows/manual UI remains E31. U0 unanswered;
  no UTDF writer, Fill from model, timing proposal or Micro side-by-side.
- **Docs:** editor contract/index, §9/JT12, Node-analysis/schema, D418/index, B39/E31, module maps,
  ROADMAP/NEXT updated. Answered B37/B38 archived whole to keep OWNER_SITTING under 500 lines;
  oldest PROGRESS entry archived whole, ten live entries retained.

## 2026-10-30 — J4T: real Find crossings Fill and explicit polygon preview (D417)

- **Owner B38:** extend the chosen fit policy to every three-/four-leg junction template.
  Generated polygons fit automatically; a false flag uses explicit previewed Fit polygon +
  Fill, one Undo, never restoring true. First template choice keeps current ownership through
  the crossing column or later Node form; switching template/Detach/polygon edits still clear it.
- **Implemented:** share D407's original/cut/Connector surface hull and widest-lane padding
  with transitions, retaining new-Link-leg and neighbour-overlap guards. Named fit-required
  recovery opens old/proposed outlines; Cancel is unchanged. Preview and Command share a
  private plan and retain edited-detail/reference refusals. Remaining NODE_CONNECTOR_CROSSES
  names both Node and each crossing Connector. No field/schema, timing default or F4 change.
- **Tests first:** original JT20 fails three cases; the all-template/first-choice/diagnostic
  extension has 11 real failures after correcting a no-op setup assert and rank-aware seams.
  The explicit-action Qt row also fails before the button/dialog exists. Normal T fixtures now
  keep the natural polygon; deliberately roomy pocket/reference cases remain explicit.
- **Coverage:** junctionsignals 40/40 (21 JT20), junctionfill 25/25 including frozen JT13,
  junctiontemplates 19/19, junctioneditor 27/27 and transitions 14/14. Natural shared/pocket
  four-leg and signalised T run through both entry paths on both sides; explicit fit covers
  both T controls/four stems and four-leg pockets, guards/refinement, exact Undo and reopen.
  Qt drives later first choice → Fill for all three families, explicit preview Cancel/Apply,
  required phases and nested site forms in English/Thai on both sides. The authored transition
  fixture now declares its polygon non-generated instead of relying on old first-choice clearing.
- **Verification:** desktop build 103 tasks, CTest **257/257** (283.98 s, parallel 5), targeted
  suites 6/6 (30.70 s). One-Node CLI raw bytes match before/after B38 and the pre-J4T baseline
  except only root buildCommit. Check **257/257** (281.54 s) passes. Linux offscreen only; headless is
  unconfigured, Windows/manual appearance remains E30. U0 has no answers; no J7/writer started.
- **Docs:** §5/JT20, signal/editor/schema/readiness contracts, D417/index, B38/E30 and module
  maps updated; old out-of-repo evidence paths removed. Oldest PROGRESS entry archived whole.

## 2026-10-30 — J4T core checkpoint; natural polygon choice pending (D416, B38)

- **Owner:** J6/J6a approved. B33 fixes three stages/compass numbering; B37 selected two full
  incoming lanes per arm (F|T, T|K, F|K), existing P2 + legs=3, and a measuring-copy stem
  extension for both signalised and two-way-stop T templates. Ordinary F4 remains unchanged.
- **Implemented:** one installed protected T; only major through 2/6, physical major far 1/5,
  existing minor 4/8. Both numbered rings remain, including a one-phase ring. No timing
  defaults: cycle/green/yellow/allRed are entered; clearance/waiting stay separate and cycle
  closes exactly. Required numbers derive before timing is complete; Fill, form and seams
  share that set. No new enum, stored field, schema or HCM procedure.
- **Cuts:** extend only the full-width measuring stem to the far major painted boundary,
  then use the existing overlap cuts. Published geometry/stations are on the original Links,
  with reference/outside/refine guards and one Undo. D400 is superseded for templates only.
- **Tests first:** seven new signal/T rows failed on the old catalog/mapping/cuts, while all
  nine JT9 cases passed; the former orthogonal-T refusal row failed on its new success assert.
  UI digit extraction was corrected after its first assert included the explanatory suffix;
  re-running the original form proves the missing four-phase subset before restoring the fix.
  A catalog fixture now explicitly selects a four-leg entry rather than assuming it is first.
- **Coverage:** both sides/four compass stems, literal U0 numbers, single-ring waiting,
  missing/unused/grid/cycle/protected-lane guards, physical opposition versus labels, manual
  controller/checker cases, pocket drafts/re-apply, retained/refused references, full Undo/reopen,
  HCM's six movement greens and Micro seed 42. Node form is driven in English/Thai.
- **Docs:** signal contract split before the root page's 500-line limit; JT18/JT19, D416/index,
  schema/readiness/module maps, B37/E30, ROADMAP and NEXT updated; oldest whole entry archived.
- **Final workflow finding (B38):** removing the fixture's authored 60 m square exposes
  NODE_CONNECTOR_CROSSES with the actual 14.5 m-radius Find crossings polygon on both sides.
  The T core is committed at b7be7c94 but the slice is not complete. Template/major selection
  can already clear generated, so another polygon policy is an owner choice. Asked before
  implementing growth; JT20 is pending. Preserve the failing patch and original/candidate
  JSON in the evidence directory; restore the roomy tests to green while awaiting B38.
- **Verification:** desktop build and CTest **257/257** (279.81 s, parallel 5) pass; native
  junctionsignals **19/19**, junctionfill **25/25**, junctiontemplates **19/19**, junctioneditor
  **27/27** and both-locale Qt forms pass. CLI raw bytes match except buildCommit. Check
  **257/257** (280.92 s) passes. U0 has no answers; no writer/interchange claim or J7 work. Linux only; headless
  unconfigured, Windows/manual UI not run (E30).
  After restoring the core fixtures, the checkpoint desktop build and targeted signal/recipe
  tests pass **2/2** (7.74 s); docs/file-size/diff guards pass. B38 remains unanswered.

## 2026-10-30 — J6a: exact endpoints fix Fill and corridor exit lanes (D415)

- **Owner:** J6 at 3ec3d39/09b0689 is approved. Diagnose ordinary Fill's pre-existing
  conflict-range failure before J4T; stop for this slice's review, no UTDF writer.
- **Cause:** matchedStation's terminal subtraction/interpolation can return a runtime exit
  one rounding unit past the path: 22.315802608466722 versus 22.31580260846672 m.
  The three-junction repro and translated single junction fail on both sides; the origin
  single-junction control passes. This is numerical conversion, not a corridor/geometry limit.
- **Fixed:** exact endpoint returns in the shared converter; interior interpolation and
  existing outside clamping stay. Ordinary History Fill and atomic corridor rebuild succeed
  with three exit lanes, including the former road-32 fixture. Remove D413's temporary code
  translation and locale strings. Core/whole-document validators and authored extents stay;
  no new field/schema, timing default, conflict algorithm or ownership rule.
- **Tests first:** JT17's endpoint/translated/corridor rows and rewritten JT16 success rows
  failed before the correction (18 passed, 9 failed); both origin controls already passed.
  Native junctioneditor is now **27/27**, including nine JT17 cases, both driving sides,
  three-junction closure, preview parity, save/reopen and exact JSON Undo/Redo per Command.
- **Docs:** supersede D413 with D415/index, update §5/JT16/JT17, editor/M3 station contracts,
  approved J6/current J6a queue, E29 and ROADMAP; archive the oldest entry whole, keeping ten.
- **Verification:** desktop build, CTest **257/257** (280.06 s, parallel 5), and check
  **257/257** (277.28 s) pass, including frozen JT13, docs/file-size/architecture guards.
  Node-analysis CLI raw bytes match except buildCommit. Linux GCC 14.2.0/Qt 6.8.2 offscreen;
  headless unconfigured, Windows/manual UI not run. Evidence: `/workspace/j6a-review/`.

## 2026-10-30 — J6 forms, B36 and complete review rows (D412, D414)

- **Finished:** Nodes → Junction templates… stages structural role rows, geometric turn labels,
  project-owned capture, Add/Edit and loader validation; Junction template… stages major/lane
  overrides, independent directions, per-pocket storage/taper and NEMA cycle/phase numbers.
  Each accepted catalog/Node edit is one Command/Undo; nested dialogs stage only. Cancel changes nothing. Existing catalog/schema
  rules stay, including 63 when the captured whole catalog contains transitions.
- **Drafts:** switching templates retains every entered site value, lists inapplicable/missing
  items and keeps Fill disabled. Explicit clear/remove actions correct the draft; empty is
  missing and entered yellow/allRed zero survives. No timing or length default.
- **B36 implemented:** the existing setter retains current Node.generated for same-template
  site numbers. Template/Detach, major/lane structure and polygon edits clear it; it never
  resurrects ownership. The form calls the independent direction setter only when changed.
  Find crossings → F3 → taper form → Fill enlarges the still-generated polygon in one Undo.
- **Preview/seams:** affected Nodes and unique object counts replace hundreds of visible rows;
  all ids remain expandable before Apply. Complete three-junction closure/shared pockets and
  three-leg roles pass. Pure comparison detects missing Stops/priority and uncovered automatic
  conflicts through the existing generation rules; no runtime/HCM priority changed.
- **Tests first:** B36 and missing-control native rows, both missing form actions and compact
  preview assertions failed before implementation. Additional rows cover explicit draft
  correction, actual catalog movement/mixed-transition edits, schema/JSON reopen and full Undo.
  Compile/setup errors are not product reds: Qt's slots macro required a different local name;
  the three-Node fixture now iterates copied Node ids as generation replaces its document.
- **Docs:** contracts/JT11/JT16, D414/index, schemas/readiness/module maps, B36/E29 and NEXT
  updated; oldest whole entry archived, retaining ten. J6 awaits owner review, no J4T/writer.
- **Verification:** targeted native **18/18** and junction-ui/catalog-ui/recipe-ui pass;
  forms run on both driving sides and English/Thai, crossing/seam/corridor on both sides.
  Desktop build, CTest **257/257** (251.60 s, parallel 5) and check **257/257** (248.32 s)
  pass, including architecture/file-size/docs guards and frozen JT13. CLI node-analysis matches
  except buildCommit. Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless unconfigured and
  Windows/manual UI not run (E29). Captures/logs: `/workspace/j6-forms/`.

## 2026-10-30 — J6 review: actionable corridor conflict refusal (D412, D413)

- **Owner:** carry-over 85329d1 accepted; continue J6. B36 keeps the current generated flag for
  same-template site values, clears it for another template/Detach/polygon, never restores it.
  The remaining setter/forms implement that answer after this deliberately small review commit.
- **Fixed:** road-32's three outgoing lanes now refuse with JUNCTION_REBUILD_CONFLICT at the
  staged Street segment and Thai/English recovery: keep the outgoing lane count or choose
  another section/layout. Only final core conflict-range indices are translated; other issues
  and all validation/reference guards remain. No invalid geometry is published or clamped.
- **Tests first:** both driving sides fail on the missing named refusal before implementation.
  The fresh fixture fills with matching seams but whole-document validation also finds a
  conflict range; matching a recipe is not proof of runtime validity. Initial exact coordinate
  equality in the test was corrected to a numeric tolerance before claiming the product red.
  Preview/Apply preserve complete JSON, saved state and Undo/Redo; incoming three lanes retain
  JUNCTION_LANE_COUNT. The existing successful corridor cases remain.
- **Docs:** B36/D412, D413/index, JT16/known limitation and NEXT updated; oldest entry archived
  whole, keeping ten. Main J6 forms and the compact affected-set preview remain authorized.
- **Verified:** desktop build, native junctioneditor **13/13**, CTest **255/255** (247.76 s,
  parallel 5) and check **255/255** (246.21 s), including docs/file-size/architecture guards.
  CLI node-analysis matches except buildCommit. Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless
  unconfigured, Windows/manual UI not run. No final J6 approval, J4T or UTDF writer work.

## 2026-10-30 — J6 command/seam/corridor checkpoint (D410, D411)

- **B35 answered:** all recommendations selected: explicit listed seam replacement with
  reference guards, automatic complete shared-Link closure, preserve incompatible site drafts.
- **Implemented:** private previews/atomic explicit re-apply and corridor cross-section Commands;
  restore both ends before F3/regeneration, including pockets. Preserve retained measurements/
  inputs, refuse edited neighbours or measured generated heads atomically, and keep ordinary
  Fill/Apply guards. A unique generated pocket-prefix query stops at Nodes; no association is saved.
- **Seams:** pure lane/movement/pocket/transition/control/timing comparison; the same advisory
  in Micro/Meso/Macro Readiness. Changed major control is detected even when lane rows match.
  Template-id changes keep all entered site fields as a draft, with disabled Fill and full Undo.
- **UI:** optional matching Junction template column; explicit seam replacement list, Cancel,
  Apply again/Detach; staged corridor overrides and complete affected-set preview in one Undo.
  Linux Qt drives both sides. Reopening a revision-zero document now invalidates the touched
  object-table caches; otherwise the valid corridor file's Street segment table stayed empty.
- **Tests first:** initial JT11/JT16 cases failed, then targeted timing/control/readiness,
  crossing and Qt preview cases failed before their fixes. Positive pocket corridors required
  resolving the detached out-leg; an input fixture's mutually exclusive route target and a
  nested-modal test's lifetime were corrected after forcing assertions identified setup errors.
- **Checkpoint:** template catalog/Node recipe forms are not complete. B36 asks the same-template
  site-edit generated flag rule; no answer is assumed. NEXT names the remaining free-function
  forms and tests. Pause at the context boundary, not final J6 approval. No J4T/UTDF writer.
- **Docs:** J6 detail split before the 500-line limit; B35/B36, D410/D411/index, E29, schemas,
  readiness and module maps updated. Archive the oldest whole entry, retaining ten.
- **Verified:** desktop build, native `junctioneditor` **11/11**, Qt crossing/seam/corridor
  rows on both driving sides, CTest **255/255** (247.09 s, parallel 5) and `check` **255/255**
  (245.20 s) pass, including architecture/file-size/docs guards. CLI node-analysis matches
  except `buildCommit`. Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless unconfigured;
  Windows/manual UI not run (E29). Logs and comparison: `/workspace/j6-main/`.

## 2026-10-30 — J6 first carry-over: overlap preview (D409)

- **Owner:** J5 d5e2bbe approved; start J6 including the D402/D403 corridor Command. First
  commit must make a transition's neighbour-overlap refusal visible before Apply.
- **Implemented:** after containTransition, compare the private polygon with the other Nodes
  using the existing overlap predicate. NODE_OVERLAP names each neighbour, disabling the Fill
  offer; the whole-document validator remains a backstop. Add Thai/English text for the code.
- **Tests first:** the new JT10 row failed on the enabled offer after proving that the original
  polygons were disjoint and the planned enlargement overlaps, on both driving sides. All
  14 native cases now pass, including unchanged preview/History and direct generation refusing
  before publishing a document. No new field/schema or geometry policy; frozen JT13 untouched.
- **Main J6:** re-read §3–§5/§10/JT11, D402/D403 and the seam/ownership contracts. Asked B35's
  three choices before main code: Apply-again versus edited-detail refusal, the affected
  corridor set, and incompatible entered site values. Recommendations are not answers; await
  the owner while finishing the independent carry-over. No J4T or UTDF writer work.
- **Docs:** §7/JT10, J6 status, E28, B35, D409/index and NEXT updated. Archived the oldest
  whole PROGRESS entry, retaining ten. Main J6 remains pending the B35 answers.
- **Verified:** desktop build, JT10 **14/14**, Qt transition selector on both driving sides,
  CTest **253/253** (253.57 s, parallel 5) and `check` **253/253** (253.16 s) pass, including
  architecture/file-size/docs guards. CLI node-analysis matches except `buildCommit`.
  Linux GCC 14.2.0/Qt 6.8.2 offscreen; headless unconfigured; Windows/manual UI not run (E28).

## 2026-10-30 — J5 review: refuse polygon captures (D408)

- **Fixed:** after generated transition polygon enlargement, every resulting Link leg must
  already have been a leg, with the same entry/exit direction. New legs refuse with
  `TRANSITION_POLYGON_CAPTURES` at each captured Link. Thai/English recovery says to shorten
  the taper or draw the Node polygon by hand. Existing missing-arm and NODE_OVERLAP guards stay.
- **Tests first:** JT10 failed on the owner's real side-road setup before the guard. The
  forcing asserts establish original 2 Street segment / 4 Link legs and prospective 3 / 6
  membership before checking the refusal; 60 m and both driving sides are covered, with the
  side road mirrored for right-hand traffic. The y=25 control stays outside and fills with
  one complete JSON Undo/Redo. A neighbouring Node still refuses NODE_OVERLAP.
- **Atomicity:** capture refusal preserves the complete document, revision, saved state and
  Undo/Redo history. Planning/preview publish nothing. No stored field/schema, taper/envelope
  geometry policy, core, HCM, J6/J4T or UTDF writer changes; frozen JT13 files remain untouched.
- **Docs:** §7/JT10, Street segment workflow, command map, E28, D408/index and NEXT updated.
  Archived the oldest whole PROGRESS entry, retaining ten. Stop for J5 review.
- **Verified:** desktop build, JT10 **13/13**, Qt transition selector on both driving sides,
  CTest **253/253** (238.25 s, parallel 5) and `check` **253/253** (234.08 s) pass, including
  architecture, file-size and documentation guards. CLI node-analysis matches except
  `buildCommit`. Linux GCC 14.2.0/Qt 6.8.2 offscreen only; headless is not configured, and
  Windows/manual UI driving were not run (E28).
