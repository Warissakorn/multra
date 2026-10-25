# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---


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

## 2026-10-25 — Simplified motorcycle styling

- **Asked:** simplify the motorcycle. One flowing body, straight handlebar, single tank
  highlight, saddle and front light replace the faceted fairing and fine trim.
  Dark tyres and the small screen retain readable direction and vehicle identity.
- Existing physical bounds, connected picking and zoom detail thresholds remain covered.
- **Verification:** Linux/Qt 6.8.2 offscreen: rendering build and four display suites
  (appearance, canvas LOD, run presentation, lane-change display) pass. Close-up inspected;
  architecture, file-size and documentation checks pass.

## 2026-10-25 — Modern sport-bike styling

- **Asked:** make the motorcycle more modern. Compact angular fairing, tucked waist,
  swept grips, tapered screen, faceted tank and a two-part stepped saddle replace the
  rounded layout. Two separated swept LED-style strips identify the front.
- Body unions preserve solid wheel/fairing joints and the existing physical bounds.
  Checks cover both LED strips and centreline picking at the wheel overlaps.
- **Verification:** Linux/Qt 6.8.2 offscreen: rendering build and four display suites
  (appearance, canvas LOD, run presentation, lane-change display) pass. Production-painter
  close-up inspected; architecture, file-size and documentation checks pass.

## 2026-10-25 — Refined motorcycle appearance

- **Asked:** improve the motorcycle's looks. Rounded cowl, tank shoulders and tail replace
  the arrow-shaped body; a separate handlebar crosspiece defines its width. Dark tyre tread,
  grips, windscreen and saddle contrast with the highlighted tank and filler cap. The headlamp
  sits on the front cowl rather than the tyre tip.
- Physical dimensions and runtime behaviour retain the existing data. Checks cover connected
  body picking, dark tyres, and distinct tank/saddle detail alongside the existing LOD checks.
- **Verification:** Linux/Qt 6.8.2 offscreen: rendering build and all four targeted display
  suites pass (appearance, canvas LOD, run presentation, lane-change display). Production
  painter preview inspected at working and overview scales; repository checks pass.

## 2026-10-25 — Refined heavy-vehicle appearance

- **Asked:** a more attractive, realistic heavy vehicle. The shared rigid-truck archetype
  now has a rounded cab, wide windscreen, distinct raised cab roof and side glass,
  an exposed chassis neck, softened box corners, roof corrugations and a rear-door header.
  Both heavy-vehicle and rigid-truck select this archetype through existing appearance data.
- Dimensions, axle pose, engine parameters and picking retain the same rigid-body contract.
  Regression checks cover the solid chassis hit area and recessed cargo-roof detail.
- **Verification:** Linux/Qt 6.8.2 offscreen: rendering targets build; appearance, canvas LOD,
  run presentation and lane-change display suites pass. Production gallery inspected at two
  scales. Architecture, file-size and documentation checks pass.

## 2026-10-25 — More realistic van plan view

- **Asked:** refine the van's appearance. Its silhouette now has a short cab-over nose,
  parallel passenger-compartment sides and upright rear doors, independently of the pickup.
  A continuous raised roof, shallow bonnet, divided cab/sliding-door/rear-quarter windows
  and narrow tailgate glass replace the earlier car-like cabin layout.
- Physical dimensions, axles, traffic parameters and the existing display LOD are preserved.
  The appearance suite checks the distinct silhouette, window pillars, solid roof and rear glass.
- **Verification:** Linux/Qt 6.8.2 offscreen: desktop rendering targets build; appearance,
  canvas LOD, run presentation and lane-change display suites pass. File-size, documentation
  and architecture checks pass. Production-painter gallery inspected at two scales.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
