# A set-back at holding lines: sources and an NGSIM look — 2026-11-26

The owner asked whether `lineRest` should stop a suitable distance short of the line, and chose "ตามแนะนำ": sources
first, then a contract for a key whose default is 0 (LINE_STOP §3's L2). Nothing here sets a value (D178, D544).
Today, under `lineRest`, every model rests with its front at the line (LINE_STOP §2, D652).

## Read

- **PTV Vissim manual, *Editing the driving behavior parameter Following behavior*** (online help, 2021 and 2024
  pages, read 2026-11-26):
  - "Vehicles stop in front of signal heads at standstill distance (ax)+ 0.5 m." (same on both pages)
  - *Standstill distance for static obstacles*: "Standstill distance (ax) for vehicles upstream of static
    obstacles." Its range is 0.01–10 m, default 0.5 m.
  - The obstacles are "for example, signal heads, stop signs, bus stops, priority rules, conflict areas".
  - The text is not tied to Wiedemann 74 or 99.
  - The 2024 page *Modeling signal groups and signal heads* says vehicles "stop at a standstill distance in front of
    the signal controller" and "The default standstill distance is 0.5 m."
  - **Not settled by the pages read:** whether the stop is at the static-obstacle value alone or at the
    following ax plus 0.5 m. The two sentences differ. A search summary also mentioned a normal distribution
    when the option is off; no page read states that.
- **Eclipse SUMO, *Definition of Vehicles, Vehicle Types, and Routes*** (documentation source on GitHub, read
  2026-11-26), junction-model parameters:
  - `jmStoplineGap`, default **1 m**: "This value configures stopping distance in front of a priority /
    TL-controlled stop line."
  - `jmStoplineGapMinor`, default **0**: the same "in front of a non-priority stop line".
  - "In case the stop line has been relocated by a stopOffset item, the maximum of both distances is applied."
  - A lane's or edge's `stopOffset` element moves the stop line back for chosen vehicle classes (bike boxes). No
    default was found.

So both programs have the key. Their defaults, 0.5 m and 1 m, are each program's own choice; neither page cites a
field measurement.

## Searched, not found

- No field study read measures where the first car of a queue rests relative to a signal's stop line.
- A stop-sign study (three T-junctions with limited sight lines) found drivers stopping a mean of 9.6 ft past the
  limit line. It concerns sight lines at stop signs, not signals, and was seen only in a search summary.

## An NGSIM look (not a measurement of the stop line)

Script: [line-setback-ngsim.py](line-setback-ngsim.py), run with `python3 -I` on the Lankershim CSV of
[queue-approach-ngsim](queue-approach-ngsim.md).

- **NGSIM gives no stop-bar position in the trajectory file.** The script uses the point where tracks first enter
  an intersection (`int_id` 0 → k) as a stand-in. For each direction, lane and intersection that point is
  consistent: the 10–90 % spread is 2.3–3.4 m, mostly sampling at 0.1 s and video noise. Whether that boundary is the
  stop bar is **not known**. The dataset's CAD files would settle it; they were not downloaded.
- **First cars at rest** (cars, no leader at or upstream of the boundary, within 15 m of it; n = 465):
  - the front rests at a median of **0.96 m short** of the boundary;
  - percentiles 10 / 25 / 50 / 75 / 90: −3.13 / −0.86 / 0.96 / 2.87 / 6.35 m (negative is past it);
  - 36 % rest past it;
  - northbound has a median of 0.33 m (n = 241), southbound 1.47 m (n = 224).
- **Reading:** if the boundary is the stop bar, real first cars rest about 1 m short with a spread of several
  metres either side. That is the order of both programs' defaults, but it rests on an unverified boundary and on
  one arterial. It is not a citation for a default.

## What this means for the contract

1. **A key with default 0** keeps every result as today. It needs no citation: the owner or a study sets the
   value (L2 of LINE_STOP §3, with L3's law aiming at `line − distance`).
2. **A non-zero default** would need a field source. Vissim's 0.5 m and SUMO's 1 m are other programs'
   defaults, not measurements. The NGSIM look agrees in order but does not settle it (the stop bar is
   unverified).
3. **Where the key lives** (driver behaviour, as in Vissim and SUMO, or project) and **which lines it applies to**
   (signal, give-way, Stop, zone, gate; SUMO uses 0 for non-priority lines) are the owner's choices for the
   contract.
