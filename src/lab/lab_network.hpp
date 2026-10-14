#pragma once
// D165 (docs/reference/BEHAVIOUR_LAB.md §2): the lab's test network, a stadium ring built through
// the editor's own commands, so the canvas draws it and buildScenario compiles it exactly as it
// would any project's drawing. Never saved: the lab rebuilds it from LabSettings.
#include "lab_settings.hpp"
#include "../project/document.hpp"

namespace trafficsim::lab {
// The four Links (lower straight, right curve, upper straight, left curve) joined by four
// Connectors, travelled anticlockwise; one Route round all eight; the lab's own types and
// behaviours as an owned catalog; a fixed-time signal head on every lane of the lower straight
// when `road.signal`. No input: an input on a closed ring has an upstream segment, which a Run
// refuses (UNSUPPORTED_INTERNAL_INPUT), so LabRun compiles the drawing itself and places vehicles.
ProjectDocument buildLabDocument(const LabSettings& settings);
// One lap of the ring's reference line, metres (the straights, curves and joins).
double referenceLapLength(const RoadSettings& road);
}
