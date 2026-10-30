# Junction template editor and rebuild Commands (M14 J6)

J6's detailed contract, split from [JUNCTION_TEMPLATES](JUNCTION_TEMPLATES.md) §3–§5 before
its 500-line limit. D402/D403 book the corridor Command; D410 records the owner's B35 answers.
The layout and HCM results continue to come from the drawing, never the recipe.

## 1. Owner answers (B35, D410)

- The seam's explicit **Apply again…** lists the affected junction/detail before replacing
  edited detail in one Command and Undo. Ordinary Fill still refuses `JUNCTION_EDITED`.
  All paths refuse references they cannot preserve, naming the object; no attached data is dropped.
- The cross-section Command discovers the complete affected set through shared Links,
  lists the junctions/detail before Apply, and restores/regenerates every end in one candidate.
  One refusal leaves document, History, revision and saved state unchanged.
- Changing a template keeps every entered site value. Inapplicable pockets/phases remain
  visible as a draft for explicit correction. Fill stays disabled until valid. No site defaults.

## 2. Editor surfaces

- **Template editor:** project-owned catalog, captured from installed entries as the existing
  catalog editors do. A table per role edits far-to-kerb movement rows, pocket and pocket side,
  with geometric left/right names for the current driving side. Validation is §3's existing
  loader/Command validation. Editing a template changes the recipe, not the drawing.
- **Find crossings:** a visible Template column on Junction and Transition rows. Junction
  selection is optional, initially none; entries match the leg count. Transition selection
  remains required. Overpass disables/ignores the chooser. Confirm creates Node plus recipe
  in one Undo, with no site values invented; Cancel changes nothing.
- **Nodes → Junction template…:** edit template, major arms, optional per-leg lane rows,
  pocket storage/taper and optional NEMA timing in one Undo. Empty input is missing, not zero;
  zero yellow/all-red is an entered value. Preserve incompatible values when switching templates.
  **B36 answered (D412):** site-only edits on the same template preserve the current
  `Node.generated`; changing template, Detach and polygon edits clear it as in D393.
  Never restore a cleared flag. Major/lane-override choices remain structural edits (D393).
  Directions are edited independently and clear ownership only when actually changed.
  The form stages all edits, lists Fill's missing/inapplicable fields, and saves a partial draft
  without applying it. Per-leg values and NEMA timing use separate scrolling dialogs; explicit
  clear/remove actions are required to discard entered values. New site fields open empty;
  existing entered values remain.
- **Seam checks:** `SEAM_JUNCTION_DIFFERS` is advisory at every level, at `network.nodes[i]`.
  Apply again shows the replacement list; Cancel changes nothing. Detach drops only the recipe,
  keeps coarse/drawing, and removes the advisory in one Undo. The explicit action is distinct
  from ordinary Fill's refusal after an edit (§1).
- **Change cross-section through junctions:** stage the selected Street segment's cross-section
  id/overrides, then show the complete affected shared-Link component before one Apply.
  The ordinary Apply template guard remains. The combined action is not delete-and-refill.
  Its preview shows the Node list and unique object count, with an expandable full id list;
  a three-Node / 446-object preview does not open as hundreds of visible rows (D414).

## 3. Pure comparison and private plans

`junctionSeamIssues(network, junctionTemplates, linkTypes, controllers, timeStep)` is pure,
in `src/model/demand/junction_seams.*`. No detail means no seam: an incomplete unfilled
recipe belongs in Fill's missing-piece list. With detail, compare refined objects, lane/movement
rows, pocket side/storage/taper, transition taper/alignment and signal timing/NEMA bindings.
The passed time step is the document's existing run step, not a timing default.

`junctionReapplyPreview` and `junctionCrossSectionPreview` return affected Node/object ids and
named issues from per-call candidate documents. They publish nothing. Their paired Commands
run in `History::execute(..., true)` and validate the entire candidate before publication.
No applied layout, provenance receipt or cross-edit cache is stored.

