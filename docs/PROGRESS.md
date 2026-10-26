# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## 2026-10-25 — Named Macro run definitions (D339)

- **Asked:** continue development; followed NEXT's booked named-run seam (§19 b, D277).
- **Changed:** optional schema-56 `macroRuns` stores ordered id/name/period/settings definitions.
  The editor manages the list in one Undo step, then selects one definition for the existing
  staged worker. CLI `--macro-run ID` uses the same definition; JSON/CSV identify it.
- **Boundary:** named execution uses explicit settings without editing the source document or
  global settings. Named and global settings share a parser that rejects oversized integers before
  narrowing (including large negatives that formerly wrapped to a legal cap). Existing unnamed
  runs/outputs and old project schemas stay unchanged. The Macro side file still holds the latest
  cut source per period, with original project provenance;
  result-table restore and a general procedure sequence are separate seams.
- **Verification (2026-10-26):** Linux, GCC 14.2 and Qt 6.8.2 offscreen: full desktop build and
  `check` pass, **234/234 CTest cases passed**. Five focused model cases and the new/existing Macro
  UI harnesses also pass independently. Fontconfig uses bundled Thai fonts with DejaVu Sans
  fallback. Windows was not tested locally; no milestone gate closed.
- **CI follow-up (2026-10-26):** [run 641](https://github.com/Warissakorn/multra/actions/runs/40248594439)
  passed all six jobs: Linux headless/sanitizer/release/desktop and Windows core/desktop. This is
  automated evidence for D339, not an owner appearance verdict.

## 2026-10-25 — Study-area cuts preserve single vehicle types (D338)

- **Asked:** continue development; followed NEXT's first open S5 items: single-type Meso gate
  flows and CLI coverage on a study-area project.
- **Changed:** Meso `type:` classes now retain their per-slice vehicle counts (entrance PCU divided
  by class PCU) and vehicle type through run-time gate flows/matrices into ordinary Micro inputs.
  Types sharing the same gates stay separate from each other and from composition classes.
- **Boundary:** no engine, loading, stored schema, UI or catalog-ownership change. External unused
  compositions stay unused, including with a reduced project-owned vehicle library. Unknown
  vehicle types refuse Run; unknown class prefixes retain their advisory.
- **Tests:** SA17 covers mixed classes, non-unit PCU, class grouping, exact input manifests,
  source/catalog preservation, seeded replay, side-file parity and stored-field rejection. SA18
  launches the real CLI for single, batch and comparison JSON/CSV exports with absent/fresh/stale
  side files and exact provenance hashes.
- **Verification:** Linux, GCC 14.2 and Qt 6.8.2 offscreen: full desktop build and `check` pass,
  **231/231 CTest cases passed**. Fontconfig uses bundled Thai fonts with DejaVu Sans fallback, as
  in the earlier desktop sessions. Six focused type-cut cases also pass in a separate harness.
  Windows untested; no milestone gate closed.

## 2026-10-25 — Shipped projects reviewed; four-leg gets its 300 s cool-down (D337)

- **Asked:** check the four shipped projects, then whether any is unneeded; the owner chose the
  recommendation (“ทำตามแนะนำ”): keep all four, give four-leg its cool-down, document the folder.
- **Review:** all four regenerate byte-identical from their `tools/` builders, have unique IDs,
  `nextId` above every ID, resolved references, and run deterministically (seed 42 twice) with
  none overloaded at seeds 42-51. No file is redundant: four-leg is the main regression fixture, the
  T-junction the only Yield/right-hand one, the M2.6 template the only routing-decision gate-study
  shape, the lane-change lab the D98 development bed (8 tests, C4).
- **Dropped owner decision found:** D201 B8 (2026-10-16) queued a 300 s cool-down for
  `four-leg-signalised`; it left NEXT by 2026-10-20 without being done. Now
  `fixture::shippedFourLeg()` adds it to the committed file only (schema 17 → 27); the M2.6 template
  and in-memory variants keep the bare drawing. Seeds 42-51 had about 52 vehicles still in the
  network at 900 s and every movement warned unfinished.
- **Tests:** the codec and no-cool-down rows that used the committed file as their bare baseline
  (queue counter schema 16, EP schema 22 and warm-up 0, CD1/CD6/CD7, TT8, L5) clear the cool-down
  first; EP and CD1 now pin the committed period. `road`/`roadcatalog` skip non-`.traffic.json`
  files so the new [`data/projects/README.md`](../data/projects/README.md) is not parsed.
- **Verification:** Linux, GCC 13.3: headless CTest 154/154 and `check` pass; desktop (Qt 6
  offscreen) CTest 230/230. Four-leg seeds 42-51: no movement unfinished, 504.7 vehicles completed
  of 504.7 generated, safety clamps 0. Windows untested. No milestone gate closed.

## 2026-10-25 — Review fixes: user-bound automation endpoint, gesture BUSY, car default (D336)

- **Asked:** apply the review of the automation branch (D335) and of PR #190 (D333) and open one PR.
- **Endpoint ownership:** a bare Unix name was `/tmp/<name>`, so another account could create it
  first and receive every request; the per-user lock excluded only this user's editors. Both
  ends now resolve `automationServerName`: on Unix a hashed socket in the owner-only runtime
  directory. On Windows `multra-control` refuses a pipe whose server process is another user
  (`UNTRUSTED_SERVER`).
- **BUSY during a gesture:** a guarded command arriving while a mouse button is held is refused,
  as under a modal dialog; a drag's release would otherwise commit over the patched document.
- **Default type:** catalogs sort by ID, so D333's bus became the default of a new Vehicle Input,
  the template of a new catalog type and a parking exit's placeholder. `preferredVehicleType`
  (car, else the first) is now the one rule, also used by the Behaviour Lab.
- **Code review follow-ups:** an open menu is `BUSY` too; a padded seed is stored canonically,
  not truncated by the 10-character field; the endpoint lock is resolved without Qt's test mode
  (`automationLockPath`), and the test host uses `useTestStorage()` with one fixed name; a patch
  never copies the embedded image; `commitChange` is the one post-edit path; History's reset
  generation, not per-site clears, makes the documentId; the Lab uses `preferredVehicleType`.
- **Verification:** see the PR; Linux only. The Windows refusal needs a second account.

## 2026-10-25 — Visible desktop control for AI agents (D335)

- **Asked:** operate the compiled Windows application through commands while the user
  watches its native window. Added opt-in local JSONL IPC (`--automation`) and the compiled
  `multra-control` client; no scripting runtime or separate model is required.
- Commands inspect or atomically patch the live project, Open/Save, Undo/Redo, control
  the visible simulation, select/frame objects, capture PNG and read/export results.
  Session/revision guards prevent stale edits; invalid patches preserve document, History
  and run state. Bounded stepping observes every fixed step and refreshes the canvas once.
- Added IPC/client tests, live-widget command tests and a separate-process test host built
  from the real desktop entry point with isolated Qt storage. Updated packaging and the
  [Windows PowerShell workflow](reference/DESKTOP_AUTOMATION.md).
- Shared protocol bounds keep client/server/capabilities aligned; unchanged embedded image
  buffers remain shared across History rather than being copied into each AI edit.
- **Verification:** Linux, GCC 14.2 and Qt 6.8.2 offscreen: full desktop build and `check`,
  **230/230 CTest cases pass**. The host's default Fontconfig tripped two UI font-metric
  assertions; selecting bundled `data/fonts` plus DejaVu fallback resolves both, followed by
  the full passing run. Editor font code and the original assertions were unchanged.
  The staged production desktop/control pair also runs from `/tmp`, advances 600 fixed steps,
  captures its Thai window and reports the shared request/step bounds correctly.
  Windows runtime and packaged dependency acceptance remain open; no milestone gate closes.

## 2026-10-25 — Shorter heavy-vehicle cab

- **Asked:** shorten the heavy-vehicle head to match the light vehicle. The owner confirmed
  the smaller grey rigid-truck from the displayed fleet as the reference. Both truck cabs cap at
  a nominal 2.04 m; the 12 m heavy cab was previously 2.88 m. The fixed load box extends
  forward to keep the full 12 m body and rear bumper, without changing traffic/axle data.
- A piecewise drawing transform applies to the body, glass, panels and lights together.
  Tests compare the cab, glazing and lights of the 8.5 m and 12 m trucks in physical units,
  alongside the existing bounds, glass picking, zoom and turning checks.
- **Verification:** Linux/Qt 6.8.2 offscreen: full desktop build and all 227 CTest cases
  pass after the cab change, including appearance, colour preferences, canvas LOD,
  run presentation and lane-change display. Equal-length cab comparison and updated
  gallery inspected; architecture, documentation and file-size checks pass.

## 2026-10-25 — Original motorcycle and editable mixed colours (D334)

- **Asked:** restore the original motorcycle and make vehicle colours editable/mixed.
  The owner explicitly chose the pointed D333 silhouette shown in the first seven-type gallery.
- **Built:** View → Vehicle colours, fixed or mixed palettes per type, colour picker,
  Add/Remove, standard palette and per-type reset. Preferences save atomically as local
  application display settings and load on restart; the editor and Behaviour Lab share them.
  Stock swatches live in `data/vehicle-colors.json`. Custom project vehicle IDs are supported.
- Colour selection hashes vehicle/type IDs; it consumes no engine random values and stays
  fixed through repaint, pause, zoom and seeking. Cancel/failed saves preserve the old state.
  Project revisions, schemas and traffic parameters are unchanged. See
  [colour preferences](reference/VEHICLE_POSE.md#vehicle-colour-preferences-d334).
- **Verification:** Linux, GCC 14.2, Qt 6.8.2 offscreen: full desktop build and all 227 CTest
  cases pass, including picker/persistence/atomicity, actual mixed-fleet colour stability,
  standard-palette validation and existing display/catalog suites. The seed-42 CLI example
  remains byte-identical. Architecture, documentation and file-size checks pass. Thai dialog
  and mixed-fleet previews inspected. No Windows/high-DPI acceptance claim.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
