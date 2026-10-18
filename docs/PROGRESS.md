# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

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

## 2026-10-18 — Give-way line stop implemented (D240)

D239's contract ([evidence](evidence/give-way-stop.md)).
- **Rule:** on a tick that would clamp at a binding give-way line, a stop within the type's maximum
  brakes at `v²/(2·gap)` instead. BA147–BA149 failed first; the guards were proved by mutation.
- **Measured:** the shipped Yield T-junction's 6 clamps at seeds 42–81 go, with delays unchanged.
  Every other shipped output is byte-identical. The 11 replaced ticks braked at 2.5–7.4 m/s². Ir
  +0.11–0.15 %.
- **Correction:** D232's "none at seeds 42–51" was wrong (seed 46 has 2).
- **Sanitizer:** CI ASan caught BA151 reading a reference into a temporary scenario; the row now
  holds it in a local.

`check` 178/178, Linux only.

## 2026-10-18 — Give-way line stop contract (D239, docs only)

The owner asked for the easiest open item: M3.2.8a.1's give-way line clamps (D232)
([GIVE_WAY_STOP](reference/GIVE_WAY_STOP.md)).
- **The owner's choice:** brake to stop at the line.
- **The contract:** only on a tick that today clamps at a binding give-way line, and only when the
  stop fits the type's maximum, the vehicle brakes at `v²/(2·gap)` and no clamp is reported.
  Everything else stays byte-identical. Rows BA146–BA151.

## 2026-10-18 — B-9 red-light running implemented (D238); tier B complete

D237's contract ([evidence](evidence/red-running.md)).
- **Rule:** a hashed share of drivers pass a red head in its first seconds while moving; a
  `red-running` event when they pass the line; `redRunning` in reports only with the keys. BA139
  and BA144 failed first against a step that ignored the keys.
- **File and editor:** schema 44, refused below; the pair and ranges validated on both models; two
  behaviour-dialog rows. Every hook proved by mutation.
- **Measured:** shipped runs byte-identical. At a 5 % share runs are rare and rise steeply with the
  window (four-leg 2, 8, 24 in 40 runs for 1–3 s); delay falls within its half-width. Ir −0.44 %
  (layout).

`check` 177/177, Linux only.

## 2026-10-18 — B-9 red-light running contract (D237, docs only)

NEXT's item, the last of tier B ([RED_RUNNING](reference/RED_RUNNING.md)). Today red holds
every vehicle that can still stop.
- **The owner's choices:** only in the first T seconds of red and only while moving; a fixed share
  of drivers decided once per vehicle by a hash; no new interaction with crossing traffic, but each
  run is counted.
- **The contract:** `redRunningShare` and `redRunningTime` as a pair at schema 44; W74's hash at
  index 15; a `red-running` event and a `redRunning` count written only with the keys; rows
  BA138–BA145.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
