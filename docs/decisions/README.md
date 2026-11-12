# Decision index

This is navigation, not a second decision log. The complete decisions and reasoning are
in [RECORD](RECORD.md) (D400 onward; D1–D399 frozen in [RECORD-D001-D199](RECORD-D001-D199.md) and [RECORD-D200-D399](RECORD-D200-D399.md), D486). Read the applicable rows
and their evidence before reopening a choice. IDs remain stable even when records move.

| Topic | Start with these decisions |
|---|---|
| Desktop automation and AI control | D350, D349, D348, D347, D346, D345, D344, D336, D335 |
| Product scope, stack, naming and owner gates | D516, D505, D378, D290, D289, D288, D287, D286, D285, D284, D278, D270, D269, D268, D267, D266, D265, D264, D263, D262, D261, D260, D259, D201, D178, D177, D145, D175, D1–D17, D34, D38, D49, D51–D53 |
| Macro assignment (S2, M10.1) and four-step (M10.2) | D485, D484, D483, D482, D481, D480, D479, D478, D477, D476, D475, D474, D473, D472, D471, D470, D469, D468, D277, D276, D275, D274, D273, D272, D271 |
| Meso engine (M11) | D293, D292, D291, D158 |
| Formats, validation and diagnostics | D18a–D18d, D19a–D19d, D21, D54–D56 |
| Link/Connector geometry and lifecycle | D20–D23, D26, D55, D72–D80, D86, D106–D110, D114–D118 |
| Demand and routing | D501, D234, D233, D229, D228, D162, D161, D160, D158, D157, D156, D155, D154, D153, D142, D25, D32–D33, D37, D42–D46, D71, D93–D94, D111–D113 |
| Signals, conflicts, Stop/Yield and queues | D443, D442, D441, D440, D439, D438, D437, D436, D258, D257, D256, D255, D254, D253, D251, D250, D249, D248, D247, D246, D241, D240, D239, D238, D237, D232, D231, D230, D227, D147 (supersedes D36 for projects), D35–D36, D40, D47–D48, D50, D57–D69 |
| Following, commitment and lane changes | D459, D458, D457, D456, D455, D454, D453, D452, D451, D450, D449, D448, D447, D446, D445, D444, D258, D257, D256, D255, D254, D253, D252, D251, D250, D249, D248, D247, D246, D241, D240, D239, D238, D237, D236, D235, D234, D233, D232, D231, D230, D229, D228, D227, D226, D225, D224, D223, D222, D221, D220, D219, D218, D217, D216, D215, D214, D213, D212, D211, D210, D209, D208, D207, D206, D204, D203, D202, D189, D188, D187, D186, D185, D184, D183, D182, D181, D180, D179, D178, D177, D176, D166, D165, D138, D137, D136, D135, D129, D127, D126, D125, D124, D123, D122, D121, D120, D69, D71, D87–D95, D98, D101–D102, D105, D108 |
| UI, gestures, display and results | D513, D509d, D509c, D509b, D509a, D509, D508, D507, D504, D503, D500, D499, D498, D497, D467, D466, D465, D464, D463, D462, D461, D460, D435, D434, D433, D432, D270, D268, D265, D264, D263, D262, D261, D260, D259, D245, D244, D243, D242, D205, D200, D199, D198, D197, D196, D195, D194, D193, D192, D190, D151, D150, D149, D148, D146, D144, D141, D139, D134, D133, D132, D131, D130, D128, D24, D30, D39–D40, D60–D65, D81, D83–D84, D96–D97, D100, D102–D104, D116 |
| Performance, checks and documentation | D520, D519, D518, D517, D515, D514, D512, D511, D510, D506, D502, D496, D495, D494, D493, D489, D488, D487, D486, D283, D282, D281, D280, D279, D164, D163, D159, D152, D143, D140, D27–D29, D31, D70, D82, D85, D91, D99 |

## Reading a historical choice

Check its date, reasoning and failure condition, then follow any later decision that
supersedes it. For example, D15 supersedes the original D3/D4 stack, D11 supersedes
the D9/D10 names, D107 replaced D106's runtime path choice, and D114 supersedes D107 after the owner
requested interior edits to steer vehicles. A historical row is
not automatically the current contract. Current interfaces live in the reference docs.

## All decision IDs

