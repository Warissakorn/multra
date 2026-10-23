# Dialog heights against short screens — 2026-10-23

Branch `claude/adoring-feynman-63tgnt` on `main` `048cc89`, plus D318. Local host: Linux, GCC 13.3.0,
Ninja, Qt 6.4.2 offscreen, `--preset desktop`. **Linux only;** Windows paint and high-DPI belong to
`native.yml` and the owner. Decision: [D318](../decisions/RECORD.md#d318).

## Method

The UI suites (`ctest --preset desktop -R ui`, 61 tests) were run with an `LD_PRELOAD` shim that
wraps `QDialog::exec()` and logs each dialog's object name, `sizeHint()` and `minimumSizeHint()`.
The shim is a scratch tool and is not committed. It sees every dialog a UI test opens through
`exec()`; dialogs that no test opens are not measured.

A 1366×768 laptop, or 1080p at 150 % scaling, leaves about 650–690 px under a title bar and a task
bar. A dialog whose **minimum** height is above that cannot show its buttons at all.

## Before and after (height in px; the largest instance per dialog)

| Dialog | Minimum before | Natural before | Minimum after | Natural after |
|---|---|---|---|---|
| `editorBehaviourEditDialog` | **1378** | 1418 | 330 | 588 (Prototype); one W74 tab scrolls |
| `editorCatalogTypeDialog` (vehicle type) | **696** | 818 | 124 | 822, capped on open |
| `editorInputDialog` | 500 | 740 | 124 | 660, capped on open |
| `editorDecisionDialog` | 452 | 694 | 124 | 746, capped on open |
| `editorCoarseDialog` | 334 | 606, one row per leg | 124 | 608, capped on open |

Every other dialog measured had a minimum of 398 px or less and a natural height of 552 px or less
(the signal controller is the tallest). Their long parts are tables that scroll by themselves, so
they are unchanged.

The behaviour editor at 1366×768 (`QT_QPA_PLATFORM=offscreen:configfile=…`, one screen 1366×768):
- Prototype: 640×579, all four tabs readable, Confirm visible.
- W74: capped at 691 px (90 %), its *Car following* tab scrolls, Confirm visible.

## Rows

`requireFitsScreen` (`tests/dialog_fit_support.hpp`) checks four things:
- the minimum height is at most 480 px;
- Confirm is not inside a scroll area;
- the dialog opened no taller than its screen;
- Confirm lies inside the dialog.

It does not depend on the screen, so the Windows run checks the same property.

| Test | Dialog |
|---|---|
| `behaviour-library-ui` | Behaviour editor, Prototype and W74 |
| `demand-catalog-ui` | Vehicle type |
| `demand-edit-ui` | Input and decision |
| `road-ui` | Coarse Node values |

Failure first: with the five product files reverted and the tests kept, `behaviour-library-ui`,
`demand-catalog-ui` and `demand-edit-ui` fail ("cannot shrink to a laptop screen"). `road-ui` passes:
the coarse and decision rows are guards against growth, not reproductions, because both minimums
were already under 480 px.

## Correction: heap corruption in the behaviour editor (found 2026-10-23, same day)

`behaviour-library-ui` aborted with `corrupted double-linked list` in 2 of 7 parallel runs after
D318. Valgrind found invalid writes in every run, inside Qt 6.4.2's `QFormLayout::setGeometry`. The
writes came from `setRowVisible` while the shown dialog switched its model: the form was laid out
again between a row's label and its field. Fix: each form's layout is disabled while the rows
change, then re-enabled and activated. After the fix: valgrind 0 errors; 12/12 sequential and 6/6
parallel `-R ui` runs pass.

A crashed or killed UI run leaves its recovery file under `~/.qttest/share/<suite>/recovery`.
The next local run of that suite then waits at the recovery dialog until ctest times it out
(60 s). CI starts clean.
