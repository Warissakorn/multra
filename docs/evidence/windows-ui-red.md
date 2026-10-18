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

## Round 4: a top-group button held the panel wide (D243) — 2026-10-18

On `f10fdc7` (D242), `windows-desktop` still failed the same three tests. This time D242's
diagnostic named the cause (job 117653333216):

```
narrow: editorId beside its label; label x 8 w 123, field x 139 w 126, host 273, policy 1, viewport -1
narrow: editorDrivingSide beside its label; ... host 257, policy 1, viewport -1
```

These are the top group's forms (ID/Name/Behaviour and the appearance fold), not a scrolled page.
Their host, the dock body, was 273 px wide in the "200 px" floating panel.

**Cause.** Nothing above the tab pages scrolls. On Linux the floating dock's minimum width is the
body's: 218 px, set by `editorAppearanceToggle`, a text button that cannot wrap (202 px, plus 16 px of
margins). That is why the narrow fixture got 218, not 200. On Windows the same button is wider, so
the panel's minimum and the body's width were about 273 px. That is more than two columns, so the
forms correctly kept their rows beside their labels. D242's `dock->width() <= 220` check passed only
because it was read straight after `resize(200)`. Most likely Windows applied the window's minimum
size afterwards.

**Fix.** `ElidingToolButton` (`editor_interaction.*`):
- it asks for its full text (`sizeHint`), but its minimum width is the text replaced by an ellipsis;
- its horizontal policy is `Preferred`, where `QToolButton`'s is `Fixed`;
- it paints the text elided when narrower, and the action's tooltip keeps the whole text.

The three text buttons above the pages use it: the appearance toggle, Apply behaviour type and
Apply display. Buttons inside the scrolled pages keep their behaviour (E17). On Linux the body's
minimum in English falls from 218 to 169 px, so the narrow fixture is now really 200 px.

**Row** (1×, 1.5×, 2×). In the narrow fixture, `editorAppearance`'s text grows until the toggle
asks for two columns plus 40 px. The dock is resized to 200 px. Then:
- the dock must stay at or under 220 px;
- the toggle must be narrower than its full text;
- no top row may sit beside its label;
- the language switch restores the text.

Before the fix: "A long top-group button widened the narrow panel" (the dock went to 299 px).

**Mutations,** each restored:

| Mutation | Caught by |
|---|---|
| The toggle a plain `QToolButton` | "A long top-group button widened the narrow panel" |
| No `Preferred` policy | the same |
| The minimum width the full text | the same |

UI only.

**Windows result: not the cause.** `windows-desktop` on `cd1954f` (job 117661026863) failed with
exactly the same lines (host 273 px, viewport −1). The toggle was not what held the panel on Windows;
the eliding buttons stay as the owner's choice, but they did not fix the check.

Ruled out on Linux: a two-column form whose rows may wrap does not need label plus field (a 130 px
field minimum left the body's minimum at 169 px). The 273 px is 8 + 123 + 8 + 126 + 8, the label
column plus the remaining field width, so it does not name its source. When a narrow check fails,
`aligned` now also prints:
- the dock's width and minimum;
- the body's width and minimum hint;
- the tab widget's and tab bar's hints;
- the six widest visible widgets above the pages.

The next Windows run names the widget that holds the panel.

## Round 5: a wrapped text's minimum held the panel (D244) — 2026-10-18

The diagnostic from `5c40b8e` named the widget (job 117667101947, identical at 1×, 1.5× and 2×):

```
narrow: dock 273 (minimum 273), body 273 (hint 273), tabs hint 153, tab bar hint 149
  257 QLabel editorEffectiveBehaviour
  98 QWidget
```

**Cause.** `editorEffectiveBehaviour` is the word-wrapped text under the behaviour type. On Windows
(Qt 6.5.3) it asked for a 257 px minimum. On Linux, Qt 6.4.2 asks for its longest word, 69 px, and
Qt 6.8 (PySide6, also on Linux) asks for 63 px. The text itself is the same. That one minimum held
the body at 273 px and the floating dock at 273 px, wide enough for two columns, so the top forms
never stacked. The `<= 220` check had passed only because it was read before Windows applied the
window's minimum.

Nothing in the editor sets that label's minimum width. Margin, indent, a frame, text selection and
rich text each left Qt's value at the longest word plus extras on Linux. The Windows path that gives
257 was not reproduced.

**Fix.** `ReflowLabel::minimumSizeHint` for a wrapped plain text is now its longest word: the width
`QFontMetrics::boundingRect` gives at a 1 px wrap width, plus its margins and indent. It is never
more than Qt's own value. Form labels keep their column rule (D205).

**Rows** (1×, 1.5×, 2×). In the narrow fixture, in both languages, every visible wrapped text above
the pages must ask for no more than its longest space-separated word plus 8 px. On failure it prints
its minimum, Qt's value, any set minimum and the word.

- On Linux it passes before and after: Qt already gives the word here, so the row cannot fail first
  locally. Its failing evidence is the Windows run above (257 against a word of about 70 px).
- **Emulation:** with Qt's value inflated by 300 px for every wrapped text, the build passes with
  the cap. Without the cap it fails, and the Linux panel cannot narrow at all ("Narrow fixture did
  not take effect").

Unconfirmed on Windows until `windows-desktop` runs. UI only.
