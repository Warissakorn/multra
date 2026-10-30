# NEXT — what a session does first

The single live to-do, **open items only**: what shipped is in [PROGRESS](PROGRESS.md), the
[archive](archive/README.md) and the [decision record](decisions/RECORD.md); gates are in
[ROADMAP](ROADMAP.md); owner items only in [OWNER_SITTING](plans/OWNER_SITTING.md). Rewrite this file
each session ([AGENTS](../AGENTS.md#session-end)); never append a finished slice here (D319). The
previous full list is [archived](archive/NEXT-state-2026-10-23.md).

## Standing facts

- **Name:** MULTRA (D284, D287); only the file's `"format": "TrafficSim"` stays until a schema step
  reads both. Licence GPL-3.0-or-later (D285); a CLA comes before any outside contribution.
- **Local evidence is Linux only** (offscreen Qt; session versions in PROGRESS). Automated Windows
  CI is recorded separately; no owner Windows/high-DPI appearance claim. No session closes a milestone gate.
- **Books:** five owner copies (Sheffi, HCM 7, Ortúzar & Willumsen, Planning Handbook 4th ed.,
  Yperman); repositories, page rules, caveats and the chapter map by milestone are in
  [BOOKS](reference/BOOKS.md). Attach with `add_repo`, never vendor; cite the book, read the image
  before using a number. Checks of 2026-10-23: [book-copies-check](evidence/book-copies-check.md).
- **Before an engine slice:** `src/core/` files are at the 500-line limit; split along the
  responsibility a change touches first (D319). The owner deferred splitting `types.hpp` on its
  own: estimated 466 → 420 objects rebuilt after a runtime-state edit, 338 with `eventJson`,
  `checkpointJson` and `summaryJson` moved out of `project/json.hpp`. Do it with the first engine
  slice that edits `Vehicle` or `SimState`, and measure the rebuild before and after.
- **New dialogs** are free functions with their own file (as `behaviour_editor.cpp`), not new
  `EditorWindow` members, and one whose rows can grow uses `dialog_fit` (D318, D319).
- If `map-ui` fails in a parallel `check`, capture its output before re-running.
- **M0 is closed** by owner ruling (D369, [evidence](evidence/m0-plausibility.md)). Bare M0
  scenarios still hold amber as red (D147); a change that moves the M0 baseline reopens it.
- **Shipped projects (D337):** all four stay; [their README](../data/projects/README.md) names each
  one's purpose and builder. `four-leg-signalised` now carries a 300 s cool-down (schema 27): a test
  that needs the bare drawing clears it with `changeEvaluationPeriod(d, 0, std::nullopt, 0)`.

## M14 — the owner's current pick (D386, D387): junction templates and UTDF export

- Contracts [JUNCTION_TEMPLATES](reference/JUNCTION_TEMPLATES.md) and [UTDF_EXPORT](reference/UTDF_EXPORT.md);
  ROADMAP M14. D387: no builder tool; Street segments crossing make Nodes and a junction template generates
  the detail; transitions align per template; Road became Street segment first (D389).
  B30 answered as recommended (D388: JT-1a–JT-5a, UX-1a–UX-4a). SR is done (D389): Road is Street
  segment and schema 59 renamed the keys; Windows look is OWNER_SITTING E24.
- **U0 is prepared and waiting on the owner** ([OWNER_SITTING I](plans/OWNER_SITTING.md#i--utdf-probe-m14-u0)):
  - the sheet is [UTDF_PROBE](plans/UTDF_PROBE.md), and MULTRA's hand-written file is
    `tests/data/utdf/u0-handwritten.csv`;
  - UTDF_EXPORT §3 has the field names from one Synchro UTDF 8 file;
  - when the answers return, fix §3/§6 from them, add the golden file if the owner allows it, and
    write UT0;
  - no UTDF writer code before then.
- **J1 is done (D390):** the posted `speedLimit` is on the Link (schema 60), defaulted from the
  cross-section template; 58–59 approach speeds move to their Links. Micro does not read it yet (a
  slice of its own, D381); `freeSpeed` stays Macro's and Meso's. Owner look: OWNER_SITTING E25.
- **Current stop: J5 is implemented (D406, JT10 passing); await the owner's review.** Do not
  start J6 or J4T before approval. J4 is approved at 5ab3ea3. B34 is answered: taper entirely
  upstream, Transition-only selector, two symmetric installed templates and owned mixed directions;
  [JUNCTION_TEMPLATES §7](reference/JUNCTION_TEMPLATES.md) is the contract. Do not re-ask these choices.
  New owned transition structure uses schema 63; existing recipe fields keep their earlier floors.
  The fill dialog's generatedCommand mode and JT13 remain accepted; J3b is approved at a52476b.
- **B33 is answered (D405):** book J4T before U1: one protected three-stage signalised T
  preset, existing compass NEMA numbers with unused phases omitted, 2/6 at zero. Its own
  contract/rows and implementation review come in a separate slice; docs only during J5.
- **B32 is answered (D404):** fixed numbered rings/barriers, P4 4 then 8 with derived red
  waiting, entered allRed as clearance, exact cycle closure, rotated compass mapping and
  2/6 time-zero anchor. §8 has the timing construction; do not re-ask these choices. All site
  timings remain required. No timing default or writer is authorized.
- **B31 is answered (D399):** all-way-stop generation waits until A5 or U1 needs it. Keep the
  row listed but disabled with `FILL_NODE_ALL_WAY_STOP`; do not extend StopControl or consume
  schema 63 for all-way stop. Link.generated uses 62; J5 transition structure uses 63. Do not re-ask B31 in J4.
- **Boundaries to retain:** J4 requires complete site timing (D404); full-lane mismatch
  refuses with existing → required counts. An orthogonal T with a wide median can leave a
  touching through pair and stays `EDIT_CONNECTOR_GAP` (D400), with no invented cut length.
  Street segment Apply template over junction-generated Links is `JUNCTION_STREET_SEGMENT_DETAIL`;
  F4 detail plus a chosen recipe alone keeps Apply available (D402). After reopen without Undo,
  delete the junction's generated Links, Apply the Street segment template, then fill again.
  Deleting Links removes their attached objects. Fill the junction before placing stationed
  measurements. A changed base shape cannot silently move/resize the detached upstream Link.
  HCM's existing storage/derivation remains; recipe keys and Link-based Node inputs stay separate.
- **U0 checked for J4:** UTDF_PROBE §6 and OWNER_SITTING I still have no answers. J4 can only
  claim MULTRA contract checks; left-hand/phase mapping in Synchro/Vistro stays unverified and
  may change after U0. This effect was reported before code. Do not invent yellow/all-red or
  any timing default; do not start the UTDF writer. Template editor/Find crossings UI stays
  J6; Windows fill inspection is OWNER_SITTING E26, and signal inspection is E27.
- **Booked for J6, before U1 (D402, D403): one Command: change cross-section through a junction.**
  Change the cross-section and regenerate junction detail while preserving references in one
  Undo step. **Corridors are required:** a Link between two filled junctions is generated for
  both; today's delete-and-refill recovery also removes the neighbour's leg. Add a *Select this
  junction's generated Links* aid that identifies shared Links/affected neighbours, or make the
  Command corridor-aware and keep both ends consistent. Docs only in J4; no J6 code or selection
  aid is implemented yet. Retain the recovery's attached-object and shared-leg warnings.
- Later, each only after approval: J6 editor surfaces, J7 analysis panel/results
  (which may need two stops), then U1–U3 once U0's answers are in.
- Watch B29 (b) and (e): a turn key across levels and HCM storage change what `Node::junction`,
  `approachLayout` and UTDF's Lanes read.

## Object interchange follow-up (M1.23.1–M1.23.2, D365–D367)

- Owner selected native object-table CSV export over the schema queue. Packaged Windows
  acceptance is [OWNER_SITTING G1](plans/OWNER_SITTING.md#g--object-interchange); no owner or
  full M1.23 gate closes from Linux tests. Read [OBJECT_TABLE_CSV](reference/OBJECT_TABLE_CSV.md)
  for columns/units and nested JSON conventions before extending interchange. GeoJSON,
  imports/CRS mapping, recent-file/document workflow and measured rendering retain their scope.
- Packaged Windows PNG acceptance is OWNER_SITTING G2. Read
  [NETWORK_IMAGE](reference/NETWORK_IMAGE.md) before changing bounds or rendering. The
  remaining M1.23 formats/workflow/performance work still needs its own bounded slices.

## AI automation follow-up (D344–D350)

- Open automation work: an authoritative formal schema covering every optional/versioned
  field remains separate from D350 observed-shape discovery plus codec dry validation.
  Use [PROJECT_SCHEMA](reference/PROJECT_SCHEMA.md) for root presence rules, codec owners
  and save-version floors. Next: map project-owned Composition/type-share/interval rows,
  preserving catalog presence/ownership, contains-based optionals and feature-selected save
  floors 18/19 without inventing new read gates. Extend the composed partial schema only with
  codec parity cases; keep type references, unique positive weights, valid time windows and
  run-readiness in existing validators. Vehicle/behaviour catalogs and other optional-feature
  fields remain codec-owned until mapped.
  Never promote sample-derived hints to an authoritative contract.
  Explicit batch/comparison/Macro/Meso identities/exports remain separate from single-run APIs.
  Reuse existing validation,
  History and GUI run timer; no generic job/session subsystem is booked yet.
- Windows acceptance is [OWNER_SITTING H](plans/OWNER_SITTING.md#h--desktop-automation-on-windows)
  (H1 for D335/D336, H2 for D344–D350).

## Order of level work (D378)

Micro, Meso and Macro carry equal weight; open level work follows the study chain, Macro → Meso →
Micro ([PROBLEM](PROBLEM.md) §3). When the owner asks for an engine slice, the next one is
**M10.2 (trip generation)**, then M10.3–M10.4, M9, M11.5 and the M11 gate, M12. Each starts with
its contract. M13 is a Node method serving all three; A4 waits for the owner. OWNER_SITTING B26
(hybrid meso–micro; the user roles) is open.

**Booked by D379 (B27, on delegation), each starting with its contract when the owner asks:** one
posted speed on the Link (D380 confirmed it; the M13 approach `speedLimit` becomes derived; D381: the
field is optional, Micro reads it as a default and never a cap, and low site speeds are valid), per-lane
allowed modes, *Fill from model* for Node turn volumes, and M13 on coarse Nodes only if HCM 7 prints
a planning-level method (otherwise back to the owner).

## Road-model review follow-up (D382–D385)

Batches A–D are done (D382–D385). What is left waits on the owner:
- **On Windows:** OWNER_SITTING E24 (the Street segments tab's multi-row *Apply template* and the *Find
  crossings* warning at 100/150/200 %, Thai and English).
- **For the owner:** OWNER_SITTING B29 (generated Node size, one turn key across levels, explicit
  Street segment directions, partial HCM totals, HCM storage, CountSet).

## Next session — the owner's pick (S5 is complete, D327–D332)

- **Open from S5:** restoring the full Macro or Meso table on reopen would need a new owner
  choice: the owner chose the cut source only (D331). The stored source and freshness line remain
  the reopen contract.
- **Other picks:**
  - **Owner checks:** desktop automation (D335/D336) is OWNER_SITTING H1, the vehicle library and
    colours (D333/D334) C13, and the OSM real-study check (M1.28 MAP10) E11. A session prepares
    the packaged build and the sheet; it does not record a verdict.
- **Drawing level is not separation (D201 B3, booked 2026-10-16, not yet done):**
  `automatic_conflicts.cpp` still skips path pairs whose drawing `level` differs, against
  [M3_CONTRACT](reference/M3_CONTRACT.md) §1 ("Drawing level is not physical separation"); D201
  kept the contract and queued the code. Failure first: two crossing Links on different levels get
  an automatic area. `automatic_conflict_tests.cpp`'s `levels_apart_do_not_conflict...` asserts
  the old skip and changes with the fix. Decide in the slice's contract whether an S3.5 overpass is
  the object that separates them, and say so before changing generated Nodes.
- **Lateral, before any shipped behaviour carries its keys**
  ([lateral-run-audit](evidence/lateral-run-audit.md)): on `m2.6-study-template` a changer past its
  window straddles the line and jams both lanes (seed 45: 1908 → 1531 completed; clamps 1 → 33).
  LANE_CHANGE_DURATION §9's reopen condition is met; abandoning a change is the owner's call. The
  switched-on cost (+10–28 % wall) is not worth cutting until then.
- **Meso (M11, in through M11.4b):** the gate needs the benchmark: waiting on the owner's copy of
  Boyles et al. 2025 (D315), then the tolerance and rows. Priority at a Node waits on Brilon and Wu
  (2001) and a cited `t_ij`; HCM 7 Ch. 20's headways are a citable candidate (BOOKS §4). M11.5
  waits on M9, whose two-route fixture has a published candidate (Ortúzar Example 11.1).
- **Macro (S2):** B21, Sheffi Fig 5.1's capacities, for the owner. §19 (c), a procedure sequence,
  remains booked until M10.2–M10.4 provide its stages (D277).
- **M3.3.3b — the W99 contract before code** (ROADMAP M3.3, D177): read the following-model
  requirements in [DRIVING_BEHAVIOUR](reference/DRIVING_BEHAVIOUR.md), then write a sourced state,
  equation and parameter contract with hand-checkable acceptance cases. Keep the unvalidated
  marker (D13, D369) and the owner gates; this item does not authorize closing those gates.
- **Clamps:** 5 left on M2.6 (pull-out 1, late catch-up 4), the lane lab's 1 (D252) and source
  clearance (D108); `joinAnticipation` ships off (D255); D95 stays off.
- **Node detail:** the booked local performance target is met after D353: 16 unconnected Nodes
  0.609 s (Release, median of three), below 1.5 s, with byte-identical documents. No further
  optimization is booked. Keep `multra-node-detail-benchmark 16 output.json` as the regression
  measurement; re-profile before proposing more work.
  - `automaticConflicts` remains network-wide. Scoping to Node bounds or incremental updates
    would still need the owner's choice; this optimization changes neither scope nor output.
- **Analytical junction capacity (M13, D370):** contract [NODE_ANALYSIS](reference/NODE_ANALYSIS.md);
  A1 (D372) and A2 (D373, D374: shared lane groups, critical v/c, back of queue) implemented. Next
  A3 implemented (D377, [NODE_ANALYSIS_A3](reference/NODE_ANALYSIS_A3.md) §7). Open, for the owner: a permitted far
  turn in l+sl, lr or one-lane layouts (refused, narrows D375 1a) and Q1 for permitted lane groups. A4 only when asked.
- **Engine spread costs, not booked:** `closestVehicle` 8 %, per-tick `Vehicle` moves 6 % (D283).
- **Owner checks on Windows:** D318's dialogs are OWNER_SITTING F4, E1's tool glyphs and click rule
  (D306) E23, and paint E22. M2.8 has only owner checks left.
