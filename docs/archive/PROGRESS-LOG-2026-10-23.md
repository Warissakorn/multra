# PROGRESS log archive — 2026-10-23

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

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
