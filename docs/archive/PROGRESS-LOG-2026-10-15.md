# PROGRESS log archive — 2026-10-15

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.
## 2026-10-15 — M3.5.0 perception contract (D179; docs only)

[PERCEPTION](../reference/PERCEPTION.md) is the first contract in D178's order.
- **Two leaders.** The **physical leader** (today's unbounded search) keeps the hard cap, zone
  admission and phase-2 caps. **Perceived leaders** (within `lookAheadDistance`, up to
  `observedVehiclesAhead`, effective gaps less the room of the vehicles in between) feed only
  the acceleration, so an unseen leader becomes a counted clamp, never an overlap.
- **Mirror check.** A target-lane follower counts only within `lookBackDistance`, plus an optional
  rear time-to-collision test. Something alongside always refuses the change.
- **Reaction time** is an action delay: a per-vehicle FIFO of `round(τ/dt)` computed
  accelerations. It replaces the review's ring of past snapshots, because it costs one entry per
  vehicle per tick and needs no remapping of past positions. Hypothetical tests stay undelayed.
- **Data.** Five optional keys; absent = today. One new schema, only when used.

Rows BA30–BA38 are pending; nothing is implemented. Checked: `check` on Linux.

## 2026-10-15 — Owner's answers to B14 and B15 (D178; docs only)

"B14 รับ, B15 ใช้ค่ามาตรฐานไปก่อน และจะทำการวิจัยในอนาคต". The review's order is the queue, keep-left
is the default bias on left-hand-traffic networks (a behaviour key), and turn-on-red/U-turns stay
in M3.9. Motorcycle values start from cited published standards, flagged uncertain, until the
owner's research. Recorded in RECORD, ROADMAP M3.4/M3.8/M3.9, NEXT and the review header; B14/B15
moved out of OWNER_SITTING.

## 2026-10-15 — Driving-behaviour completeness review (D177; docs only)

The owner asked for a thorough review of driving behaviour (lateral behaviour, mirror checks,
overtaking, everything that makes runs realistic) and chose "review plus roadmap" with
motorcycles as core scope. [The review](../audits/DRIVING_BEHAVIOUR_REVIEW.md) reads the whole of
`src/core/` and the behaviour contracts. It finds a one-dimensional engine, one observed vehicle
each way at unbounded distance, no reaction delay, instantaneous lane changes with a display-only
slide, overtaking only as D95 (off), no turning speeds and no lateral position. Its gap matrix
covers twelve areas (A–L). Lateral, lane-change duration and motorcycle work all need one
2D-occupancy contract, and perception and reaction need one observation contract, so both
come before W99. ROADMAP gains M3.5–M3.9 and an extended M3.4 (hard rule 8); PROBLEM §2 gains the
motorcycle row and §8.1 is answered; NEXT follows the review's order; B14/B15 hold the owner's
remaining calls. Checked: `check` (docs, sizes, architecture, CTest) on Linux.

## 2026-10-15 — A cited, uncalibrated `w74` preset (NEXT item 1, D176)

`data/driver-behaviour/presets/w74-sumo-defaults.json` maps every one of the 18 keys from SUMO's
`MSCFModel_Wiedemann` defaults (pinned at `79e42b50`), the file W74.md already follows. It sits
in a subfolder because the catalog loader reads only top-level `*.json`, and an unused `w74`
behaviour would give every vehicle traits (BA18); `tools/t_junction_sweep.hpp` now lists the
same files the loader reads, so its archived metadata still matches. A codec test pins that the
preset is valid and not loaded. **Evidence** ([w74-preset](../evidence/w74-preset.md)): with the
preset, four-leg discharge ≈3.44 s (fixture ≈3.11 s, prototype ≈2.05 s) and 3.2–4.7 clamps
per run (fixture 8.2–14.2), 72–98 % at amber, which `w74` behaviours still hold as red. No
defect, nothing calibrated. The fixture no longer reproduces BA27's hashes (D159, M4.2), so
both were rerun on one build. Shipping it, and an amber value, are OWNER_SITTING B13.
Checked: desktop build and full CTest (Linux, GCC 13.3), `check`, CLI seed 42 unchanged.

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
