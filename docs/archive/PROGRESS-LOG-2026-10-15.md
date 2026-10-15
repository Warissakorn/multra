# PROGRESS log archive — 2026-10-15

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.
## 2026-10-15 — Documentation cleanup for AI reading (no behaviour change)

The owner asked for docs that are organised, trimmed and efficient for an AI to read, and for
an analysis of whether the archive and the evidence are still needed. **Archive:** the 124
PROGRESS files were reachable only from the archive index. They hold history; the reasoning is
already in RECORD. They are merged verbatim, newest first, into 17 dated `PROGRESS-LOG-*` files
(134 → 31 archive files, index 206 → 45 lines), and each keeps its former file name as an anchor.
**Evidence:** every file is cited, either directly or through its note, and tests read the
`m3.2.7-*` data, so nothing is deleted. A new `evidence/README.md` says to read the note, not the
raw data. **Live docs, moved whole rather than summarised:** PROGRESS keeps its recent entries
(498 → 91 lines); ROADMAP keeps open milestones, gates and status rows (50 → 27 KB); RECORD
collapses the wholly superseded D3, D4, D9, D10 and D14; VISSIM_PARITY drops its 2026-09-14
§1, §3 and §6; the answered 2026-10-09 roadmap review moves to `archive/`. Every ID, gate and
anchor is kept. Checked: `trafficsim-check-docs` and `trafficsim-check-file-sizes` built
standalone (Linux, GCC 13). The full CTest was not run because nlohmann/json is missing in this
container; no code, test, data or spec file changed.

## 2026-10-15 — Documentation consistency pass (no behaviour change)

The owner asked for docs an AI can read efficiently. The session path (CLAUDE → AGENTS → NEXT)
went from 470 to 262 lines: AGENTS' three reading sections became one Session path. NEXT keeps
only live items; its notes moved verbatim (checked line by line) to
[ENGINEERING_NOTES](../reference/ENGINEERING_NOTES.md), with machine notes in BUILDING. Appearance
checks of merged reviews moved to OWNER_SITTING C10–C12 and E10–E11. `native.yml` passed on `main`
`c2bacd1`, retiring NEXT's CI notes. Status lines contradicting the code were corrected: W74
runs (D138), LOS and batches exist (M5), M8 is implemented, schema 31. History, archive, evidence
and specs are untouched.

## 2026-10-15 — Multi-level workflow design accepted (D175)

Docs only. A design review in chat (Thai) walked how the three levels, the editor and the
network work together, compared with Aimsun, and asked for starting at any level. Two scrutiny
passes corrected the drafts: the D145 plan reused an M5.1 Node that does not exist, bound OdMatrix
periods to Micro's run duration, and stored subarea cuts that go stale; per-field status flags
duplicated what field presence already says; mid-link Micro destinations need a core contract
(D153), so Micro keeps driveways as Links. Web checks before recording: GEH/DMRB wording
(secondary sources, flow test added), HCM 1,900 pc/h/ln with wide local variation, UTM 47N/48N
split at 102°E with legacy Indian 1975 datums, and motorcycle PCU varying within the green. The
owner accepted every recommendation; [MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) records
the design, answers and order (S0 Node first). No code, schema or test changed. The D147 entry
moved whole to `archive/`.
