# DIFFUSION — removing a vehicle stuck waiting for a lane change, as Vissim (G3, D613)

The owner's choices, 2026-11-21, on [realism-gaps-vissim-sumo-aimsun](../evidence/realism-gaps-vissim-sumo-aimsun.md)
G3 (in chat, as recommended): **as Vissim** (only a vehicle stopped waiting for a necessary lane change),
**remove and count separately**, **off by default, set per driver behaviour**. Not validated (rule 4).

## 1. What the other programs do

| | Vissim | SUMO | Aimsun Next |
|---|---|---|---|
| Who | A vehicle waiting at the emergency stop position for a lane change | Any vehicle that waited too long (jam, yield, wrong lane) | Any vehicle stationary longer than the maximum stationary time (Network Checker) |
| Action | Removed from the network | Moved to the next free edge of its route ("teleport") | Logged, or removed |
| Default | *Waiting time before diffusion*: 60 s per Louisiana DOTD's calibration sheet; not confirmed in PTV's help (unreachable from this session) | `time-to-teleport` 300 s; negative disables | Not read |

Sources: TxDOT's Vissim calibration guide (13.5.3.1: diffusion time is the longest a vehicle may wait
at the emergency stop distance before it is removed), SUMO's *Why vehicles are teleporting* and its
mailing list, Aimsun Next's *Dynamic network checks*, all read through search results on 2026-11-21.

## 2. Rule

- **Key:** `diffusionTime` on a driver behaviour, seconds in (0, 3600]. Absent: never, and every run
  is unchanged to the bit.
- **Who counts as stuck:** exactly the vehicle LANE_LOCKS calls *waiting at its dead end*
  (`waitingAtDeadEnd`, D578): standing on a stub route at its dead end for a mandatory lane change, not
  changing lanes this tick and not moving sideways. A vehicle waiting at a red head, a give-way line, a
  Stop, a lot gate or behind a queue is not stuck.
- **When:** after `diffusionTime` of consecutive such ticks (rounded up to whole ticks, at least one),
  the vehicle is removed at the start of the tick, after mandatory lane changes and missed exits
  (D581). A tick out of the condition restarts the count.
- **With `missedExitAfter`:** a missed exit with somewhere to go happens first and the count restarts on
  the new route. Diffusion removes only a vehicle that is still waiting.
- **Effect:** the vehicle leaves the network and emits `DiffusedEvent` (time, vehicle, route). It is
  neither completed nor active. It adds no travel time or delay to the means, as in Vissim.
- **Counting:** runs whose behaviours carry the key report `removed` in the run summary, the
  evaluation and the batch (JSON and CSV), as D581's `diverted` is reported. Other runs keep their
  bytes.

## 3. File and editor

- Schema 82 (`diffusionVersion`). The key is written only when set; an older file carrying it is
  refused with `EDIT_UNSUPPORTED_FIELD`, and a value outside the range with
  `INVALID_BEHAVIOUR_PARAMETER`.
- The behaviour editor has an optional `diffusionTime` row beside `missedExitAfter`.

## 4. Rows

| Row | Statement |
|---|---|
| DF1 | The key round-trips at 82, is refused at 81, and 0 and 3601 are refused at its path |
| DF2 | A vehicle held at its stub's dead end is removed on its `diffusionTime`-th waiting tick, with one `DiffusedEvent`; it never arrives; the rest of the queue is unchanged in count |
| DF3 | Without the key, the same road keeps the vehicle waiting to the queue's release (the forcing) |
| DF4 | A vehicle waiting at a red head for longer than `diffusionTime` is not removed |
| DF5 | With `missedExitAfter` shorter than `diffusionTime` and an onward route, the vehicle diverts and is not removed |
| DF6 | The same seed gives the same run, events included |
| DF7 | The summary, the evaluation and the batch carry `removed` only when a behaviour has the key |
| DF8 | Every shipped project and the counted study give identical reports (no shipped file sets it) |
