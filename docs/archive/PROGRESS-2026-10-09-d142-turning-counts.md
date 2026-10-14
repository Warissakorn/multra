# PROGRESS archive — moved whole 2026-10-14

## 2026-10-09 — Volume from turning counts (D142, schema 26)

The owner answered ROADMAP O10's "count sheet typed once vs D46": an input may take its volume
from its entry decision's turning counts. `VehicleInput.volumeFromCounts` is opt-in, so D46 and
every existing file are untouched. `src/project/counted_volumes.cpp` holds the one rule:
`countedDecision` (the decision named, or the placed one on the entry Link), `countedVolumes`
(summed base-route counts × 3600 / length per decision interval) and `syncCountedVolumes`, called
on read, in `putInput`/`putRoutingDecision`, in `History::execute` and at the top of
`withRoutingDecisions`, so no path sees a stale volume. The input's intervals are not written for
such an input. Issues `INPUT_COUNTS_NO_DECISION`, `INPUT_COUNTS_EMPTY` and
`INPUT_COUNTS_POSITIONED` (a D119 decision is a downstream split, not a source). The input dialog
gains the checkbox, which locks the typed volume, counts and Periods and shows the counted total;
the input and decision rows say which feeds which. `field<bool>` now refuses a non-boolean.

Evidence (Linux, GCC, Qt 6.4.2 offscreen, Debug): `demandcounts` 8/8 (VC1–VC7, each movement's
vehicles equal its counts for unplaced and placed decisions), `counted-volume-ui` (VC8), and the
D46 test unchanged. CLI `42 --project` output for the four shipped projects equals `origin/main`'s
(`c7628e8`, built in a scratch worktree) byte for byte. Native Windows CI and the owner look are owed.

---
