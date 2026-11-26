# NEXT state cut on 2026-11-26

Text cut verbatim from [NEXT](../NEXT.md) when it was compacted to open items again (D319). Nothing here is
current; NEXT keeps each section's open lines. Links are rewritten for this folder.

## Next session — the M3.3.3b W99 and line-stop bullet (D637–D667)

- **M3.3.3b W99 — contract (D637) through composition (D642), CC1 as a curve (D645–D648), BA231's timestep
  record (D649), the creep's hold (D650) and the stop at CC0 (D654); W99 runs, unvalidated:** [W99](../reference/W99.md). Groups `w99`,
  `w99codec`, `w99traits`, `w99run`, `w99cc1codec`, `w99cc1run` and `behaviour-library-ui` pass. Heavy vehicles are
  bounded by their type's acceleration, not halved (D644). The unvalidated marker stays.
  **Stopping at the line (D651–D653):** [LINE_STOP](../reference/LINE_STOP.md) L3F is in, as `clampRules.lineRest`
  (schema 87, on by default for projects). Every W74/W99 behaviour, and every prototype carrying
  `amberDeceleration`, stops at signal, give-way, Stop, zone and gate lines; dead ends stay leaders; bare M0 keeps
  its bytes. D653: a front exactly at a line has not passed it (`pastLine`, three observers). With it, D652's
  "startup lost time toward HCM" is withdrawn: the switch lowers it by 0.3–1.0 s. BA27/BA231 and the M0 scene
  against SUMO are re-run ([W99](../evidence/w99-discharge.md#after-d652-and-d653),
  [M0 under project rules](../evidence/m0-plausibility.md#under-project-rules-d652-d653)).
  **Next:**
  - **W99 behind a moving leader.** D654 stops W99 exactly at CC0 behind a standing obstacle, which takes
    BA231's clamps from 345 to 42 per run, the four-leg from 337 to 39 and M2.6 from 1,259 to 150. Most of the
    rest are behind leaders still creeping when they stop. Capping those needs the leader's braking, which
    `Leader` does not carry. It is a contract question before code.
  - **Startup lost time below HCM's 2.0 s** in every model and in SUMO is the absence of a reaction at green, not
    a line question. PERCEPTION's `reactionTime` is the existing mechanism; any value needs a citation (D178).
    Not proposed until the owner asks.
  - **The M6 U record and sweep (D543, D546)** run with the switch off; re-measure them under D652/D653 if M6
    work resumes.
  - **A cited W99 preset** only if one exists (D178).
  **The owner's F9 (2026-11-25):** the first car stops at the line but unrealistically (constant −2 m/s², then
  a jump to 0); followers crawl without end (prototype 40–60 s, W74 25–30 s, predating D652). The owner chose
  **A then B**: one stop law for lines and standing vehicles, then a softer, cited stop profile. A first attempt
  of A failed nine groups and was not committed ([LINE_STOP §9](../reference/LINE_STOP.md#9-the-owners-look-on-windows-and-a-queue-behind-a-standing-car-open),
  [patch](../evidence/line-stop-queue-attempt.patch)). **A is done for W74 and W99** ([LINE_QUEUE](../reference/LINE_QUEUE.md), D655 contract, D656 Q2): queues come to rest
  without a crawl, and W99's clamps are 8 per run on BA231.
  **The prototype's crawl** ([PROTOTYPE_QUEUE](../reference/PROTOTYPE_QUEUE.md), D657–D658): the owner chose P17.
  The attempt was not committed: `lab` d217 clamps with `reactionTime` 0.5 s, three scene forcings were lost, and
  BA197 served 4 % fewer ([patch](../evidence/prototype-queue-attempt.patch)). D657's "P17" figures were P17L's
  (corrected).
  **The reaction delay is done** ([REACTION_STOP](../reference/REACTION_STOP.md), D659–D661, the owner's R9). Under
  `lineRest`, delayed vehicles rest without clamps or red passes: at 0.5 s W74 clamps 9 / 44 per run (four-leg /
  M2.6), against 755 / 3,277 before. Runs without a delay are unchanged.
  **Prototype queues rest** ([PROTOTYPE_QUEUE §7](../reference/PROTOTYPE_QUEUE.md), D662): P17 on D661. Without a
  delay, crawl 17 → 1.3 s, 0 clamps; jerk and hard braking up, which are B's profile. At `reactionTime` 1 s the
  prototype's delay is +28 s against the switch off (the two start rules add up). The archived M3.2.7c congested
  variant now runs with `lineRest` off; a headway-arm fixture under `lineRest` would be new work.
  **Next:**
  - **B is closed** ([STOP_PROFILE §10–§11](../reference/STOP_PROFILE.md), D663–D667). The owner's B3P stands, and
    today's law stays toward vehicles at rest (D667).
    - The prototype eases into a line's stop (IDM, T = 0, the type's a and b): four-leg hard braking 1,027 → 903,
      with the same delay and clamps.
    - **Left:**
      - the approach to a standing queue: measured on NGSIM Lankershim (D666). The median onset is 1.4–1.9 m/s² of
        required deceleration, nearer today's law (2.0) than B3 (0.8). The owner kept today's law (D667). The staged
        approach and the higher final braking are known deviations; a second site would reopen them;
      - the last step to rest (Akçelik & Biggs 1987, Deligianni 2017, both unread);
      - W74's and W99's profile, which D258's floor sets.
    - Akçelik & Biggs (1987) and Deligianni (2017) are still unread (owner's copy).
  - **A leader braking to a stop** is outside Q2.
  **Owner (Windows):** [F11](../plans/OWNER_SITTING_WINDOWS.md) on package #152 (`a18a229`, D652–D665): the
  prototype's first car eases into its stop at a red line. #152 also carries F10.
  **Owner (Windows):** [F10](../plans/OWNER_SITTING_WINDOWS.md) on package #151 (`699b109`, D652–D662): queues at
  rest for W99, with and without `reactionTime` 0.5 s; it re-checks F9 b and c. #150 (`a5abe16`) lacks P17.
  **Owner (Windows), package #149 (`f5653f8`, D652–D654):**
  - [F9](../plans/OWNER_SITTING_WINDOWS.md): the `lineRest` checkbox, queues at the line, a W99 queue, and the save
    and reopen.
  - [F8](../plans/OWNER_SITTING_WINDOWS.md): the CC1 table.
  - [F7](../plans/OWNER_SITTING_WINDOWS.md): the out-of-range recheck (D643).

## Closed sections (2026-11-22 to 2026-11-23), merged into one "Left from closed work" section

## Parallel sessions of 2026-11-23 — merged

Briefs: [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md). All four merged (A D626–D627, B D630, C D634, D no ID).
**Left:** the owner's look at the Thai Results strip at 1024 on Windows (C). The bucket rebuild after each
source insertion is done (D635, benchmark −0.81 %).

## M3.2 — closed by the owner (D625)

Closed 2026-11-23. Still open on their own gates: [OWNER_LOOKS_CARVED](../plans/OWNER_LOOKS_CARVED.md) M3.2.10 and M3.2.13
(now with item 4, M3.2.4e/M3.2.12 on Windows), M3.2.8a.1, M3.4, M6. Branching stays read-only (D624); a visibility
key only for a study with a flared approach and a cited default.

## Priority rules like Vissim (D604, D606, D614–D617) — done; the owner's look passed

[PRIORITY_RULES](../reference/PRIORITY_RULES.md): all slices done, item 51 passed 2026-11-22 ("51 ok"). **Left:** a class
picker in place of typed class ids if the owner asks.

## G7 waiting acceptance (D618, D619) — done; item 52 passed (2026-11-22)

[YIELD_IMPATIENCE](../reference/YIELD_IMPATIENCE.md); its display findings are D620. **The other realism gaps**
([evidence](../evidence/realism-gaps-vissim-sumo-aimsun.md)): G8 needs nothing (matches all three programs); G4, G5,
G6 are named in ROADMAP M3.4; G9 is M3.3.3b.