For a cross-section change, find the shared-Link closure before mutation, restore every
affected junction, apply the staged section(s), then regenerate every affected junction.
Generated detached pocket prefixes resolve through unique continuity Connectors, stopping at
any Node-owned Connector (D391). Such associations exist only in the private plan, not the file.
The special restoration mode permits the requested cross-section change; ordinary re-apply
keeps its template/base-match and edited-detail guards. A hand-edited neighbour refuses the
whole corridor Command. Existing stationed/reference guards remain and preserve retained data.

**J6a correction (D415, supersedes D413).** Three outgoing lanes are supported by ordinary
Fill and corridor rebuild. The former failure was numerical: `matchedStation` could map an
exact Connector endpoint one rounding unit beyond the runtime lane path's length. This also
affects a translated single junction; a single junction at the origin was the passing control.
Exact endpoints now map directly to zero/full target length, with interior conversion unchanged.
The D413 refusal and locale strings are removed. Whole-document validation remains unchanged;
no authored extent is clamped or tolerance added to the core validator.

## 4. Acceptance and implementation status

- **JT11, Commands/comparison:** refined Connector gives the advisory on both driving sides;
  ordinary Fill refuses; explicit preview lists it, Apply again and Detach each fix the seam
  with complete JSON Undo/Redo. A measured generated signal head still refuses with its counter.
  A recipe timing or transition taper change differs from the unchanged drawing. **Passing.**
  Changing only the template id preserves all incompatible entered site values; Fill stays
  disabled and save/reopen plus Undo/Redo retain the draft. Catalog changes, a three-leg pocket,
  missing Stops and uncovered automatic conflicts are compared using the existing priority rules.
- **JT11, crossing UI:** optional blank Junction chooser, matching templates only, one Undo,
  no site defaults; Transition's required selector and Cancel stay. Driven on both sides. **Passing.**
- **JT11, seam UI:** replacement preview/Cancel, Apply again and Detach with one Undo on both
  driving sides; screen fit. **Passing.** Readiness uses the same pure advisory at all three levels.
- **JT16, corridor Command:** a changed shared segment rebuilds both filled junctions, including
  pockets at both ends, on both driving sides. Retained input/measurement references survive
  reopen and rebuild; a manually edited neighbour refuses atomically. Three exit lanes now
  rebuild and validate on both sides (D415); incoming mismatch keeps `JUNCTION_LANE_COUNT`.
  **Passing.**
- **JT16, corridor UI:** staged override with no invented values; affected set includes both
  Nodes, Cancel is unchanged, Apply and one Undo restore the complete document. Both sides,
  screen fit and revision-zero project reopen. **Passing.**
- **JT11, catalog UI:** Add from selected/Edit, far-to-kerb role rows with geometric left/right
  names, invalid row/through-pocket refusal, project-owned capture and exact Undo/Cancel/reopen.
  Accepted movement edits and mixed transition directions use the existing loader/schema floors.
  **Passing**, both driving sides and English/Thai (`tests/junction_forms_ui_tests.cpp`).
- **JT11, Node UI:** template/draft preservation and explicit correction; major, lane overrides,
  independent compass directions, per-pocket storage/taper and NEMA cycle/phase fields.
  Missing fields stay absent, entered yellow/all-red zero survives, Fill refuses incomplete
  drafts. Find crossings → F3 → taper form → Fill enlarges the generated polygon, with no
  manual polygon edit. **Passing**, both driving sides/locales; B36 never resurrects ownership.
- **JT16, chain:** the affected set reaches all three junctions through two shared Links,
  rebuilds every end and passes save/reopen/full JSON Undo/Redo on both sides. **Passing.**

J6 was approved at 3ec3d39/09b0689. J6a awaits review: JT17 covers exact endpoint conversion,
single-junction controls at zero and x=200, and ordinary History Fill/rebuild of the three-Node
corridor on both sides, with save/reopen and one Undo/Redo per Command. **Passing.**
Native `junctioneditor` has 27 passing cases;
junction-ui/catalog-ui/recipe-ui and transition-ui cover the surfaces (full-run evidence in PROGRESS).

Headless/Windows evidence is reported separately; Linux offscreen tests do not close the owner look.
No J4T, UTDF writer, HCM method or posted-speed cap is part of J6.
