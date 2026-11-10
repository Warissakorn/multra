# Random streams per input (contract, proposed)

**Status on 2026-11-10: contract only, no code. The owner's choices in §7 are open; nothing in
`src/` changes until they are answered** (D496). Milestone status lives in [ROADMAP](../ROADMAP.md).
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

W74 traits and red runners do not draw: they hash `(seed, vehicle id, index)` (D129, D238).

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
| Routing decision | none | hashed | `u = hash(seed, vehicle's input key, vehicle's ordinal within its input, decision index)`, as W74 traits are hashed; no state, so no order dependence |

State lives where the consumer's state already is: one `std::uint32_t` in `InputState` and one in
`LotState`. A vehicle carries its ordinal within its input (a `std::uint32_t` on
`PendingVehicle`) for the hashed draws.

**Scope limit, stated rather than hidden.** W74 traits and red runners still hash the global
vehicle id, which shifts when another input generates earlier. Re-keying them by
`(input key, ordinal)` is the owner's choice 2; without it, `perInput` scenarios that use W74 or
red running keep that one dependence.

**Renaming an input changes its stream.** The id is the key; a rename is a different stream by
construction.

## 5. What the user sees

Nothing in the editor's controls. A run's summary and each CSV gain one field,
`randomStreams`, so a report states which mapping produced it. Results of a `shared` and a
`perInput` run of the same seed differ and are never compared as one batch: `compareBatches`
refuses two batches with different values, as it refuses different seed lists.

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

## 8. Slices, once answered

1. Rows IS1–IS9 written and failing where they should; the switch in core, the scenario codec
   and the summary/CSV field.
2. Input and lot streams; hashed routing draws.
3. The project switch, its schema step and Run settings control (by choice 1).
4. Choices 2 and 3, if booked.
