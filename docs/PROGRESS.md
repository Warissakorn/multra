# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-15 — Documentation consistency pass (no behaviour change)

The owner asked for docs an AI can read efficiently. The session path (CLAUDE → AGENTS → NEXT)
went from 470 to 262 lines: AGENTS' three reading sections became one Session path. NEXT keeps
only live items; its notes moved verbatim (checked line by line) to
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md), with machine notes in BUILDING. Appearance
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
owner accepted every recommendation; [MULTI_LEVEL_WORKFLOW](plans/MULTI_LEVEL_WORKFLOW.md) records
the design, answers and order (S0 Node first). No code, schema or test changed. The D147 entry
moved whole to `archive/`.

## 2026-10-14 — Authorized whole-repository review fixes (D167–D174)

The owner approved the nine-item review plan together. Fixed evaluation passages, journey free-flow,
suffix display, phase bins (16 MiB budget), Lab axles, long OD searches (`OD_SKIPPED_DEMAND` advisory),
Split routing stations and batch cancellation (workers owned by generation); corrected parking identity
docs. The 16 MiB budget, OD advisory, worker map and Split guards were carried over from draft PR #150.
Validation: desktop build on Linux (GCC 13.3), 138/138 CTests; Windows desktop is CI-only (D167–D174).

## 2026-10-14 — Behaviour Lab: Urban circuit and heading-up camera (D166)

The owner asked for a varied lab network with straights and intersections, and the camera turning
with the vehicle. Answers: layouts as presets (Oval kept), fixed-time signals, the tuned vehicle
turning at some intersections. The Urban circuit (now the default, `lab_urban.cpp`) is four loops
that never share a lane — circuits refuse rules and zones and two feeders are a merge — crossing at
four split-phase intersections; `addLoop` cuts a closed path at its intersection boxes into Links
and Connectors. The first BL14 draft wanted the tuned vehicle at < 0.1 m/s at a red and never saw
it: queued behind others, the prototype creeps at ≈0.4 m/s; the row now asks for < 1 m/s. An hour
on 1 and 2 lanes has no safety clamp. Heading-up rotates the view each frame; the ~30 canvas scale
reads became `viewScale()` (hypot(m11, m12)), identical unrotated. Rows BL13–BL16. Linux only
(GCC 13.3, Qt 6.4.2 offscreen). The D146 entry moved whole to `archive/`.

## 2026-10-14 — Behaviour Lab: a window for tuning driving behaviour (D165)

The owner asked for a tool to develop driving behaviour: a view locked on one vehicle driving
endlessly round a purpose-built network, and a panel for the vehicle's, the road's and the other
traffic's parameters. Answers: opened from the editor's menu; changes apply live, keeping the
vehicles; prototype model first. Contract and rows BL1–BL12 in
[BEHAVIOUR_LAB](reference/BEHAVIOUR_LAB.md), written before the code.
- **Core** (three opt-in seams, absent = byte-identical): `Route::circuit` — the index holds the lap
  three times and a vehicle past 2L drops back L, so every look-ahead sees across the seam with no
  new code path; `retuneSimulation` swaps a same-shape scenario under the running vehicles;
  `placeVehicle` fills the ring at t = 0 with an exact count, drawing as an arrival does.
- **`src/lab/`** (Qt-free): a stadium ring built with the editor's commands, compiled through
  `buildScenario` directly — `compileDocument` refuses an input on a closed ring
  (`UNSUPPORTED_INTERNAL_INPUT`), and the lab needs none. `LabRun::apply` retunes when only values
  changed and restarts otherwise; telemetry reads the engine's spans as `closestVehicle` does.
- **Window**: the Run canvas follows a vehicle (`setRunFollow`, click to switch), a three-tab panel,
  a 60 s speed/gap chart, the unvalidated marker, and *Apply to project* as one History edit.
- The first BL10 draft retuned `followingTime` and saw no change: the prototype's steady gap is
  `standstill + (bx_add + bx_mult z) √v`; `followingTime` only sets how fast it is reached. The row
  now retunes `additiveSafetyDistance`.
Linux only (GCC 13.3, Qt 6.4.2 offscreen): desktop build, `circuit` 8/8, `lab` 5/5,
`behaviour-lab-ui`, CLI 42 unchanged, `check` green; Windows waits for `native.yml`. The D145 entry
moved whole to `archive/`.

## 2026-10-14 — Engine per-tick work (D164)

The owner chose the engine. A callgrind run of M2.6 (GCC 13.3, RelWithDebInfo) counted 3,833.9 M
instructions. Three commits brought it to 3,364.2 M (−12.3 %): signal events reuse `headColors`
(−94 M), queue lines walk only vehicles on crossing routes (`observe` 713 → 404 M), and arrivals
check the due time first (−64 M). Every shipped project's CLI output was byte-identical, one seed
and seeds 42–45; headless 90/90. Not done: `closestVehicle`, `occupiedSpans`, the event string copy.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
