# Archived session — branch audit and Results recovery

Historical entries moved whole from PROGRESS. Current work is in [NEXT](../NEXT.md).

## 2026-10-09 — Branch audit; Results discharge and clamp pages recovered (D144)

Every origin branch ahead of `main` was checked. `main` was re-rooted on 2026-09-28 (four root
snapshots), so the 38 older branches share no history with it and git's "ahead" counts mean
nothing; by content, every decision they name is in `main`, and the only files they hold that
`main` lacks are the retired M0 window (D24) and an unreferenced `data/fonts/NotoSansThai.ttf`.
PR #128 (`claude/dazzling-gates-df64jc`, closed unmerged) held the one piece of lost work:
Results inner pages for queue discharge and safety clamps (`9b81b17`, `cbfff89`). Its W74 and
M5 commits were already replayed or superseded. The two commits were cherry-picked onto the D142
branch, merged by hand with M5.6 (the batch view sits in the Movements page; `startBatch` selects
it), renumbered D135 → D144 (and a stray D134 → D139 in DRIVING_BEHAVIOUR), and `section-ui`
now finds the Results page as the other suites do. `check`: 111/111 (Linux, Qt 6.4.2 offscreen).
