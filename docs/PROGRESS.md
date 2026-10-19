# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-19 — `joinAnticipation` as an off-by-default switch (D254, D255)

The owner chose D253's A4 anyway, as a fourth project switch that is off by default, and asked for
the line approach as a contract with numbers first (D254, [JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md) §5).
- **Engine:** `src/core/join_anticipation.*`. With the switch on, a vehicle within 50 m of a join runs
  its following law a second time against the other stream's first moving vehicle past its line,
  and keeps the stricter acceleration. Off, the step reads one empty vector.
- **File and editor:** `clampRules.joinAnticipation`, absent = off, written only when true at
  schema 46; a fourth Run settings checkbox; one Undo step.
- **Rows:** BA168–BA172 and the Run settings UI test. The "nearer the join" test proved redundant
  under mutation (a positive projected gap implies it) and was removed; every other filter is caught.
- **Measured:** on, seeds 42–81 of the four shipped projects reproduce D253's A4 outputs byte for
  byte; off, all four are byte-identical to D251, with the pin unchanged. Ir off: +0.16 % / +0.11 %
  / +0.00 % (four-leg, lane lab, M2.6).

## 2026-10-19 — Lane lab clamp traced (D252); join anticipation measured (D253, docs only)

The owner asked for both of NEXT's M3.2.8a.1 candidates, with the contract and numbers before any
code for the second.
- **D252** ([evidence](evidence/merge-clamps.md#d252-the-lane-change-labs-clamp)): the lane lab's
  only clamp (seed 74) is not a merge. A follower brakes at only 4.4–4.8 m/s² behind a leader
  stopping hard at its stub lane's dead end. That is D241's follower class, from the prototype
  braking's softness (D227).
- **D253** ([JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md), [evidence](evidence/join-anticipation.md)):
  eight reverted prototypes.
  - **Replacing the following leader** with the other stream's vehicle produced phantom leaders, side-by-side
    stops, and acceleration into the real leader: thousands of clamps, or 177 clamps and 12
    overlaps.
  - **A4** (a second following law, keeping the stricter acceleration) is clean, but it nets M2.6 only
    17 → 16 and moves T-junction delay from the minor road to the major road (7.069 → 7.545 s).
  - **Not recommended.** The line approach (D227's floor) is named as the better candidate.

No engine code is committed. `check` 179/179, Linux only.

## 2026-10-19 — Merge clamp switches implemented (D250, D251)

The owner chose D249's recommended package, as project switches that are on by default
([MERGE_CLAMPS](reference/MERGE_CLAMPS.md) §3, [evidence](evidence/merge-clamps.md#d251-the-package-implemented)).
- **Engine:** `ClampRules{leaderTravel, sideEntryAtLine, redLineStop}` on the definition.
  - `tickCap` relieves the hard cap only on a tick that would clamp.
  - `caps.cpp` holds B and M; the step's line-stop test takes Q.
  - `simulation.cpp` stays at 500 lines through a byte-identical refactor into `holdCap` and `leaderCap`.
- **File and editor:** `definition.clampRules` at schema 45, written only when a switch is off;
  three Run settings checkboxes; one Undo step.
- **Rows:** BA158–BA167, plus the Run settings UI test. D105's row keeps its clamp with
  `sideEntryAtLine` off. All 14 mutations are caught.
- **Measured:**
  - M2.6 41 → 17 clamps; delay 46.013 → 46.010 s; trips unchanged; no overlaps;
  - all off is byte-identical on all four projects; the pin and the TS rows are unchanged;
  - Ir +0.50 / +0.73 / +0.68 %.
- **A prototype bug:** D249's red-head stop braked with the wrong distance. Its measurement stands
  as recorded, and the implementation's numbers replace it.

`check` 179/179, Linux only.

## 2026-10-18 — Red running closed (D248); M3.2.8a.1's clamps traced and options measured (D249)

- **D248 (docs only):** the owner kept `redRunningTime` as an upper bound and a runner's committed
  crossing counted. RED_RUNNING now says that with `amberDeceleration` a run almost always starts at
  red onset.
- **D249 (docs only, [MERGE_CLAMPS](reference/MERGE_CLAMPS.md), [evidence](evidence/merge-clamps.md)):**
  - **Trace:** temporary probes traced all 41 shipped clamps (M2.6, seeds 42–81) to four causes:
    - 20: a minor creeping at its line, 1 m from the join, beside a major entering the shared lane;
    - 9: a minor that pulled out as committed;
    - 8: a major catching a minor still on its connector;
    - 4: a creep at a red head.
  - **Options:** seven, each a reverted prototype. B+M+Q removes 24 of the 41, with delay, trips,
    overlaps (0), the CLI pin and the TS reference rows unchanged. An unscoped red-head stop fails
    2 TS rows. A 3 m setback and "held is not committed" make things worse.
  - **No engine code is committed;** the owner chooses.

`check` 178/178 on the committed tree, Linux only.

## 2026-10-18 — Red running: the owner's answers to D241 (D246, D247)

NEXT's first candidate. The owner chose all three recommended options
([RED_RUNNING](reference/RED_RUNNING.md) §1, §3; [evidence](evidence/red-running.md#d247-the-owners-answers-to-d241-2026-10-18)).
- **The rule:**
  - a runner is let through anew only when it cannot stop gently (`amberDeceleration`, else the
    type's `comfortableDeceleration`);
  - once let through, the head does not hold it again during that red while it moves;
  - the crossing is counted even after the window.
- **Code:** `letThrough` replaces `runsRed`. The (vehicle, head) pairs persist in
  `SimState::redReleases`. `StopService` and `RedRelease` move to `line_state_types.hpp`, so `types.hpp`
  stays under 500 lines.
- **Rows:** BA139 amended, BA152–BA157 added. Six mutations are each caught.
- **Measured:**
  - shipped outputs are byte-identical;
  - with a 5 % share, M2.6's clamps return to 41, so D241's added clamps go;
  - the 1, 2 and 3 s windows now give identical outputs (four-leg 5 runs, M2.6 15): a probe found
    every run starting at, or 0.3 s after, red onset (read as vehicles that went
    on amber; not probed further);
  - Ir +0.18 % / +0.10 %.

`check` 178/178, Linux only.

## 2026-10-18 — Windows ships libthai for Thai line breaks (D245)

The Windows diagnostic showed that the 257 px text was Thai ([evidence](evidence/windows-ui-red.md#round-6-thai-had-no-line-breaks-on-windows-d245--2026-10-18)).
- **Cause:** Qt finds Thai line breaks through libthai, which it loads at run time as `thai.dll`. The
  Windows runner had none, so a Thai sentence broke only at its spaces. A Linux probe gives 60 px
  with libthai and 268 px without it. That is why D242–D244 did not change Windows.
- **The owner's choice:** ship libthai with Windows.
- **Fix:**
  - CI and the package install MSYS2's libthai, as `thai.dll` with `libdatrie-1.dll` and `thbrk.tri`;
  - the package ships them, with their LGPL licences;
  - the editor points `LIBTHAI_DICTDIR` at `data/libthai` beside the executable.
- **Tests:** the Thai pass of the narrow fixture requires a line break inside a Thai sentence. A
  mutation to an unbroken Latin string is caught.

`check` 178/178, Linux only. Unconfirmed on Windows until `windows-desktop` runs.

## 2026-10-18 — A wrapped Properties text asks only for its longest word (D244)

The Windows diagnostic named the widget ([evidence](evidence/windows-ui-red.md#round-5-a-wrapped-texts-minimum-held-the-panel-d244--2026-10-18)).
- **Cause:** `editorEffectiveBehaviour` asked for a 257 px minimum on Windows (Qt 6.5.3), against
  69 px on Linux. That held the floating panel at 273 px, so the top forms never stacked.
- **Fix:** `ReflowLabel`'s wrapped minimum is its longest word, never more than Qt's.
- **Tests:** a check that every wrapped top text asks for no more than its word (plus 8 px). It
  passes on Linux before and after; the evidence that it fails is the Windows run. Inflating Qt's
  value fails without the cap and passes with it.

`check` 178/178, Linux only. Unconfirmed on Windows until `windows-desktop` runs.

## 2026-10-18 — Top Properties buttons elide (D243)

D242's diagnostic on Windows named the real cause ([evidence](evidence/windows-ui-red.md#round-4-a-top-group-button-held-the-panel-wide-d243--2026-10-18)).
- **Cause:** the top forms' host was 273 px with no viewport. The appearance toggle's unwrappable
  text set the floating panel's minimum width (Linux 218 px, Windows about 273).
- **The owner's choice:** buttons shrink and elide with "…".
- **Fix:** `ElidingToolButton` (full-text size, ellipsis minimum, `Preferred` policy, elided
  paint) for the toggle, Apply behaviour type and Apply display.
- **Tests:** a long-toggle row failed first (the dock went to 299 px), and three mutations are
  caught.

`check` 178/178, Linux only. **Windows result:** the same failure (host 273 px), so the toggle was
not the cause there. The narrow check now prints the dock, body and tab widths and the widest top
widgets, so the next Windows run names the culprit.

## 2026-10-18 — Windows narrow Properties forms (D242)

The owner asked to fix CI ([evidence](evidence/windows-ui-red.md#round-3-a-narrow-form-kept-a-row-beside-its-label-d242--2026-10-18)).
- **Failure:** `windows-desktop` failed `properties-layout-ui` (1×, 1.5×, 2×) with "A narrow form
  kept a field beside its label", on main too.
- **Cause:** a scrolled page is never narrower than its widest action button. On Linux the English
  Link page is 248 px in the 200 px dock, 8 px short of two columns. A slightly wider Windows button
  reaches them, so the form kept rows beside their labels.
- **Fix:** `FormReflow` decides by the visible width (the scroll viewport's when that is less) and
  watches the viewport.
- **Tests:** the new row failed first with Windows' message, and both mutations are caught. A row
  still beside its label in a narrow check now prints its form and widths.

`check` 178/178, Linux only. Unconfirmed on Windows until `windows-desktop` runs.

## 2026-10-18 — M2.6's red-running clamps traced (D241, no code change)

NEXT's item 1 ([evidence](evidence/red-running.md#d241-the-added-m26-clamps)). A temporary probe
was never committed.
- **Where:** only seed 51 (3 → 5 at 2 s and 3 s) and seed 73 (0 → 1 at 3 s) change.
- **None is a red-head hold.** Every added clamp is bound by a vehicle leader:
  - Seed 51: a runner released at red onset, after braking for amber, meets the green flow at a
    derived merge rule. It is let go 0.55 m behind a leader and hits the standstill cap (D105,
    D201 B5); its follower clamps behind it.
  - Seed 73: a runner held again when the 3 s window ends stops at 6 m/s², and its follower needs
    10.6 m/s².
- **Found:** at 2 s the seed-51 runner crosses after the window as committed under
  `amberDeceleration`, so no event is counted. RED_RUNNING §3 now says so. Three §3 behaviours are
  listed as owner choices.

`check` 178/178, Linux only. CLI 42 byte-identical after the probe was reverted.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
