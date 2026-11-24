# Decision index

This is navigation, not a second decision log. The complete decisions and reasoning are
in [RECORD](RECORD.md) (D600 onward; D1–D599 frozen in [RECORD-D001-D199](RECORD-D001-D199.md), [RECORD-D200-D399](RECORD-D200-D399.md) and [RECORD-D400-D599](RECORD-D400-D599.md), D486). Read the applicable rows
and their evidence before reopening a choice. IDs remain stable even when records move.

| Topic | Start with these decisions |
|---|---|
| Desktop automation and AI control | D574, D573, D572, D571, D570, D569, D568, D567, D566, D565, D564, D563, D562, D561, D560, D559, D558, D350, D349, D348, D347, D346, D345, D344, D336, D335 |
| Product scope, stack, naming and owner gates | D612, D602, D601, D557, D556, D555, D554, D553, D552, D551, D550, D549, D548, D547, D546, D545, D544, D543, D542, D541, D540, D539, D538, D537, D536, D535, D534, D533, D532, D531, D530, D529, D528, D527, D516, D505, D378, D290, D289, D288, D287, D286, D285, D284, D278, D270, D269, D268, D267, D266, D265, D264, D263, D262, D261, D260, D259, D201, D178, D177, D145, D175, D1–D17, D34, D38, D49, D51–D53 |
| Macro assignment (S2, M10.1) and four-step (M10.2) | D485, D484, D483, D482, D481, D480, D479, D478, D477, D476, D475, D474, D473, D472, D471, D470, D469, D468, D277, D276, D275, D274, D273, D272, D271 |
| Meso engine (M11) | D293, D292, D291, D158 |
| Formats, validation and diagnostics | D18a–D18d, D19a–D19d, D21, D54–D56 |
| Link/Connector geometry and lifecycle | D20–D23, D26, D55, D72–D80, D86, D106–D110, D114–D118 |
| Demand and routing | D501, D234, D233, D229, D228, D162, D161, D160, D158, D157, D156, D155, D154, D153, D142, D25, D32–D33, D37, D42–D46, D71, D93–D94, D111–D113 |
| Signals, conflicts, Stop/Yield and queues | D607, D443, D442, D441, D440, D439, D438, D437, D436, D258, D257, D256, D255, D254, D253, D251, D250, D249, D248, D247, D246, D241, D240, D239, D238, D237, D232, D231, D230, D227, D147 (supersedes D36 for projects), D35–D36, D40, D47–D48, D50, D57–D69 |
| Following, commitment and lane changes | D619, D618, D613, D609, D608, D459, D458, D457, D456, D455, D454, D453, D452, D451, D450, D449, D448, D447, D446, D445, D444, D258, D257, D256, D255, D254, D253, D252, D251, D250, D249, D248, D247, D246, D241, D240, D239, D238, D237, D236, D235, D234, D233, D232, D231, D230, D229, D228, D227, D226, D225, D224, D223, D222, D221, D220, D219, D218, D217, D216, D215, D214, D213, D212, D211, D210, D209, D208, D207, D206, D204, D203, D202, D189, D188, D187, D186, D185, D184, D183, D182, D181, D180, D179, D178, D177, D176, D166, D165, D138, D137, D136, D135, D129, D127, D126, D125, D124, D123, D122, D121, D120, D69, D71, D87–D95, D98, D101–D102, D105, D108 |
| UI, gestures, display and results | D620, D617, D616, D615, D614, D611, D610, D606, D605, D604, D603, D513, D509d, D509c, D509b, D509a, D509, D508, D507, D504, D503, D500, D499, D498, D497, D467, D466, D465, D464, D463, D462, D461, D460, D435, D434, D433, D432, D270, D268, D265, D264, D263, D262, D261, D260, D259, D245, D244, D243, D242, D205, D200, D199, D198, D197, D196, D195, D194, D193, D192, D190, D151, D150, D149, D148, D146, D144, D141, D139, D134, D133, D132, D131, D130, D128, D24, D30, D39–D40, D60–D65, D81, D83–D84, D96–D97, D100, D102–D104, D116 |
| Performance, checks and documentation | D526, D525, D524, D523, D522, D521, D520, D519, D518, D517, D515, D514, D512, D511, D510, D506, D502, D496, D495, D494, D493, D489, D488, D487, D486, D283, D282, D281, D280, D279, D164, D163, D159, D152, D143, D140, D27–D29, D31, D70, D82, D85, D91, D99 |

## Reading a historical choice

Check its date, reasoning and failure condition, then follow any later decision that
supersedes it. For example, D15 supersedes the original D3/D4 stack, D11 supersedes
the D9/D10 names, D107 replaced D106's runtime path choice, and D114 supersedes D107 after the owner
requested interior edits to steer vehicles. A historical row is
not automatically the current contract. Current interfaces live in the reference docs.

