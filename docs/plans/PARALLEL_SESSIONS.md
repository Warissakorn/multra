# Parallel sessions — 2026-11-23

Four slices that need no owner choice, chosen from [NEXT](../NEXT.md) so that they touch disjoint files
and can run at the same time, each in its own cloud session on its own branch. This file is the brief
each session receives; it is not evidence of completion. Status stays in NEXT and ROADMAP.

## Why these four

Everything else open in NEXT waits on the owner (OWNER_SITTING, an **Ask**, or "the owner's pick") or is
told to stop. These four are named in NEXT as left work with no owner gate, and their files do not overlap:

| Slice | Branch | Files it owns | Decision IDs (D-numbers) | BA ids |
|---|---|---|---|---|
| A — leader order kept across ticks | `claude/cool-shannon-btynrs-leader-order` | `src/core/` (the leader search), `tests/` (existing files), ENGINEERING_NOTES | 626–629 | BA216–BA219 |
| B — split `CMakeLists.txt` | `claude/cool-shannon-btynrs-cmake-split` | `CMakeLists.txt`, `cmake/`, BUILDING | 630–633 | none |
| C — Results tab strip at 1024 in Thai; clamp note | `claude/cool-shannon-btynrs-results-tabs` | `src/shell/editor_results*`, `data/locales/`, `tests/` (existing files) | 634–637 | BA220–BA223 |
| D — ARCHITECTURE's *Editor boundary* into the contracts | `claude/cool-shannon-btynrs-architecture-prose` | `docs/ARCHITECTURE.md`, `docs/reference/*` contracts | 638–641 | none |

## Rules every session follows (paste with each brief)

1. Read AGENTS.md, NEXT and the task-map row for your slice first, as AGENTS says. One system: only your
   slice. Work only on your branch; push with `git push -u origin <branch>`. Do not open a pull request.
2. **Shared files, to keep the four merges trivial:**
   - `docs/PROGRESS.md`: add exactly one entry at the top; never archive or edit another entry (the base
     branch left room: 8 entries, the limit is 12).
   - `docs/decisions/RECORD.md`: append only, using only your reserved IDs; leave unused ones unused.
     Add index lines only for your IDs.
     `multra-check-docs` refuses a `D`-number with no RECORD row, so write one only once its row exists.
   - `docs/NEXT.md`: edit only the bullet your slice came from (named in your brief) and its line in
     *Parallel sessions of 2026-11-23*. Do not rewrite the rest.
   - **Do not edit `CMakeLists.txt`** unless you are slice B. Put new test cases in an existing test file
     and an existing group.
3. Build and check as AGENTS says (`cmake --preset desktop` if Qt is present, otherwise `headless`, and
   say which). Before pushing: the full `ctest` and `cmake --build <dir> --target check` pass. Report
   failures with their output; never skip, disable or loosen a test. Linux-only verification is said so.
4. If the measured result says "do not ship", revert the code, keep the evidence and record the decision:
   a measured revert is a finished slice.
5. End as AGENTS' *Session end* says, then reply with: what changed in outcomes, the numbers, the checks
   run and their results, the commit hashes, and anything left for the owner.

## A — the leader order kept across ticks

From NEXT *Optimisation passes*, the bullet **Engine, the lever left**.

- Read [ENGINEERING_NOTES](../reference/ENGINEERING_NOTES.md) (*Leader search by rear, measured and
  reverted*, the callgrind recipe and the do-not-retry list), D28, D31, D597, D598, and `src/core/README.md`.
- The 2026-11-19 attempt rebuilt a by-rear order every tick and lost where queues are short. Try the
  untried version: keep each bucket's order across ticks (vehicles rarely overtake in a lane) and repair
  it with an insertion pass, so the per-tick cost where queues are short is near zero.
- Reproducibility first (rule 2): no unordered iteration, no thread-dependent arithmetic; the order is a
  pure function of the state. Event hashes for seeds 42 and 43 must be identical before and after.
- Measure with callgrind, before and after, on all three: the counted study, M2.6 and the 48-crossing
  engine benchmark (Release). Ship only if the study improves and neither M2.6 nor the benchmark gets
  worse by more than 0.5 %; otherwise revert and record the numbers.
- `src/core/` files stay at or under 500 lines (`types.hpp` is 471). If the change edits `Vehicle` or
  `SimState`, NEXT's standing fact applies: split `types.hpp` in the same slice and measure the rebuild.
