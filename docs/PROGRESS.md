# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-10-23 — UI suites isolate their recovery drafts (D320)

- **The problem:** a crashed or killed UI run left its recovery draft behind. The next local
  run of that suite then waited at the startup recovery dialog until ctest's 60 s timeout.
  Eleven suites had no Qt test mode, so they wrote drafts into the user's own data folder.
- **The fix:** all 51 `EditorWindow` suites call `useTestStorage()`. It enables test mode and
  removes drafts whose lock nobody holds, keeping a running instance's draft.
- **Rows:** `test-storage-ui` fails without the cleanup, and fails at the window (not a hang)
  without the file checks.
- **Results:** a planted crash draft no longer blocks `behaviour-library-ui`. Two full runs pass
  219/219; the eleven suites wrote nothing to the user's folder.

## 2026-10-23 — Optimization review: UX, build, docs, engine (D319)

- **Measured first** (Linux; `.optimize/` ledger, not committed):
  - UX: a `QDialog::exec` shim over the 61 UI suites, in English and Thai;
  - engine: Release `multra-cli`, seeds 42–51, and callgrind;
  - build: Debug rebuilds after touching one file;
  - docs: the words a session reads before it starts.
- **Done:**
  - the seam dialog's *Use Node* button lost 8 px to the table's item padding; the column gets
    it back, the only clipped control measured;
  - NEXT cut to open items (1113 → 469 words);
  - D319 records the accepted rules.
- **Found on the way, fixed:** D318's behaviour editor corrupted the heap. Qt 6.4.2's
  `QFormLayout::setRowVisible` on a shown dialog writes out of bounds (valgrind); the forms now
  stay still while their rows change. A crashed local UI run leaves a recovery file that makes the
  next run wait at its dialog.
- **Not done:**
  - caching lateral bands (+10–28 % wall switched on, no shipped keys);
  - a shared `multra_shell`;
  - splitting `types.hpp`: estimated 466 → 420 objects rebuilt, 338 with the JSON
    declarations moved. The owner deferred it to the next engine slice that edits the runtime
    state (NEXT).

## 2026-10-23 — Dialogs keep their buttons on screen; the behaviour editor in tabs (D318)

- **The owner's report:** the behaviour editor was taller than the screen; not every key showed
  and Confirm was off screen. Measured, it needed at least 1378 px.
- **The editor now has three parts:**
  - a header with users, id, name and model;
  - four tabs that each scroll: *Car following*, *Lane changes*, *Perception and drivers*, and
    *Junctions, signals and speed*;
  - a footer with the error and the buttons.

  The other model's rows are hidden. Prototype opens at 579 px at 1366×768.
- **Other dialogs:** the UI suites, measured through a scratch `QDialog::exec` shim, gave vehicle
  type 696 px minimum, input 500, decision 452, and coarse Node values (one row per leg). These now
  share `dialog_fit` (a scrolling body, a fixed footer and a first size capped to the screen).
- **Rows:** `requireFitsScreen` in four UI suites. Three fail with the old dialogs; the coarse and
  decision rows are growth guards.

## 2026-10-23 — Audit: lateral behaviour and movement in switched-on runs (no code change)

- **Shipped runs never move sideways:** no catalog behaviour carries the lateral keys.
- **Switched on** (keys 0.5/1.0/1, four projects, seeds 42–51), a per-tick check found:
  - no 2D overlap, lateral rate within the cap, no position jump, no backward motion;
  - every lane change lateral, typically 3.5 s.
- **m2.6-study-template regresses:**
  - a changer past its window waits on the lane line at its dead end and jams both lanes
    (seed 45);
  - one waiting straddler is clamped repeatedly (seed 42); clamps go from 1 to 33.
- **The lab:** two changers swapping lanes lock side by side for about 50 s.
- LANE_CHANGE_DURATION §9's reopen conditions are met. The record is in
  [lateral-run-audit](evidence/lateral-run-audit.md), and the item is in NEXT.

## 2026-10-23 — S4.4: Node proposals after a run; S4 is complete (D317)

- **The Nodes tab** now lists, under the Nodes, the Nodes this revision's Macro or Meso result
  points at:
  - a Node with a leg at v/c 1.0 or more (Macro);
  - a Node with a leg that spills back in any slice (Meso).

  Selecting a row selects the Node. An edit makes the result stale, and the list empties.
- **A junction without a Node** is listed by its end, for example `link-1@50`, and its row points
  at *Suggest nodes*. A hot end at the network's edge gives no row: there is nothing to refine.
- **Rows:** RS9, RS10 (`nodeproposals`), with the editor part in `macro-ui`; four mutations caught.

## 2026-10-23 — B23 answered; S4.3: Fill defaults (D315, D316)

- **B23** (Meso benchmark), "as recommended": Boyles et al. 2025 for the corridor and network
  cases, Daganzo 1994/1995 as second choices, and not-blind cases as verification only. The next
  step is the owner's copy of Boyles et al.
