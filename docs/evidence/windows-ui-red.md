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

## Round 3: a narrow form kept a row beside its label (D242) — 2026-10-18

Branch `claude/m3.4-lane-change-duration-slice1`, on `a80c64f`. Same local host. **Unconfirmed on
Windows** until `windows-desktop` runs on the pushed head.

**What failed.** After D205, `properties-layout-ui` (1×, 1.5×, 2×) gets further on Windows and fails
the narrow floating-panel check:

```
A narrow form kept a field beside its label
```

It fails the same way on main `b066624` (run 38924644741, job 117052970818) and on PR #177's heads
(for example job 117641915295 on `a80c64f`). The message did not say which form.

**Cause.** `FormReflow` stacks a form (`WrapAllRows`) when its host is narrower than two label
columns (240 px). The three tab pages are hosts inside a `QScrollArea` with `widgetResizable`, so a
page is never narrower than its widest child. Text-only action buttons cannot wrap (OWNER_SITTING
E17). Measured on Linux in the 200 px floating dock:

| Language | Page | Page width | Viewport | Widest child |
|---|---|---|---|---|
| English | Link | 248 | 188 | 232 (an action button) |
| English | Connector | 223 | 188 | 207 |
| English | Image | 188 | 188 | 168 |
| Thai | Link | 196 | 188 | 180 |
| Thai | Connector | 204 | 188 | 188 |
| Thai | Image | 188 | 188 | 149 |

The English Link page's form gets 248 − 16 = 232 px, 8 px short of two columns. A Windows button
only a little wider (its font and style) reaches 240, so the form kept its rows beside their labels
in a 188 px viewport. Linux stayed under the threshold and passed.

**Fix.** The width that decides the policy is the visible one: the host's contents width, or the
enclosing scroll area's viewport width, whichever is less. `FormReflow` also watches that viewport,
because a page at its minimum width does not resize when only the viewport does. The top group (the
dock body, not scrolled) is unchanged.

**Rows** (`tests/properties_layout_ui_tests.cpp`, 1×, 1.5×, 2×). In the narrow fixture,
`editorAddLinkPoint`'s text grows until the Link page's minimum is two columns plus 40 px. The dock
then goes to 300 px and back to 200; the forcing asserts the page keeps its width while only the
viewport narrows.

| Check | Before the fix | After |
|---|---|---|
| At 300 px the Link form shows two columns | — | pass |
| At 200 px no row sits beside its label ("wide page") | "A narrow form kept a field beside its label", with the new diagnostic naming seven Link fields at host 267 px, viewport 188 px | pass |

`aligned` now prints, for a row that stays beside its label in a narrow check, the field, both x
positions and widths, the host width, the wrap policy and the viewport width. If Windows still fails,
the next run says why.

**Mutations,** each restored:

| Mutation | Caught by |
|---|---|
| The viewport width not used | "A narrow form kept a field beside its label" |
| The viewport not watched | "A viewport wide enough for two columns kept the Link form stacked" |

UI only: no engine, schema or project file changed.
