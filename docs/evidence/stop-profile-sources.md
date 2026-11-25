# Sources for B, a softer stop profile — search of 2026-11-25

The owner asked for B's sources first ("build package เริ่ม B หาแหล่งอ้างอิง"). B is the profile of LINE_STOP's
`lineStopAcceleration`. Today it accelerates up to the comfortable-braking curve `v = √(2·b·gap)`, brakes on it at
the type's `comfortableDeceleration`, then rests at once (LINE_STOP §2, D652). Nothing here is used yet: rule
D178 needs a value read from its source, and most sources below were **not** read. Each line says which.

## Read

- **Kesting, A., Treiber, M. (2008). Calibrating car-following models using trajectory data: methodological study.**
  arXiv:0803.4063v1 (read 2026-11-25, PDF downloaded with the owner's permission).
  - **p. 3, Eq. (2)–(3):** `v̇ = a[1 − (v/v0)^4 − (s*/s)²]`, with `s* = s0 + vT + vΔv/(2√(ab))`. The last term
    "implements an 'intelligent' driving behavior including a braking strategy that, in nearly all situations,
    limits braking decelerations to the comfortable deceleration b. Note, however, that the IDM brakes stronger than
    b if the gap becomes too small."
  - **p. 7, Table 1:** b calibrated per dataset and objective is 0.61–3.73 m/s², s0 1.04–3.40 m, a 0.96–1.58 m/s²,
    T 1.03–1.51 s. It is one radar-equipped car in city traffic, so a range, not a default.
  - **p. 15:** the driver's standstill gaps at three red lights were 1.39, 1.42 and 1.64 m ("a deterministic
    car-following model allows only for an averaged and, thus, 'effective' description").
- **Zhou, S., Zheng, S., Tian, J., Jiang, R., Zhang, H.M. (2025). Twenty-five years of the Intelligent Driver Model:
  foundations, extensions, applications, and future directions.** arXiv:2506.05909v2 (read 2026-11-25).
  - **p. 5, §2.1.2, the modelling assumptions after Treiber & Kesting (2013):** "Under normal conditions, braking is
    gradual, smoothly reducing deceleration to zero as the vehicle approaches a steady state or stops. In critical
    situations, the deceleration exceeds comfortable levels until safety is restored". Also: "Transitions between
    driving modes ... are smooth. The model ensures that the jerk ... remains finite".
  - This is a review restating Treiber & Kesting's book, which is not read here.

- **SUMO documentation** (sumo.dlr.de, read 2026-11-25):
  - *Definition of Vehicles, Vehicle Types, and Routes*, vType attribute table:
    - `decel` defaults to 4.5 m/s²: "The deceleration ability of vehicles of this type";
    - `emergencyDecel` defaults to 9.0;
    - `apparentDecel` defaults to `decel`;
    - the default `carFollowModel` is Krauss.
  - *Car-Following Models*, section "decel, apparentDecel, emergencyDecel": "For the default Krauss model this is a
    hard limit whereas for other models such as IDM the decel bound is less strict."
  - The page says nothing on a braking profile or on smoothing near the stop. It calls itself a stub, and points to
    *Developer/Implementation_Notes/Vehicle_Models* (not read).
- **FHWA-HRT-17-016** (2017), SHRP2 naturalistic driving at rural high-speed intersections (fhwa.dot.gov, §§
  "Brake distance", "Probability of making a complete stop"):
  - brake application averaged 329.4 ft (SD 115.8) from the intersection centre, at 61.7 mi/h (SD 11.5);
  - a complete stop occurred in 49.7 % of cases.
  - **It reports no deceleration rates or profiles.** Not usable for B's shape.

## Found but not read (abstracts or search summaries only)

| Source | What the summary says | Why not read |
|---|---|---|
| Akçelik, R., Biggs, D.C. (1987). Acceleration profile models for vehicles in road traffic. *Transportation Science* 21(1), 36–54 | Polynomial and sinusoidal models, with zero acceleration (and, except one, zero jerk) at the start and end; peak position adjustable for a given mean; "similar results were found for deceleration profiles" | Paywalled (INFORMS); SIDRA's old link answers 404 |
| Akçelik, R., Besley, M. (2001). Acceleration and deceleration models. 23rd CAITR | Calibrated deceleration models | As above |
| Deligianni, S.P., Quddus, M., Morris, A., Anvuur, A., Reed, S. (2017). Analyzing and modelling drivers' deceleration behaviour from normal driving. *TRR* 2663, 131–141 | 16 drivers on a 16.5 km route (TeleFOT), about eleven million observations. "The most used profile ... follows a hard braking at the beginning when detecting a danger and then becomes smoother" | The Loughborough copy (and the author's thesis there) answer HTTP 202 with an empty body, a page that needs a browser |
| Wang, Dixon, Li, Ogle (2005), TRR 1937, stop-sign deceleration with in-vehicle GPS; Haas et al.; Chang | Mean decelerations of about 3.5 m/s² (Haas, 0.36 g) and 2.9 m/s² (Chang, 0.295 g); braking force not constant (Weaver et al., SAE 2011) | No open copy found |
| Fambro et al. (1997), NCHRP Report 400, stopping sight distance | Measured stopping decelerations | `onlinepubs.trb.org` returned HTTP 503 twice |

## IDM's profile, computed (not a source)

Kesting & Treiber's Eq. (2)–(3), with a = 1.4, T = 1.2, s0 = 2 (within Table 1) and b = 2 (MULTRA's car), v0 = 16,
dt 0.01. A car from 15 m/s, 200 m behind a car at rest:

| t (s) | v (m/s) | gap beyond s0 (m) | acceleration (m/s²) |
|---|---|---|---|
| 4 | 14.8 | 138.1 | −0.15 |
| 8 | 13.7 | 80.7 | −0.47 |
| 12 | 10.2 | 31.7 | −1.37 |
| 14 | 7.0 | 14.4 | −1.84 |
| 16 | 3.3 | 4.2 | −1.68 |
| 18 | 0.76 | 0.47 | −0.79 |
| 20 | 0.00 | −0.06 | −0.09 |

- It brakes from about 165 m. The deceleration rises to a peak of 1.87 m/s² (below b) about 10 m out, then falls to
  about 0.1 m/s² at rest.
- It reaches rest in about 20 s, but 6 cm inside `s0`. A law built from it still needs MULTRA's exact finish at the
  target (D596).
- **Today's law,** from the same start, holds 15 m/s to 56 m out, brakes at a constant 2 m/s² for 7.5 s, and rests at
  once.

## What the summaries agree on (to verify before use)

- A measured stop is not constant braking: deceleration builds up and eases off toward rest (Akçelik & Biggs; IDM;
  Weaver et al.). Today's law stops at its full comfortable rate.
- The models that remove the end jerk do so with an acceleration that is zero at both ends (Akçelik & Biggs's
  polynomial) or that falls smoothly to zero near the target (IDM).

## Next

- **Read:** IDM's equations and its braking principle (Kesting & Treiber 2008; Zhou et al. 2025). These are enough to
  contract an IDM-shaped stop with the type's `comfortableDeceleration` as b.
- **Still unread:** Akçelik & Biggs (1987), and Deligianni et al. (2017), whose field profile (hard first, then
  smoother) differs from IDM's (gradual, then easing). Either needs the owner's copy or a reachable host.
- SUMO 1.18 is installed here, so its stop at a red line can be measured directly as a comparison. A model is not
  field data, though.
