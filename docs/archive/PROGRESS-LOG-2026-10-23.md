# PROGRESS log archive — 2026-10-23

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

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
order panel → fills → proposals ([READINESS](../reference/READINESS.md)).
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
