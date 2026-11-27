# Sources for the approach to a standing queue — search of 2026-11-26

The owner asked for sources first ("ทำเรื่องการชะลอเข้าหาแถวรถต่อ หาแหล่งอ้างอิงก่อน"). STOP_PROFILE §9 (D664) left one
question open. Under B3, a car slows from far off toward a vehicle at rest. It then stops 30 % less often, but its
delay is 9.5 s (20 %) higher. Do drivers slow that early for a standing queue that may move off? Nothing here is used
yet (D178). Each line says what was read.

## Read

- **Kiefer, R.J. et al. (2003). Forward Collision Warning Requirements Project, Task 1: refining the CAMP crash alert
  timing approach by examining "last second" braking and lane-change maneuvers.** DOT HS 809 574, NHTSA (ROSA P copy,
  read 2026-11-26).
  - **Method (pp. 4–5, §§ Method):**
    - test-track trials, with a surrogate target that is stationary, slower, or braking;
    - drivers were told to "brake at the last second possible ... using 'normal' braking intensity", or "hard";
    - 3,536 braking trials, ages 20–70; site, age and gender effects were "relatively small".
  - **Figure 1, p. 18** (read from the plotted points, about ±0.01 g). A driver approaching a stationary lead vehicle
    at 30 / 45 / 60 mph began **"normal" last-second braking** at a mean required deceleration `v²/(2·range)` of about
    **0.17 / 0.23 / 0.26 g** (1.7 / 2.3 / 2.5 m/s²). "Hard" braking began at about 0.28 / 0.34 / 0.36 g.
  - **p. 31:** the 95th percentile of "normal" equals about the 50th percentile of "hard", "across a wide range of
    kinematic conditions".
  - **What it says for MULTRA:**
    - Today's law starts braking when the required deceleration reaches b = 2 m/s² (0.20 g). That is within these
      means: the latest moment a driver still calls normal.
    - B3 starts from about 144 m at 15 m/s, a required deceleration near 0.08 g. That is much earlier than "last
      second".
    - **It does not say when drivers habitually start.** The instruction was "the last second possible", toward a
      parked target that never moves off.
- **Kiefer, R.J. et al. (2005). Forward Collision Warning Requirements Project, Task 4 final report.** DOT HS 809 925
  (ROSA P). It compares CAMP's onsets with NADS simulator data (Figure, p. 37). It notes that the greatest divergence
  between the two is in the stationary-lead case. Its plotted values were not transcribed.
- **Rittger, L. (2015). Driving behaviour and driver assistance at traffic light intersections.** Dissertation,
  Universität Würzburg (OPUS copy, read 2026-11-26).
  - **Study 1 (pp. 34–45)** was a static driving simulator (WIVW, SILAB), with 12 participants approaching at
    13 m/s (46.8 km/h). The factors were solid green, solid red, red-to-green and green-to-red, with and without a
    lead vehicle, and with and without fog.
  - **p. 41:** at a solid red, drivers "initiated the stop at red" at **90–70 m** before the intersection (a required
    deceleration of about 0.9–1.2 m/s² at 13 m/s).
  - **p. 44, Table 6:** toward a red that turns green before arrival, drivers **released the accelerator in 94 %** of
    approaches without fog, and braked in 47–50 %. Toward a solid green they released it in 39–42 %.
  - **p. 42:** a lead vehicle changed speed only in the last 50 m, because the stop point was farther back.
  - **It concerns a red signal, not a standing queue.** It is a simulator, and it is a small sample.
  - **p. 26–27** (citing Berndt et al. 2007, a roadside laser scanner, 270 profiles): when the signal turned red far
    from the intersection, "the majority of drivers reduced speed slightly over the 60 m of traffic light approach",
    and some braked strongly. Not read at its source.
- **Treiber, M., Kesting, A., Helbing, D. (2006). Delays, inaccuracies and anticipation in microscopic traffic
  models.** Physica A 360, 71–88; arXiv:cond-mat/0404736v3 (read 2026-11-26).
  - It models temporal and spatial anticipation (looking several vehicles ahead), and finds it stabilising.
  - It cites IDM's "built-in anticipative and smooth braking strategy" (p. 3).
  - It is a model, not data on queues.

## Found but not read

| Source | What the summary says | Why not read |
|---|---|---|
| Chen, Sherony, Gabler (2016). SAE 2016-01-1448, TTC and enhanced TTC at brake application, 100-Car normal driving | Over 870,000 braking events with a closing lead vehicle. It is not known whether a stationary subset is reported | SAE paywall |
| Kusano, Gabler (2011). SAE 2011-01-0576 | In real lead-vehicle-stopped rear-end **crashes**, mean TTC at braking 1.1–1.4 s | SAE paywall; crashes, not normal driving |
| NHTSA DOT HS 810 846 (2007), 100-Car rear-end analyses | About 90 % of drivers had TTC ≥ 2 s at lead braking onset | `nhtsa.gov` answers 403 to scripts |
| Rittger et al. (2015), *Cognition, Technology & Work* | Study 1 of the dissertation as a journal paper | Blocked; the dissertation was read |
| Kaul & Baumann (2011); Kaul (DLR) | Traffic-light approaches with and without a lead vehicle | Not reached |
| Laquai et al. (2013), ITSC (TUM copy downloaded) | Compares models' deceleration toward a standing vehicle in SUMO | A model comparison; not read beyond the title page |

## What the sources read say about B3's question

1. **No source read measures when drivers begin to slow for a standing queue** that may move off, as opposed to a red
   signal or a parked target.
2. **Toward a parked target, the latest "normal" onset** (CAMP) is at a required deceleration of about 1.7–2.5 m/s².
   Today's law (b = 2 m/s²) brakes at that moment. So does B3P at lines when IDM is not lower. Habitual drivers start
   earlier than "the last second", but by how much is not measured there.
3. **Toward a red signal** (simulator), drivers start the stop at about 1 m/s² of required deceleration (90–70 m at
   13 m/s). Toward a red that will turn green, almost all of them release the accelerator early. That is
   anticipation, but of a signal, not of a queue.
4. **So B3's early onset (about 0.8 m/s² at 15 m/s) is not contradicted, but it is not supported for queues either.**
   Its 9.5 s cost cannot be judged from these sources.

## What could settle it

- **A trajectory dataset with queues at signals** (NGSIM Lankershim or Peachtree; inD). Measure, per vehicle joining a
  standing queue, the required deceleration `v²/(2·gap)` at braking onset, and whether the queue moved before it
  arrived. That would be a measurement from field data, not a calibration of B3 (D544).
- **The 100-Car or SHRP2 normal-driving TTC at brake onset** for a stopped lead, if the owner can obtain SAE
  2016-01-1448.
