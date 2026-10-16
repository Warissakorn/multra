# Main's Windows-only UI reds: evidence — 2026-10-16

Branch `claude/next-windows-ui-red`, on main `a88bf5c` (D205). Local host: Linux, GCC 13.3.0, Ninja,
Qt 6.4.2, `QT_QPA_PLATFORM=offscreen`, `build/desktop` (Debug). The Windows side is CI's
`windows-desktop` job (Qt 6.5.3, MSVC 14.44); the fix is **unconfirmed on Windows** until that job
runs on this branch.

## What failed

Job 116958442326 (run 38896597043, main `a88bf5c`; the same four since `7a44636`, PR #170):

```
115 - canvas-lod-ui (Failed)            A rotated view decides the markings differently
122 - properties-layout-ui (Failed)     th: editorConnectorObject at 144, column at 147
123 - properties-layout-ui-1.5x (Failed) th: editorConnectorObject at 144, column at 147
124 - properties-layout-ui-2x (Failed)  th: editorConnectorObject at 144, column at 147
```

## Causes

| Test | Cause | Why Linux passed |
|---|---|---|
| `canvas-lod-ui` | At exactly `3/narrowest`, `rotate(37)` gives `viewScale() = hypot(s·cos, s·sin)` an ulp below `s`, and `w·ppm >= 3` was strict | glibc's result rounds to `s` or above |
| `properties-layout-ui` | Qt on Windows has no libthai, so `editorMedianGap`'s Thai label "ระยะห่างถนนทิศตรงข้าม" is one word of ~123 px. `QWidgetItem` expands a label to its minimum, the row stays beside it, and the Link page's column moved to 147 while the Connector page stayed at 144 (16 + 120 + 8) | Linux breaks the Thai word (its minimum is 24 px) |

## Rows (failure first, forcing asserted first)

| Test | Case | Before the fix | After |
|---|---|---|---|
| `canvas-lod-ui` | `at·(1−1e-12)`: the forcing `narrowest·rounded < 3`, then markings readable | "A rounding of the scale hides the markings" | pass |
| `properties-layout-ui` (1×, 1.5×, 2×) | The Link page's `editorMedianGap` label set to a run of `i` until its minimum is ≥ 123 px (forcing: between 122 and 150 px, row beside its label), then every page's column checked; the language switch restores the text and the column | "unbreakable: editorFromLaneCount at 144, column at 148" (with the clip check lifted locally) and "A Properties label is clipped" (as committed) | pass |

| `properties-layout-ui` (1×, 1.5×, 2×) | With the word in place, the fold closed (forcing: `editorDrivingSide` hidden), the text restored by the language switch, then every form checked with the fold still closed | "hidden fold: editorDrivingSide at 140, column at 136" | pass |

Mutation: reverting `canvas_style.hpp` alone fails the first row; reverting `editor_interaction.*`
alone fails the second; removing the hidden-host `activate()` alone fails the third. All restored.

## Round 2: the branch's first Windows run

`workflow_dispatch` run 38912924434 on `21fc3ed`: `canvas-lod-ui` passed, so the threshold fix holds
on Windows. Three tests still failed, with a new message:

```
122 - properties-layout-ui (Failed)      en: editorDrivingSide at 139, column at 136
123 - properties-layout-ui-1.5x (Failed) en: editorDrivingSide at 139, column at 136
124 - properties-layout-ui-2x (Failed)   en: editorDrivingSide at 139, column at 136
```

The shared column made the width change between languages: 123 in Thai on Windows, 120 in English.
The test's loop goes 1360 th (fold open, laid out at 139), then 1024 en with the fold closed first.
`QLayout::widgetEvent` lays out only visible widgets, so the hidden fold kept 139, and `aligned` counts
it because a label is `isVisibleTo` its own host. `FormColumn::refresh` now lays out hidden hosts at
once. The third row above reproduces this on Linux.

## Results

`ctest --test-dir build/desktop -R "canvas-lod-ui|properties-layout-ui"` 4/4. Full results are in
the PROGRESS entry. UI only: no engine, schema or project file changed, so BA39 is untouched.
