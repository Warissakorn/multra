# M3.7 slice 1: lateral codec and cross-section compile evidence — 2026-10-15

Branch `claude/amazing-allen-pxg1co` on `3dd2133` plus this slice (D187). Local host: Linux,
GCC 13.3.0, Ninja, nlohmann/json 3.11.3. `--preset headless`, CTest and
`cmake --build build/headless --target check` (architecture, file sizes and docs included).
**Linux headless only**; desktop and Windows evidence belongs to the PR's native CI. Contract:
[LATERAL](../reference/LATERAL.md) §3, §8, §9, §12 step 1; decisions D180, D187.

## Rows (`tests/lateral_codec_tests.cpp`, ctest `lateralcodec`, 6 tests)

| Row | Case | Asserts |
|---|---|---|
| BA45 | Owned behaviours without lateral keys | Schema below 33; reopen and re-save byte-identical |
| BA45 | All three keys on the prototype behaviour and on an unused `w74` one | `schemaVersion` 33; every key written on both models; parse gives an equal document and identical bytes |
| BA45 | The same file at schema 32; `maxLateralSpeed` as text; an unknown key `lateralDrift` | `EDIT_UNSUPPORTED_FIELD` at `behaviours[i].lateralMinDistanceStanding`; `INVALID_BEHAVIOUR_PARAMETER` at its path; `EDIT_UNSUPPORTED_FIELD` |
| BA45 | Each one of the three missing, on the used and on the unused behaviour; two missing | `compileDocument` refuses with `INVALID_BEHAVIOUR_PARAMETER` at the first missing key's path (exactly one issue for two missing) |
| BA45 | Negative, non-finite, and (rate) 0 or above 3 m/s, used and unused | Refused at a path ending `.<key>`; 0 m, 0 m and 3 m/s load |
| BA45 | External catalog entry on the prototype; the `w74` branch | Keys read on both models |
| BA45 | Left- and right-hand networks: a two-lane Link (3.5 m, 3.0 m), a Connector onto a one-lane 4.0 m Link | Off: no cross-sections, no `drivingSide`. On: one per segment, `drivingSide` as authored; lane centres equal `laneCentreOffset` and the y of `laneGeometry` on an x-axis Link, 3.25 m apart with the sign of the side; widths; both lanes share a carriageway; the Connector path has its own, centre 0, widths 3.5 → 4.0; `createSimulation`'s id sort keeps every cross-section with its segment; a short table or a missing `drivingSide` fails validation |
| BA39 | Four-leg project, seed 42 | No cross-sections, no `drivingSide`, no keys; with all keys added to every behaviour the run still matches tick by tick (no tick reads them in slice 1) |

Mutation checks (reverted): dropping the "all three" rule fails the partial-set test; dropping the
canonical permutation fails the compile test.

Also kept green: the CLI pin (`trafficsim-cli 42` = 45 completed / 31.75870154816726 s / 1 clamp),
the frozen baselines, perception and W74 groups. `laneGeometry` now calls the extracted
`laneCentreOffset`, the same expression, so drawn and compiled lanes are the same doubles. Nine
fixtures that used schema 33 as the refused "future" version now use 34: fixture maintenance, no
behaviour change.

## Performance (switch off, LATERAL §11)

`trafficsim-engine-benchmark 40 1800`, Release, five interleaved pairs against `3dd2133` on the same
host: medians **3436 ms before, 3380 ms after**, inside noise (both completed 12065 trips, peak 626).
Nothing in the tick changed; the only new runtime work is an empty-vector test in
`createSimulation`'s canonicalisation.

## Not covered here

The Driving behaviours dialog does not show the keys; its `w74` path keeps them
(`behaviour_editor.cpp`), not covered by a Qt test. A Link's lane sections share one carriageway;
a test with an interior cut (non-zero `linkStationStart`) is left to slice 2, whose spans read it.
No span, predicate, admission or display reads the data: BA40–BA44 and BA46 remain pending.
