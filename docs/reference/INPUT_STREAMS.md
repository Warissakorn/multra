# Random streams per input (contract)

**Status on 2026-11-12: slices 1–4 are done (D506, D510, D512).** A project carries the switch as
`definition.randomStreams` (schema 74); a new project is `perInput`, a file saved before keeps
`shared`; Run settings sets it; run, batch and comparison output state it and a comparison refuses
two mappings (IS8). Slice 4 (choice 2) is next. Milestone status lives in [ROADMAP](../ROADMAP.md).
Verification platform for the measurements quoted here: Linux, GCC 13.3.

## 1. The problem

A run has one random state, `SimState::randomState` (xorshift32, [SIMULATION](SIMULATION.md)
"Determinism"). Everything that draws takes the next number from it, in the order the tick
reaches it:

| Consumer | Where | Draws |
|---|---|---|
| An input's first arrival | `detail::initializeInputs`, `src/core/demand.cpp` | 1 per input, in input-id order |
| Each arrival | `detail::generateArrivals` → `samplePending` | desired speed, two for the driver factor, the next headway |
| A routing decision at its station | `applyRouteDecision`, `src/core/routing.cpp` | 1 per passage, in vehicle-id order within the tick |
| A lot's initial occupants | `initializeLots`, `src/core/parking.cpp` | type, the vehicle's three, dwell |
| A lot's dwell and exit choice | `dwellEnd`, `releaseDepartures` | 1 each (exit only with a choice) |

W74 traits and red runners do not draw: they hash `(seed, vehicle id, index)` (D129, D238); under
`perInput` the vehicle id there is replaced by its origin's key and ordinal (D512, §4).

So one edited input moves every later draw of the run. Three consequences:

1. **Scenario comparison loses precision.** Base and alternative run the same seeds
   ([BATCH §7](BATCH.md)), but an added site input gives every other input different arrivals, so
   the difference of the two carries the noise of both. Common random numbers are the usual remedy
   and need streams that do not depend on each other.
2. **An unrelated object changes a result.** D459's weaving sweep recorded it: adding scene 4
   moved the other scenes' served counts by up to 7 % at ×0.3, where nothing interacts.
3. **Draws depend on traffic.** A routing decision's draw is taken when the vehicle reaches the
   station, so a queue elsewhere reorders the stream.

## 2. What does not change

- Same scenario, seed, engine and toolchain give the same event stream (hard rule 2).
- `core/` imports nothing; no `<random>`, no wall clock, no unordered container.
- A scenario that does not carry the new switch (§3) runs to the bit as it does today, and
  `SeedStart::raw` keeps the pre-D159 mapping for the four TypeScript fixtures, which are never
  regenerated ([AGENTS](../../AGENTS.md)).
- Vehicle ids stay one counter in generation order: they are the run's output and every event
  names them.

## 3. The switch

A scenario-level value, `randomStreams`, with two values:

- `shared` (and absent): today's single stream. Byte-identical to the current engine.
- `perInput`: the streams of §4.

This follows the engine's own rule that an optional behaviour is off unless its key is set
(`src/core/README.md`). Which value a new document gets is the owner's choice 1.

## 4. Streams under `perInput`

A stream is an independent xorshift32 state, seeded once at `createSimulation`:

```
key    = fnv1a64(kind byte, then the object's id bytes)        // stable across edits elsewhere
state  = high32( splitmix64Mix( splitmix64Mix(seed + gamma) ^ key ) )   // 0 → 0x6d2b79f5
```

`splitmix64Mix` and `gamma` are the ones D159 and D129 already use (`src/core/detail.hpp`).
The id, not the position in the sorted list, is the key: adding an input changes no other
input's index-independent stream.

| Stream | Kind | Keyed by | Carries |
|---|---|---|---|
| Input | `I` | `VehicleInput::id` | its first arrival, and each arrival's desired speed, driver factor and next headway, in today's order |
| Lot | `L` | `ParkingLotSpec::id` | its initial occupants, dwell times and exit choices |
| Routing decision | `D` | hashed | `u = hash(seed, vehicle's origin key, vehicle's ordinal within its origin, decision key)`, as W74 traits are hashed; no state, so no order dependence. The decision is keyed by its id, not its index (D506), so an added decision leaves the others' draws alone |