## All decision IDs

D1–D599 are indexed at the top of their frozen files, [RECORD-D001-D199](RECORD-D001-D199.md#index),
[RECORD-D200-D399](RECORD-D200-D399.md#index) and [RECORD-D400-D599](RECORD-D400-D599.md#index) (D486). D600 onward:

For continuous vehicle-pose work, also read [VEHICLE_POSE](../reference/VEHICLE_POSE.md) and the
2026-10-05 phase entries in PROGRESS; those entries do not invent new D-numbers.

[M14 decisions D386 onward](M14.md) — junction templates, signals, UTDF and their owner reviews.




- [D422](RECORD-D400-D599.md#d422) — A9 B41: explicit per-Node session-only manual count windows, shared batch/T guards, manual/model labels and comparison CSV; no stored field.
- [D432](RECORD-D400-D599.md#d432) — all 34 owner-requested UI audit corrections: safe staged forms/exports/document refresh, task workspaces, responsive Results, readable Lab and session density.
- [D444](RECORD-D400-D599.md#d444) — lateral crossing at the window end: one target-lane lookup (`targetSectionAt`) for the start and the crossing, one `spanHolds` test, and the whole body kept in its lane on a side it cannot cross (replaces D189's centre clamp).
- [D445](RECORD-D400-D599.md#d445) — a straddler searches a covered lane only on the section level with its front (fixes audit Finding 2's gap-0 leader and clamps).
- [D446](RECORD-D400-D599.md#d446) — while a lateral change is under way, D207's swap partner extends to a rear stub too close to accept the changer (`acceptsAsLeader`, shared with D208); `yieldMutual` moves to `lane_swap.cpp`.
- [D447](RECORD-D400-D599.md#d447) — a body over the line whose lateral move is blocked in 2D steps back like D208's refused cover (one step-back path); revises D206's BA53.
- [D448](RECORD-D400-D599.md#d448) — the lane-change start's lateral safety filter reads the target lane by `targetPartAt` (D444's to-range rule) through `followerFilterOn`.
- [D449](RECORD-D400-D599.md#d449) — the weaving lab: a synthetic Type A weaving fixture with an off/on sweep (rows BA179–BA183); at full demand the weave end locks (open).
- [D450](RECORD-D400-D599.md#d450) — at a shared dead end, a swapping pair is kept while the front one closes in, and no change lands in a waiting partner's place (BA183–BA185).
- [D451](RECORD-D400-D599.md#d451) — switched on, a change under way counts as on its target route for D207, and an approaching partner is taken where it will wait (BA186–BA187).
- [D452](RECORD-D400-D599.md#d452) — no courtesy for a change that may not land: a follower no longer holds back for a changer whose landing D450 (b) refuses (BA188–BA189).
- [D453](RECORD-D400-D599.md#d453) — switched on, a change under way that may not land claims nothing: no outward step, no partner yield, no reserved place (BA190–BA191).
- [D454](RECORD-D400-D599.md#d454) — the yielding partner of a swapping pair brakes up to `maxDecelerationCooperativeBraking` once the front one closes in (BA192–BA193).
- [D455](RECORD-D400-D599.md#d455) — gap seeking, slice 1: cooperative braking helps a refused stub anywhere in its span, its swap partner excepted (BA194–BA195; A38 revised).
- [D456](RECORD-D400-D599.md#d456) — a stub vehicle's dead end is a second obstacle: a nearer moving leader no longer hides it (BA196–BA197).
- [D457](RECORD-D400-D599.md#d457) — the lateral lab: a shipped four-scene project carrying the lateral keys as lab placeholders, a lab-only exception to D178 (BA198–BA203).
- [D458](RECORD-D400-D599.md#d458) — gap seeking, slice 2: switched on, a refused changer falls back behind its blocker, braking comfortably at most (BA204–BA205).
- [D459](RECORD-D400-D599.md#d459) — two-entry weaves: a lane family may start a joined chain on a downstream Link, with a lead-in and a rear check over its feeders (BA179 revised, BA206–BA208).
- [D460](RECORD-D400-D599.md#d460) — canvas review fixes: thin Node outlines, real-size signal heads, a centred clear button, input rates on their lanes (amends D425/D426/D432/D434/D435).
- [D461](RECORD-D400-D599.md#d461) — canvas display fixes: Fit ignores hidden rate labels, small signal heads are their stop line, the road edge follows road width, Connector heads hang beside the Link edge.
- [D462](RECORD-D400-D599.md#d462) — owner choices: no marker for hidden rates, one-page object groups lose the tab strip and title the dock, movement names stay as authored.
- [D463](RECORD-D400-D599.md#d463) — UI round 2: Properties fits its panel, the summary clips on a row, short labels with hints, a one-line scope banner, "—" in empty result cells.
- [D464](RECORD-D400-D599.md#d464) — a network image PNG shows every input rate (grey-framed box) and every signal head box; the canvas-lod threshold test measures real decoration again.
- [D465](RECORD-D400-D599.md#d465) — canvas style slice C: shipped car/pickup/van palettes, a screen-offset vehicle shadow, no Node outlines or unpinned rates over a run.
- [D466](RECORD-D400-D599.md#d466) — signal head boxes past the stop line on one straight arm, a dark casing on run vehicles (contrast rule kept in wireframe).
- [D467](RECORD-D400-D599.md#d467) — review fixes: high-DPI shadow, one palette parser, poles tested off the road; dark casings and the head's hit place kept.
- [D468](RECORD-D400-D599.md#d468), [D469](RECORD-D400-D599.md#d469), [D470](RECORD-D400-D599.md#d470), [D471](RECORD-D400-D599.md#d471), [D472](RECORD-D400-D599.md#d472), [D473](RECORD-D400-D599.md#d473), [D474](RECORD-D400-D599.md#d474), [D475](RECORD-D400-D599.md#d475), [D476](RECORD-D400-D599.md#d476), [D477](RECORD-D400-D599.md#d477), [D478](RECORD-D400-D599.md#d478), [D479](RECORD-D400-D599.md#d479), [D480](RECORD-D400-D599.md#d480), [D481](RECORD-D400-D599.md#d481), [D482](RECORD-D400-D599.md#d482), [D483](RECORD-D400-D599.md#d483), [D484](RECORD-D400-D599.md#d484), [D485](RECORD-D400-D599.md#d485) — M10.2 trip generation contract (rate sets as data, category and regression methods, attractions balanced to productions, unit carried); owner answers 1a–8a; G0 read Ortúzar Ch. 4, fixture Example 4.1, not 4.5; G1 engine, run, outputs and CLI; G2 owned sets (schema 66), zone attributes, Trip generation page; M10.3 distribution contract after reading Ortúzar Ch. 5; answers 1a–9a and D1 engine, skim, run, outputs and CLI; D2 owned deterrence sets and cost matrices (schema 67), dialogs, Trip distribution page; D3 singly constrained contract and implementation; M10.4 mode choice contract after reading Ortúzar §5.4.4 and Ch. 6–7; E1 engine, period factors (schema 69), run, outputs and CLI; E2 owned sets, Mode choice page, Write OD matrices (schema 70); nested logit contract and N1, nests at schema 71 with a Nests dialog; destination-constrained purposes (schema 72); the procedure sequence contract and P1 (schema 73, `--procedure`) and P2 (dialog and page); [D486](RECORD-D400-D599.md#d486), the decision record split every 200 IDs.








[D601](RECORD.md#d601) — M3.2.7d's owner exercise keeps steps 1, 3–4 and the verdict; priority flip and save/reopen/replay are automated.
[D602](RECORD.md#d602) — Windows owner sheet triaged: 17 items kept, 6 closed, 31 old looks carved out to ROADMAP.
[D603](RECORD.md#d603) — waiting lines with Stop or Yield are hidden outside the right-of-way tools (amends D590).
[D604](RECORD.md#d604) — Vissim-style priority rules drafted as PRIORITY_RULES; waits on the owner's §8.
[D605](RECORD.md#d605) — block-selecting tables draw one outline around each selected block.
[D606](RECORD.md#d606) — priority rules follow Vissim: max speed on the headway test, class filters on line and markers, 3.0 s / 5.0 m defaults.
[D607](RECORD.md#d607) — a conflict area's visibility, 100 m by default; unchanged results at 100 m.
[D608](RECORD.md#d608) — acceleration by speed and temporary lack of attention, as Vissim; absent by default; lost time follows the standstill acceleration.
[D609](RECORD.md#d609) — no shipped acceleration curve: each study sets it; next A2, then G3, then priority rules.
[D610](RECORD.md#d610) — conflict areas green, red and olive as Vissim; a centre arrow on each band.
[D611](RECORD.md#d611) — pointing shows a road's edges in black with arrows, selection in blue; a selected area shows its roads.
[D612](RECORD.md#d612) — M3.2.7d passes on attempt 2; M3.2 waits on its diverge row; next G3.
[D613](RECORD.md#d613) — a vehicle stuck at its dead end for `diffusionTime` is removed and counted, as Vissim; off by default.
[D614](RECORD.md#d614) — priority rules slice 1: schema 83 marker form, max speed and class filters in the engine; Stop at the line next.
[D615](RECORD.md#d615) — a Stop on a priority rule's line means a full stop before the gap test; the control may name no area.
[D616](RECORD.md#d616) — the Stop line tool adds a priority rule's markers with its line picked; markers follow their roads.
[D617](RECORD.md#d617) — priority rules show headway bands and links, list in the Conflict areas tab with a dialog, and warn when no route reaches a marker.
[D618](RECORD.md#d618) — a waiting minor driver accepts less: Aimsun's gap reduction over its yield time, SUMO's impatience at conflict areas; off by default.
[D619](RECORD.md#d619) — the gap reduction in the area and rule dialogs; the waiting-acceptance lab for the owner's look.
[D620](RECORD.md#d620) — conflict-area arrows on the lane centre and only when selected; Vissim's yellow for unauthored areas, red-red for undecided.
[D621](RECORD.md#d621) — GCC/Clang Debug builds compile with `-Og`: the desktop suite in 140 s instead of 331 s, results byte-identical.
[D622](RECORD.md#d622) — M3.2 diverge rows A62–A66: order, FIFO and replay pass; A64 fails, bodies overlap inside a split under a branch queue (owner item 53).
[D623](RECORD.md#d623) — red-red branching enforced: a body inside the split occupies the sibling branch; A64 passes.
[D624](RECORD.md#d624) — branching stays read-only; a Vissim-like visibility waits for a study and a cited default.
[D625](RECORD.md#d625) — M3.2 closed by the owner; carved looks, clamps, M3.4 and M6 stay open.
[D626](RECORD.md#d626) — the leader search bisects long routes to the first part not behind the vehicle: benchmark −5.9 %, same leaders.
[D627](RECORD.md#d627) — the by-rear leader order kept across ticks, measured and not shipped: study −8.8 %, benchmark +0.6 to +0.8 %.
[D630](RECORD.md#d630) — the root `CMakeLists.txt` split into `cmake/` includes by responsibility (498 → 194 lines); generated build byte-identical.
[D634](RECORD.md#d634) — Results tabs fit at 1024 in Thai without shorter labels (proved, every tab forced); the clamp note opens with not-validated.
[D635](RECORD.md#d635) — insertion rebuilds span buckets only for a later candidate, not after every arrival: benchmark −0.81 %, same events.
[D636](RECORD.md#d636) — `procedures` (73) and feedback (75) in the composed schema; step rows by kind, exclusive assignment; zero disagreements.
[D637](RECORD.md#d637) — the W99 contract before code: demo thresholds and regimes, PTV meanings, seven recorded deviations, rows BA224–BA232.
[D638](RECORD.md#d638) — W99 slice 1: the pure function, thresholds and key list; BA224, BA225, BA227 pass; not composed.
[D639](RECORD.md#d639) — W99 slice 2: schema 85 codec and composed-schema row; Run refuses a used w99 behaviour; the dialog declines it until its slice.
[D640](RECORD.md#d640) — W99 slice 3: the behaviour dialog edits a w99 behaviour (ten empty fields, range-checked); the lab refuses Apply to one
[D641](RECORD.md#d641) — W99 slice 4: the hashed driver trait zW (index 15), only when a w99 behaviour exists; legacy runs byte-identical
[D642](RECORD.md#d642) — W99 slice 5: composed into the tick through follow with aPrev and zW; the Run refusal lifted
[D643](RECORD.md#d643) — the behaviour dialog names the first empty or out-of-range key in order (the owner's F7 finding); a model-neutral range message
[D644](RECORD.md#d644) — W99 does not halve heavy vehicles' acceleration; the type's own acceleration limit bounds it (owner)
[D645](RECORD.md#d645) — W99's CC1 may be a per-driver cumulative curve (u(16)), the dialog's default mode; a new schema floor; contract only (BA233–BA237)
[D646](RECORD.md#d646) — CC1 curve slice 1: codec and schema 86; Run and the dialog refuse a curve until their slices (BA233)
[D647](RECORD.md#d647) — CC1 curve slice 2: the per-driver share u(16), the shared curve inverse and composition; a curved behaviour runs (BA234–BA236)
[D648](RECORD.md#d648) — CC1 curve slice 3: the dialog's Fixed / Distribution (default) and curve table; the decline removed (BA237)
[D649](RECORD.md#d649) — BA231 recorded: W99 queue discharge at three timesteps; headway falls with dt; standing-creep clamps traced, not fixed
[D650](RECORD.md#d650) — W99's standstill creep waits until one creep fits before CC0 (`CC7·dt²`); the every-tick clamps gone (BA238)
[D651](RECORD.md#d651) — Stopping at the line: contract and measured options (L1, L3, floors); L3F recommended as `clampRules.lineRest`, M0 untouched; the owner chooses
[D652](RECORD.md#d652) — LINE_STOP L3F implemented: `clampRules.lineRest` (schema 87), every W74/W99 and amber prototype stops at the line; dead ends stay leaders; M0 bytes kept
