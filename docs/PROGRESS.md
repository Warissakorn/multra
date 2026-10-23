# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

## 2026-10-22 — S3.4: Join opposite Links and Assign template (D308)

The owner chose the join tolerance: 15° off opposite, and half both widths plus 30 m apart.
- **Join opposite Links:** two Links running opposite ways on one level become one Road without a
  template. Its centreline is the mean of theirs, and the Links stay where they are. A Link
  already in a Road, a different level, the same direction, or a pair outside the tolerance is
  refused.
- **Assign template:** sets or clears the template on several Roads at once, generating no Link.
- **Editor:** both are on the Roads tab. Join takes the two Links selected on the canvas, forward
  first, and the table takes several rows.
- **Rows:** RD10 and RD11 (`roads`), with their editor part in `road-ui`; four mutations caught.

## 2026-10-22 — S3.3b: the Road tool, the Roads and Cross-sections tabs (D307)

The owner chose: apply at once, key O, and detached Roads shown in the table and on the canvas.
- **Road tool (O):** Ctrl+right-click the first centreline point, click the rest, Enter. With the
  Roads tab's template chosen, the Road and both directions' Links are one Undo step; with none, a
  Road without Links.
- **Roads tab:** template, lanes, Links and status (applied, not applied, partly detached *k of n*),
  with *Apply template*, *Edit road* (overrides) and *Delete road*. The selected Road is lit on the
  canvas, in the warning colour while partly detached.
- **Cross-sections tab:** add, edit and delete owned templates; one in use is kept.
- **Rows:** RD23–RD26 (`road-ui`); four mutations caught. Tool and tab inventories in
  `workspace-ui`, `object-groups-ui`, `design-system-ui` and `zone-ui` grew by one tool and two tabs.

## 2026-10-22 — E1: tool pointers, plain clicks after the first point, solid previews (D306)

The owner asked for three canvas changes.
- **Pointers:** no tool shows the crosshair. Select is the arrow; every other tool shows the arrow
  with its toolbar glyph on a small badge, as a move shows the hand.
- **Multi-click tools** (Zone, Node, Queue counter, Section, the two-click Connector): only the first
  point needs Ctrl+right-click; later points take a plain left click. Ctrl+right-click still works,
  and with no draft a left click still only selects (amends D84).
- **Previews are never dashed:** a solid accent line on a translucent casing, and the edges still to
  come (a Zone's closing edge, the edge to the pointer) fainter. Dashes that mean something in saved
  work stay.
- **Rows:** E1-1 to E1-4 in `zone-ui`, `node-ui`, `section-ui`, `queue-counter-ui` and
  `editor-attachments`; four mutations caught.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
