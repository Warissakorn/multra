#pragma once
// The lab lateral keys (D457): 0.5 m standing, 1.0 m at 50 km/h, 1 m/s -- the audit's values
// (lateral-run-audit), shared by the weaving sweep and the lateral lab so the two cannot drift.
// Lab placeholders, uncited and not a preset (D178's rule is waived for the lab only).
#include "../src/core/lateral_types.hpp"

namespace multra::fixture {
inline LateralParameters labLateralKeys() {
    LateralParameters p;
    p.lateralMinDistanceStanding = 0.5; p.lateralMinDistanceAt50 = 1.0; p.maxLateralSpeed = 1;
    return p;
}
}