D1–D399 are indexed at the top of their frozen files, [RECORD-D001-D199](RECORD-D001-D199.md#index) and
[RECORD-D200-D399](RECORD-D200-D399.md#index) (D486). D400 onward:

For continuous vehicle-pose work, also read [VEHICLE_POSE](../reference/VEHICLE_POSE.md) and the
2026-10-05 phase entries in PROGRESS; those entries do not invent new D-numbers.

[M14 decisions D386 onward](M14.md) — junction templates, signals, UTDF and their owner reviews.

[D419](RECORD.md#d419) — A9 B40 partly answered: expected Micro demand, explicit HV classification, whole-set sourced replacement and unresolved-demand refusal; native preview checkpoint, T/PHF/comparison pending.

[D420](RECORD.md#d420) — A9 B40 timing/batch choices answered; resolve stubs through all agreeing complete compiled lane-change targets, with origin fallback only without targets.

[D421](RECORD.md#d421) — A9 staged Fill, feature-selected schema-64 provenance and model-window Micro comparison; exact analytical CLI parity, one Undo and B41 manual window still pending.

- [D422](RECORD.md#d422) — A9 B41: explicit per-Node session-only manual count windows, shared batch/T guards, manual/model labels and comparison CSV; no stored field.
- [D432](RECORD.md#d432) — all 34 owner-requested UI audit corrections: safe staged forms/exports/document refresh, task workspaces, responsive Results, readable Lab and session density.
- [D444](RECORD.md#d444) — lateral crossing at the window end: one target-lane lookup (`targetSectionAt`) for the start and the crossing, one `spanHolds` test, and the whole body kept in its lane on a side it cannot cross (replaces D189's centre clamp).
- [D445](RECORD.md#d445) — a straddler searches a covered lane only on the section level with its front (fixes audit Finding 2's gap-0 leader and clamps).
- [D446](RECORD.md#d446) — while a lateral change is under way, D207's swap partner extends to a rear stub too close to accept the changer (`acceptsAsLeader`, shared with D208); `yieldMutual` moves to `lane_swap.cpp`.
- [D447](RECORD.md#d447) — a body over the line whose lateral move is blocked in 2D steps back like D208's refused cover (one step-back path); revises D206's BA53.
- [D448](RECORD.md#d448) — the lane-change start's lateral safety filter reads the target lane by `targetPartAt` (D444's to-range rule) through `followerFilterOn`.
- [D449](RECORD.md#d449) — the weaving lab: a synthetic Type A weaving fixture with an off/on sweep (rows BA179–BA183); at full demand the weave end locks (open).
- [D450](RECORD.md#d450) — at a shared dead end, a swapping pair is kept while the front one closes in, and no change lands in a waiting partner's place (BA183–BA185).
- [D451](RECORD.md#d451) — switched on, a change under way counts as on its target route for D207, and an approaching partner is taken where it will wait (BA186–BA187).
- [D452](RECORD.md#d452) — no courtesy for a change that may not land: a follower no longer holds back for a changer whose landing D450 (b) refuses (BA188–BA189).
- [D453](RECORD.md#d453) — switched on, a change under way that may not land claims nothing: no outward step, no partner yield, no reserved place (BA190–BA191).
- [D454](RECORD.md#d454) — the yielding partner of a swapping pair brakes up to `maxDecelerationCooperativeBraking` once the front one closes in (BA192–BA193).
- [D455](RECORD.md#d455) — gap seeking, slice 1: cooperative braking helps a refused stub anywhere in its span, its swap partner excepted (BA194–BA195; A38 revised).
- [D456](RECORD.md#d456) — a stub vehicle's dead end is a second obstacle: a nearer moving leader no longer hides it (BA196–BA197).
- [D457](RECORD.md#d457) — the lateral lab: a shipped four-scene project carrying the lateral keys as lab placeholders, a lab-only exception to D178 (BA198–BA203).
- [D458](RECORD.md#d458) — gap seeking, slice 2: switched on, a refused changer falls back behind its blocker, braking comfortably at most (BA204–BA205).
- [D459](RECORD.md#d459) — two-entry weaves: a lane family may start a joined chain on a downstream Link, with a lead-in and a rear check over its feeders (BA179 revised, BA206–BA208).
- [D460](RECORD.md#d460) — canvas review fixes: thin Node outlines, real-size signal heads, a centred clear button, input rates on their lanes (amends D425/D426/D432/D434/D435).
- [D461](RECORD.md#d461) — canvas display fixes: Fit ignores hidden rate labels, small signal heads are their stop line, the road edge follows road width, Connector heads hang beside the Link edge.
- [D462](RECORD.md#d462) — owner choices: no marker for hidden rates, one-page object groups lose the tab strip and title the dock, movement names stay as authored.
- [D463](RECORD.md#d463) — UI round 2: Properties fits its panel, the summary clips on a row, short labels with hints, a one-line scope banner, "—" in empty result cells.
- [D464](RECORD.md#d464) — a network image PNG shows every input rate (grey-framed box) and every signal head box; the canvas-lod threshold test measures real decoration again.
- [D465](RECORD.md#d465) — canvas style slice C: shipped car/pickup/van palettes, a screen-offset vehicle shadow, no Node outlines or unpinned rates over a run.
- [D466](RECORD.md#d466) — signal head boxes past the stop line on one straight arm, a dark casing on run vehicles (contrast rule kept in wireframe).
- [D467](RECORD.md#d467) — review fixes: high-DPI shadow, one palette parser, poles tested off the road; dark casings and the head's hit place kept.
- [D468](RECORD.md#d468), [D469](RECORD.md#d469), [D470](RECORD.md#d470), [D471](RECORD.md#d471), [D472](RECORD.md#d472), [D473](RECORD.md#d473), [D474](RECORD.md#d474), [D475](RECORD.md#d475), [D476](RECORD.md#d476), [D477](RECORD.md#d477), [D478](RECORD.md#d478), [D479](RECORD.md#d479), [D480](RECORD.md#d480), [D481](RECORD.md#d481), [D482](RECORD.md#d482), [D483](RECORD.md#d483), [D484](RECORD.md#d484), [D485](RECORD.md#d485) — M10.2 trip generation contract (rate sets as data, category and regression methods, attractions balanced to productions, unit carried); owner answers 1a–8a; G0 read Ortúzar Ch. 4, fixture Example 4.1, not 4.5; G1 engine, run, outputs and CLI; G2 owned sets (schema 66), zone attributes, Trip generation page; M10.3 distribution contract after reading Ortúzar Ch. 5; answers 1a–9a and D1 engine, skim, run, outputs and CLI; D2 owned deterrence sets and cost matrices (schema 67), dialogs, Trip distribution page; D3 singly constrained contract and implementation; M10.4 mode choice contract after reading Ortúzar §5.4.4 and Ch. 6–7; E1 engine, period factors (schema 69), run, outputs and CLI; E2 owned sets, Mode choice page, Write OD matrices (schema 70); nested logit contract and N1, nests at schema 71 with a Nests dialog; destination-constrained purposes (schema 72); the procedure sequence contract and P1 (schema 73, `--procedure`) and P2 (dialog and page); [D486](RECORD.md#d486), the decision record split every 200 IDs.
