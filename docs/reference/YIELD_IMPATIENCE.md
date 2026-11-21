# YIELD_IMPATIENCE — a waiting driver accepts less: Aimsun's gap reduction and SUMO's impatience (G7, D618)

The owner's choices, 2026-11-21, on [realism-gaps-vissim-sumo-aimsun](../evidence/realism-gaps-vissim-sumo-aimsun.md) G7.

- **Owner words (verbatim):**
  - "G7 ทำแบบ Aimsun"
  - on the follow-on: "ในสถานะการที่รถทางรองออกแล้วรถทางหลักหยุด แล้วรถทางรองคันถัดไปออกต่อกันกับคนแรก
    เนื่องจากมีโอกาสจากรถทางหลักหยุด"
  - "ทบทวนความเต็มใจให้รถทางหลักชะรอของ sumo"
  - "ทำทั้งสองส่วนตามแนะนำ"
- **Choices:**
  - values as Aimsun: yield time on the driver, final gap and timing at each place;
  - yield time drawn per vehicle;
  - the follow-on by SUMO's impatience, at conflict areas only.
- **Off by default.** Not validated (rule 4).

## 1. What the programs do (read 2026-11-21 from their manuals and source)

| | Vissim 2025 | SUMO (docs and `MSLink.cpp`) | Aimsun Next (latest manual) |
|---|---|---|---|
| A long wait lowers what is accepted | No: the conflict-area attributes have no such key | `impatience`: grows by waiting time / `--time-to-impatience` (180 s), clamped to [0, 1] | *Maximum Yield Time* per vehicle type, drawn per vehicle: after it at a standstill the driver "will reduce the acceptance margins" |
| How | — | The driver goes when the foe can still brake to a stop at `impatience` × its maximum deceleration (`computeFoeArrivalTimeBraking`). A standing foe therefore never blocks | The required gap falls linearly from *Initial* to *Final Safety Margin*, between the *Initial* and *Final GW Time Factor* × the vehicle's yield time |
| Waiting time | — | Consecutive time at or below 0.1 m/s, reset when it moves | Time at a standstill in a yield situation |

## 2. Driver behaviour keys (schema 84)

| Key | Unit and range | Meaning |
|---|---|---|
| `yieldTime` | s, (0, 3600] | Mean of the vehicle's yield time (Aimsun's Maximum Yield Time). Absent: no gap reduction for this driver |
| `yieldTimeDeviation` | s, [0, 3600], default 0 | Its standard deviation. Only with `yieldTime` |
| `yieldTimeMinimum`, `yieldTimeMaximum` | s; [0, mean] and [mean, 3600]; defaults 0 and 3600 | The truncation |
| `impatience` | [−1, 1] | SUMO's base impatience. Absent: off |
| `timeToImpatience` | s, (0, 3600], default 180 | SUMO's time from 0 to full impatience. Only with `impatience` |

- **The draw.** A vehicle's yield time is a normal draw, resampled into the bounds up to eight times and then
  clamped. It is hashed from (seed, the vehicle's trait identity), as START_UP's attention windows are. No
  random stream is drawn, so a run without the key is unchanged.
- **Refusals.** A value out of range, or a dependent key without its main key, is
  `INVALID_BEHAVIOUR_PARAMETER` at its path. Older files are refused with `EDIT_UNSUPPORTED_FIELD`.

## 3. Place keys: a conflict area's priority rule and each priority marker (schema 84)

- **Keys.** `finalGapTime` (s, [0, gapTime]), `gapReductionStart` and `gapReductionEnd`. The last two are
  factors of the driver's yield time, 0 ≤ start < end ≤ 100. They correspond to Aimsun's *Final Safety Margin*
  and *Initial*/*Final GW Time Factor*; the place's own `gapTime` is the *Initial Safety Margin*.
- **All three or none.** A lone key is refused, `INVALID_PRIORITY_RULE` at a missing key's path. A value out of
  range is refused at its own path.
- **Required gap** after waiting `w` seconds, for a driver whose yield time is `Y`:
  - `gapTime` while `w ≤ start·Y`;
  - `finalGapTime` from `w ≥ end·Y`;
  - linear between.
  - Either the place's keys or the driver's `yieldTime` absent: `gapTime`, as before.
- **Headway.** It is unchanged: a major vehicle within the headway still blocks.

## 4. The engine

1. **Waiting time.**
   - When some behaviour has `yieldTime` or `impatience` (`index.yields`), each vehicle counts consecutive ticks
     below 0.1 m/s at the start of a tick (`standstillTicks`). Moving resets it, as SUMO's waiting time does.
   - The waiting time is `standstillTicks × timeStep`.
   - A vehicle queued behind others therefore restarts when it moves up to the line.
2. **Conflict areas.**
   - Under `index.yields`, the zone summary also lists each major body short of entry, with its distance,
     speed and type's maximum deceleration.
   - A minor driver is blocked when:
     - a major body is inside; or
     - some listed body is within the headway, or arrives within the driver's required gap; and
     - that body cannot stop at `impatience × maxDeceleration` (SUMO's test, `0.5·v²/m ≤ distance`).
   - With `gapTime` and impatience 0 this is the zone's `majorBlocks` exactly.
3. **Priority markers.**
   - The rule loop uses the driver's required gap in place of `gapTime`.
   - Impatience is **not** applied at markers. A major vehicle on a crossing marker does not see the minor
     vehicle, so forcing it would let the two overlap.
   - At conflict areas the major vehicle waits for the minor holder, so the forcing is safe.
4. **The follow-on** the owner described:
   - The first minor vehicle goes, and the major vehicle stops for it at the area.
   - The next minor vehicle, with any positive impatience, then finds that standing major vehicle able to stop.
     It goes behind the first.
   - A fast major vehicle close by still blocks.

## 5. Rows

| Row | Statement |
|---|---|
| YI1 | A rule's keys round-trip at 84, are refused at 83, a lone key is refused, and the area's reduction reaches its compiled zone |
| YI2 | Out-of-range place and behaviour values are refused at their paths; a valid set passes |
| YI3 | The required gap is `gapTime` before `start·Y`, linear to `finalGapTime` at `end·Y`; `gapTime` without either key |
| YI4 | Yield times vary by vehicle, stay within their bounds and repeat for the same vehicle and seed |
| YI5 | Impatience grows with waiting to 1 (180 s by default), is 0 without the key, and lets through only a major vehicle that can stop |
| YI6 | At the T-junction's crossing, a major vehicle 3 s away holds a fresh driver and passes a driver whose gap has fallen to 1.5 s; within the headway it still holds |
| YI7 | A major vehicle standing 2 m short of the area holds the minor driver without impatience and not with 0.1; a 14 m/s one 10 m away still holds |
| YI8 | With `yieldTime` set but no place reducing, every vehicle's motion is identical tick for tick |
| YI9 | A marker's reduction round-trips at 84, reaches exactly its runtime rule, and above its gapTime is refused |

## 6. Left

- The rule dialog and the conflict-area dialog have no fields for the place keys yet. They are file-only until
  slice 2. The behaviour editor has the six behaviour keys.
- No measured effect on a study. The owner's look comes with slice 2's fields.