State lives where the consumer's state already is: one `std::uint32_t` in `InputState` and one in
`LotState`, each beside a `generated` count. A vehicle carries its origin's key and its ordinal
there (`streamKey`, `ordinal` on `PendingVehicle`) for the hashed draws: an input's vehicles are
keyed by the input, a lot's initial occupants by the lot, and a parked vehicle keeps its origin
when it leaves by an exit input. Under `shared` all of these stay zero. The keys are computed once
in the scenario index (`keyOfInput`, `keyOfLot`). A vehicle placed by hand (the lab's placement)
still draws from the run stream under either mapping. A checkpoint under `perInput` also writes each
input's and lot's `stream` and `generated` and each vehicle's `ordinal`; a `shared` checkpoint keeps
its bytes. Code: `random_streams.hpp`, `demand.cpp`, `parking.cpp`, `routing.cpp`.

**Hashed traits (choice 2, D512).** W74 traits and the red-runner flag hash an identity in place
of the global vehicle id: `traitIdentity(id, origin, ordinal)` in `random_streams.hpp` is the id
when the vehicle has no origin key (every vehicle under `shared`, and one placed by hand) and
`mix(mix(origin) ^ ordinal)` otherwise, so an added input no longer moves another input's traits or
runners. The hash's indices (W74 §5, RED_RUNNING index 15) are unchanged. `samplePending` sets the
key and ordinal before it hashes the traits; a parked vehicle keeps its lot's identity.

**Renaming an input changes its stream.** The id is the key; a rename is a different stream by
construction.

## 5. What the user sees

One control: *Run settings → Random streams*, a list of the two written values (`shared`,
`perInput`; parameter names are never translated). A run's JSON and each CSV state the mapping
under `perInput` — `"randomStreams": "perInput"` and a `# randomStreams: perInput` line — in the
single run (`movementJson`/`movementCsv`), the `--seeds` batch and the comparison. `shared` writes
neither, so every output written before D510 keeps its bytes (IS1): an output without the field was
produced under `shared`. The bare scenario summary (`multra-cli 42`, `summaryJson`) is unchanged:
a scenario file cannot set the switch. Results of a `shared` and a `perInput` run of the same seed
differ and are never compared as one batch: `aggregate` refuses runs under two mappings,
`compareBatches` two batches, and `requireSameRandomStreams` refuses two projects before any run
(the CLI's `--compare` and the desktop's Compare page), as different seed lists are refused.

## 6. Acceptance rows (failure-first, before code)

| Row | Statement |
|---|---|
| IS1 | With the switch absent or `shared`, every existing fixture, frozen baseline and the four TS fixtures under `SeedStart::raw` are unchanged to the bit. |
| IS2 | `perInput`: two scenarios that differ only by an added input on a disjoint road give every other input the same sequence of `(scheduledTime, desiredSpeed, driverFactor)`. Under `shared` the same test fails, which is the row's failure-first proof. |
| IS3 | `perInput`: changing input A's volume leaves input B's sequence unchanged. |
| IS4 | `perInput`: two inputs with equal volume and windows produce different sequences (no key collision in the shipped projects and the test set). |
| IS5 | `perInput`: over seeds 42–81, each input's first arrival has mean within the SM rows' band of its mean headway (the D159 check, per stream). |
| IS6 | `perInput`: a routing decision's choice for a given vehicle is the same whether or not an unrelated road is congested. |
| IS7 | `perInput`: same-build replay is exact and a seed batch is identical for every thread count (D152). |
| IS8 | Summary JSON and CSV carry `randomStreams`; `--compare` refuses mixed values. |
| IS10 | `perInput`: an added input on a disjoint road leaves another input's W74 traits, in scheduled order, unchanged; a vehicle without an origin key keeps its id as its identity. |
| IS11 | `perInput`: the same for which of another input's vehicles run a red. |
| IS9 | Cost: `perInput` is within 1 % of `shared` in `multra-engine-benchmark` instructions (one state per input instead of one per run; no per-tick allocation). |

## 7. The owner's choices

1. **Default for new documents.**
   - (a) `shared` stays the default; `perInput` is set per project in Run settings.
   - (b) New documents get `perInput` at a schema step; a file saved before it keeps `shared`.
     Shipped projects change only when regenerated by a recorded decision. *Recommended:* old
     work reproduces, new work compares well, and no frozen number is re-pinned.
   - (c) Replace the single stream outright and re-pin every frozen number, `multra-cli 42`
     included. Simplest code, largest change to the record.
2. **Hashed traits.** Re-key W74 traits and red runners by `(input key, ordinal)` under
   `perInput`, or leave them on the global vehicle id (§4's limit).
3. **Comparison statistic.** With `perInput`, the comparison can pair seeds (a paired t interval
   on per-seed differences) instead of Welch. Booked with this slice, or later?
4. **Order.** Before the M6 benchmark runs (so the validated numbers are the kept ones), or after.

**Answered 2026-11-11 (D505):** 1 (b); 2 re-key by `(input key, ordinal)`; 3 later, as its own
slice; 4 before the M6 benchmark runs.

## 8. Slices, once answered

1. **Done (D506), with slice 2.** Rows IS1–IS7 and IS9 in `tests/input_streams_tests.cpp` (group
   `inputstreams`); each `perInput` row runs again under `shared` and expects the check to fail,
   which is its failure-first proof. The switch is `Scenario::randomStreams` in core.
   **Moved to slice 3:** the codec key and IS8's summary/CSV field and `--compare` refusal. Until
   the project can set the switch no run outside the tests is `perInput`, so a field written now
   would only ever read `shared`, and writing it unconditionally would change every existing
   output's bytes.
2. **Done (D506):** input and lot streams; hashed routing draws. IS6b (added): a lot's initial
   occupants and dwell ends do not move when an input is added.
3. **Done (D510):** `definition.randomStreams` at schema 74 (`random_streams_codec.*`), written only
   as `perInput`; `newProjectDocument` sets `perInput` (choice 1 b), so a new project saves at 74 and
   a reopened older one stays `shared` at its version; shipped projects are not regenerated. The Run
   settings list (`editorRandomStreams`, one Undo step), the IS8 output field, and the refusals.
   Rows in `tests/input_streams_project_tests.cpp` (group `inputstreamsproject`) and the Run
   settings step in `study-period-ui`.
4. **Done (D512):** choice 2, W74 traits and red runners re-keyed by origin and ordinal (§4).
   Rows IS10–IS11, each forced as the others.
5. **Next:** choice 3 (paired comparison) as its own slice.