- Record the numbers in ENGINEERING_NOTES under the 2026-11-19 entry, shipped or reverted.

## B — split `CMakeLists.txt` before it passes 500 lines

From NEXT *Optimisation passes*, the 2026-11-22 bullet ("`CMakeLists.txt` is at 498 lines: split before
adding to it").

- Read [BUILDING](../BUILDING.md), the existing `cmake/*.cmake` includes (each names its decision), D489,
  D621 and `.github/workflows/`.
- Move whole responsibilities into `cmake/` includes, as `right_of_way_fixtures.cmake` did: the tool
  executables, the test executables and their group registration, and the check targets are the obvious
  three. Aim for a root file well under 400 lines that reads as a table of contents.
- No behaviour change. Prove it: before and after, on the same preset, the sorted output of
  `ctest -N` is identical, the target list (`cmake --build <dir> --target help`) is identical, and
  `compile_commands.json` is identical after sorting. Run both the `desktop` and `headless` presets if
  Qt is present.
- Windows CI builds without precompiled headers and MSVC reads the same files: do not use features newer
  than CMake 3.24, and keep any `if(MSVC)`/`WIN32` blocks byte-for-byte in their new home.
- Merge note: slices A and C are told not to touch `CMakeLists.txt`. If one of them had to, the integrator
  merges B last.

## C — the Results tab strip at 1024 in Thai, and the clamp note

From NEXT *Tables like Excel*, the bullet **Left from the compactness review**.

- Read [EDITOR_WORKFLOW](../reference/EDITOR_WORKFLOW.md), the compactness-review entry it came from
  ([archived NEXT](../archive/NEXT-state-2026-11-19.md)), D490, D318–D320, and `src/shell/README.md`.
  Entry points: `src/shell/editor_results.cpp`, `results_levels.*`, `data/locales/`.
- 1: at a 1024 px wide window in Thai, every Results tab is reachable without the strip scrolling, or the
  overflow is a deliberate, discoverable control. Prefer shorter labels or a compact layout over a new
  widget; parameter names are never translated.
- 2: the clamp note on the clamps tab states, in its own text, that results are not yet validated (rule 4),
  in every locale.
- Tests: a UI test (offscreen) that opens Results at 1024 × 768 in Thai and asserts that every tab is
  visible without scrolling, and that the clamp note carries the not-validated statement. Assert the
  forcing first (the locale and size took effect), then the consequence (AGENTS). Use `useTestStorage()`.
- Take a screenshot before and after at 1024 × 768 in Thai and English, commit them under `docs/images/`
  only if a contract cites them, and state that the look is Linux only; the Windows look is the owner's.

## D — ARCHITECTURE's *Editor boundary* into the contracts

From NEXT *Review of 2026-11-10*, the Architecture bullet ("its *Editor boundary* prose (about 130 lines of
per-decision notes) still belongs in the contracts").

- Read [ARCHITECTURE](../ARCHITECTURE.md) whole, then each contract a paragraph belongs to:
  [NETWORK_EDITOR](../reference/NETWORK_EDITOR.md), [EDITOR_WORKFLOW](../reference/EDITOR_WORKFLOW.md),
  [DESKTOP_AUTOMATION](../reference/DESKTOP_AUTOMATION.md), the Connector contracts and the rest the
  paragraphs cite.
- Move each paragraph of *Editor boundary* (ARCHITECTURE lines 150–281) to the contract that owns its
  behaviour, merging with what the contract already says rather than duplicating it (rule 3, one source of
  truth). Keep decision IDs. What stays in ARCHITECTURE is the boundary itself: who owns the document,
  that commands go through `History`, that project never imports commands, and one link per contract —
  about 15–25 lines.
- Every contract stays under 500 lines; split along a heading if one would pass 450. Fix every link that
  pointed at the moved text (`grep -rn 'ARCHITECTURE.md#' docs src`).
- Docs only: `multra-check-docs .` and `multra-check-file-sizes .` pass. Write in the PROGRESS entry a
  table of where each paragraph went.

## Integration

When the four sessions report, the integrator (this session or the owner) merges the branches into
`claude/cool-shannon-btynrs` in the order D, C, A, B, resolving only PROGRESS (stack the entries by time),
the RECORD tail (by ID) and the *Parallel sessions* lines of NEXT; then runs the full build, `ctest` and
`check` once on the merged tree.
