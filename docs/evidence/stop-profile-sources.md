# Sources for B, a softer stop profile — search of 2026-11-25

The owner asked for B's sources first ("build package เริ่ม B หาแหล่งอ้างอิง"). B is the profile of LINE_STOP's
`lineStopAcceleration`. Today it accelerates up to the comfortable-braking curve `v = √(2·b·gap)`, brakes on it at
the type's `comfortableDeceleration`, then rests at once (LINE_STOP §2, D652). Nothing here is used yet: rule
D178 needs a value read from its source, and most sources below were **not** read. Each line says which.

## Read

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
| Akçelik, R., Biggs, D.C. (1987). Acceleration profile models for vehicles in road traffic. *Transportation Science* 21(1), 36–54 | Polynomial and sinusoidal models, with zero acceleration (and, except one, zero jerk) at the start and end; peak position adjustable for a given mean; "similar results were found for deceleration profiles" | Paywalled; the open SIDRA copy (`www.sidrasolutions.com`) was refused |
| Akçelik, R., Besley, M. (2001). Acceleration and deceleration models. 23rd CAITR | Calibrated deceleration models | As above |
| Treiber, Kesting et al., IDM papers (arXiv 0803.4063, 2506.05909) | Behind a standing obstacle the deceleration is of the order of b and eases off just before the stop | `arxiv.org` does not resolve for the fetch tool, even after it was allowed |
| Loughborough repository, "Analyzing and modelling drivers' deceleration behaviour from normal driving" | The most common profile is hard braking first, then smoother | `repository.lboro.ac.uk` does not resolve, as above |
| Wang, Dixon, Li, Ogle (2005), TRR 1937, stop-sign deceleration with in-vehicle GPS; Haas et al.; Chang | Mean decelerations of about 3.5 m/s² (Haas, 0.36 g) and 2.9 m/s² (Chang, 0.295 g); braking force not constant (Weaver et al., SAE 2011) | No open copy found |
| Fambro et al. (1997), NCHRP Report 400, stopping sight distance | Measured stopping decelerations | `onlinepubs.trb.org` returned HTTP 503 twice |

## What the summaries agree on (to verify before use)

- A measured stop is not constant braking: deceleration builds up and eases off toward rest (Akçelik & Biggs; IDM;
  Weaver et al.). Today's law stops at its full comfortable rate.
- The models that remove the end jerk do so with an acceleration that is zero at both ends (Akçelik & Biggs's
  polynomial) or that falls smoothly to zero near the target (IDM).

## Next

- Read Akçelik & Biggs (1987) or Akçelik & Besley (2001) and an IDM primary text, with page numbers. Their hosts
  need the fetch tool to reach them (above).
- SUMO 1.18 is installed here, so its stop at a red line can be measured directly as a comparison. A model is not
  field data, though.
