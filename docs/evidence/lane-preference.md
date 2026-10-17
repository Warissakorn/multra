# B-7: lane preference by vehicle type — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1`, on `fea15b7` (D233's contract) plus this change (D234).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [LANE_PREFERENCE](../reference/LANE_PREFERENCE.md).

## Rows (`tests/lane_preference_tests.cpp`, ctest `lanepref`)

The rows use the shipped lane-change lab with owned catalogs:
- route-26 starts on a 2-lane Link, and route-27 and route-30 on 3-lane Links;
- route-30's kerb lane is a stub;
- its heavy-vehicle inputs are `input-32` and `input-34`.

"Before the change" is the same rows run against a stub where `laneSplit` ignores the limit.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA124 | No key | Heavy vehicles split 75/75 and ⅓ each | passes (it pins today's split) |
| BA125 | `kerbLanes: 1` on heavy vehicles | 150/0 and 200/0/0, while cars keep 500 per lane; over 300 s of a run every heavy vehicle starts on `route-26/lane-1` or `route-27/lane-1` | fails (75 vs 150) |
| BA126 | `kerbLanes: 2` | 100/100/0 on 3 lanes; the 2-lane Link is unchanged | fails |
| BA127 | `laneShares` {1, 3, 4}, then {0, 0, 5} | 50/150/0, then 100/100/0 (equal over the allowed lanes); a chain set without lane 0 keeps its split | fails |
| BA128 | Cars with `kerbLanes: 1` on route-30, whose kerb lane is a stub | All 1000 veh/h enter on lane 1. Over 300 s, more than 50 arrive on lanes 2 and 3 after a mandatory change, and none arrive on the stub | fails (333 vs 1000) |
| BA129 | `input-34` made routeless on `link-4` | Three paths without the key; one path carrying 200 with it | fails |
| BA130 | Codec | Writes schema 42 and round-trips; 41 is refused at `vehicleTypes[i].kerbLanes`; 0, 17, 1.5 and a string are refused (`INVALID_KERB_LANES`); a range fault is named at its path; the shared catalog reader takes the key; removing it returns the file to its old schema | written after the code; proved by mutation |

**Mutations,** each removed in turn:

| Mutation | Caught by |
|---|---|
| No parse | BA130 |
| No writer | BA130 |
| No schema-42 term | BA130 |
| No refusal below 42 | BA130 |
| No library key | BA130 (`EDIT_UNSUPPORTED_FIELD`) |
| No validation | BA130 |
| No routeless hook | BA129 |
| No compile hook | BA125–BA128 |

**Dialog (`demand-catalog-ui`).** It failed first on the old dialog (`Missing catalog control`).
It then checks:
- the spin box follows its tick box;
- ticking and setting 2 writes `kerbLanes: 2` in one revision, at schema 42;
- Undo and Redo work;
- unticking removes the key.

`check` **175/175**: 174 tests plus the new `lanepref`.

**First Windows run.** `windows-core` failed BA124–BA129: the compiled lab had three inputs or
fewer, even without the key. The rows' helper looped directly over a member of a temporary
`compileDocument` result, which MSVC appears not to keep alive. It now holds the snapshot in a local
first, as the other tests do. Linux was green either way. Not verified on Windows until the next CI run.

`parse.cpp` reached 502 lines on the first `check`. The reader's declaration moved to `json.hpp`,
and its call shares the `return` line.

## Shipped runs

**Byte-identical** (no shipped type carries the key):
- `trafficsim-cli 42` against D210's baseline;
- the `--seeds 42–51` output (without `buildCommit`) of all four shipped projects against D225's
  baseline.

**With the key:** a scratchpad copy of the data directory, with `kerbLanes: 1` on
`heavy-vehicle`. Seeds 42–81:

| Project | Without the key | `kerbLanes: 1` |
|---|---|---|
| `lane-change-lab`: mean delay (s) | 8.92 (±0.09) | 9.09 (±0.13) |
| Overtaking (2 lanes, 13 % heavy) | 9.93 (±0.26) | 10.34 (±0.35) |
| Three lanes (3 lanes, 12 % heavy) | 9.69 (±0.17) | 9.99 (±0.27) |
| Completed trips (mean) | 864.3 | 857.1 |
| Safety clamps | 0 | 0 |
| `m2.6-study-template` | 46.01 s, 0 clamps | identical output |

- **Lane-change lab.** Putting every heavy vehicle on the kerb lane adds 0.3–0.4 s on the two
  movements that carry them. The other three movements are within ±0.15 s.
- **M2.6 does not move.** Its inputs are routeless, with a routing decision on the entry Link, so
  the destination chooses the lane and no lane split applies (`demand_paths.cpp`, `byDestination`).
  Re-weighting there would change the destination shares. LANE_PREFERENCE §3 now says so.

## Performance (callgrind Ir, Release `-g`, seed 42, no key set)

Both builds were made in the same build directory.

| Project | `fea15b7` | This change |
|---|---|---|
| `four-leg-signalised` | 893.46 M | 890.32 M (−0.35 %) |
| `lane-change-lab` | 1255.79 M | 1250.36 M (−0.43 %) |

The rule runs once at compile. The fall is code layout, not work saved.

## Not covered here

- Keeping heavy vehicles out of outer lanes after entry, a preference by road, and weights per lane
  (§5).
- An entry Link whose routing decision chooses lanes by destination.
