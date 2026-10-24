# PROGRESS log archive — 2026-10-23

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

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