- **Fill defaults…** (Problems tab) lists the absent values the project can already cite, none
  ticked:
  - a generated Link's type from its Road's template;
  - HCM 7's jam density on a project-owned type, only where it is valid for that type;
  - a Link-less Road's Links from its template.

  Each row shows its value, source and what it clears. The ticked ones apply in one Undo step, and
  the panel checks again.
- **Rows:** RS5–RS8 (`filldefaults`), with the editor part in `road-ui`; four mutations caught.

## 2026-10-23 — S4.1–S4.2: the readiness contract and the panel by level (D313, D314)

The owner chose all four S4 options as recommended: the Problems tab with a level selector, cited
fills F1–F3 (a coarse Node's Micro detail later), proposals from v/c ≥ 1.0 and spill-back, and the
order panel → fills → proposals ([READINESS](reference/READINESS.md)).
- **The Problems tab** now checks Micro, Meso or Macro (with its period). Each level lists the
  refusals of the functions its run calls, in the run's order, then advisories and seam rows. A
  study sees everything missing at once instead of one refusal per run.
- **An edit after a check** leaves a note to check again, instead of an empty list that read as
  "no problems".
- **Rows:** RS1–RS3 (`readiness`), RS4 in `tables-ui`; the panel's refusals are checked against the
  runs themselves. Four mutations caught.

## 2026-10-23 — S3.7: seam checks; S3 is complete (D312)

- **Seam checks…** (Roads tab) lists, for the chosen level, every place where the levels disagree:
  - a Road without Links, which Micro leaves out (Micro only);
  - a generated Link whose lanes differ from its Road's, fixed in one step by *Use Link* (the Road
    follows) or *Use Node* (the template is applied again);
  - a free-speed jump above 20 km/h between consecutive Links, adjustable in the dialog, with turns
    inside junctions skipped;
  - a Road that lost a generated Link.

  Nothing is refused or changed unless a fix is chosen.
- **Rows:** RD20's Micro half, RD21, RD22 and RD29 (`roadseams`), with RD21's editor part in
  `road-ui`; five mutations caught.
- **S3 (Roads) is complete:** S3.2–S3.7, D304–D312.

## 2026-10-23 — S3.6b: coarse values dialog, refine, derive back, lifecycle (D311)

- **Detail is the junction's interior** (D311, revising D310): a Connector inside the Node, or a
  Link wholly inside. Links that only reach in are legs, and Macro and Meso now compile them as the
  arms of a Node without detail.
- **Refine and derive back** happen inside whatever Command causes them. Drawing detail into a
  coarse Node drops its values. Deleting a Node's last detail derives them: signalised from the
  heads, stages from the signal groups' greens, banned turns from missing movements, and major legs
  from who yields.
- **Lifecycle:** when a Command leaves a Node with two Link-less Roads of one template meeting end
  to start, and nothing authored on the Node, the Node is removed and the Roads joined.
- **Editor:** *Coarse values…* on the Nodes tab, for a Node without detail.
- **Rows:** RD17, RD18, RD27 (`roadcoarse`) and RD28 (`road-ui`); RD19 gains a junction over
  generated Links; five mutations caught.

## 2026-10-23 — S3.6a: link-type rank, coarse Nodes, Macro and Meso from Roads (D310)

S3.6 is split: this half ships the fields with the compile that reads them; the coarse dialog,
refine, derive back and the lifecycle are S3.6b.
- **`rank`** on link types (schema 53, a Rank field in the Link-type dialog): the legs with the
  highest rank are major; an unranked leg or a tie leaves priority undetermined, which Macro and
  Meso advise.
- **`coarse`** on a Node without detail (schema 53): control, major legs, banned turns and a stage
  signal, refused on a Node with detail, with unknown legs or with a bad signal.
- **Macro and Meso** compile Roads without generated Links, one link per template direction,
  split at Nodes without detail. Such a Node, coarse or default, gives every turn between
  different Roads minus the banned ones, with the stages' g/C when signalised. Road ends within
  2 m join.
- **Rows:** RD4, RD16, RD19 and RD20's Macro and Meso half (`roadcoarse`), and the rank field in
  `macro-ui`; four mutations caught. RD20's Micro half moves to S3.7.

## 2026-10-22 — S3.5: Find crossings, overpasses and Merge nodes (D309)

- **Find crossings** (Roads tab) lists every place where Roads on one level cross, or one ends within
  the snap tolerance (2 m, adjustable) of another. Places with three legs or more are listed,
  skipping places a Node already covers. Each row is confirmed as a Junction or an Overpass, all in
  one Undo step.
- **Junction:** the Roads and their Links are split there (routes follow), and a generated round
  Node is added, sized from the Roads' width there plus the widest lane.
- **Overpass:** the pair is stored (`network.overpasses`, schema 53) and not offered again, even
  after one of the Roads is split elsewhere.
- **Merge nodes** (Nodes tab) replaces the selected Nodes with their hull, refused when a Node with
  valid detail would gain an issue.
- **Rows:** RD12–RD15 (`roadcrossings`), with their editor part in `road-ui`; four mutations caught.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
