# M3.5 slice 1: perception codec evidence — 2026-10-15

Branch `claude/eloquent-carson-qgorzf` on `e5d0e0a` plus this slice (D181). Local host: Linux,
GCC 13.3.0, Ninja, nlohmann/json 3.11.3, Qt 6.4.2 (offscreen). `--preset desktop` 139/139 CTest
through `cmake --build build/desktop --target check`, which also runs architecture, file sizes and
docs. **Linux only**; Windows evidence belongs to the PR's native CI. Contract:
[PERCEPTION](../reference/PERCEPTION.md) §6, §9 step 1; decisions D179, D181.

## Rows (`tests/perception_codec_tests.cpp`, ctest `perceptioncodec`, 5 tests)

| Row | Case | Asserts |
|---|---|---|
| BA37 | Owned behaviours without perception keys | Schema below 32; reopen and re-save byte-identical |
| BA37 | All five keys on the prototype behaviour and on an unused `w74` one | `schemaVersion` 32; every key written on both models; `observedVehiclesAhead` written as a JSON integer; parse gives an equal document and identical bytes |
| BA37 | The same file at schema 31 | `EDIT_UNSUPPORTED_FIELD` at `behaviours[i].lookAheadDistance` |
| BA37 | `observedVehiclesAhead` as `2.0` (asserted to be a float first); `reactionTime` as text; an unknown key `lookSideways` | `INVALID_BEHAVIOUR_PARAMETER` with path; same code; `EDIT_UNSUPPORTED_FIELD` |
| BA37 | Each key out of range, non-finite or (for the integer key) fractional, on the used and on the unused behaviour | `compileDocument` refuses with `INVALID_BEHAVIOUR_PARAMETER` at a path ending `.<key>`; the bounds 1 and 8 vehicles, 0 s and 2 s load |
| §6 | External catalog entry on the prototype and on `w74`; a dialog-equivalent `w74` rebuild | Keys read on both models; absent keys stay absent; the rebuilt behaviour equals the parsed one |
| BA30 | Four-leg project, seed 42, every behaviour given all five keys (asserted different first) | Vehicles, events, time and next vehicle id equal the run without them |

Also kept green: the CLI pin (`trafficsim-cli 42` = 45 completed / 31.75870154816726 s / 1 clamp),
the frozen baselines and every other group. Nine fixtures that used schema 32 as the refused
"future" version now use 33 (`cooldown`, `traveltime`, `behaviourlibrary`, `evaluationperiod`,
`w74codec`, `los`, `demandcounts`, `mapproject`, `zonescodec`): fixture maintenance, no behaviour
change.

## Not covered here

The Driving behaviours dialog does not show the keys yet; a save through its `w74` path keeps them
(`behaviour_editor.cpp`), covered above only at the `DriverBehaviour` level, not by a Qt test.
No motion reads the keys: BA31–BA36 and BA38 remain pending.
