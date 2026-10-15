#pragma once
#include <cstdint>
#include <optional>

namespace trafficsim {
// M3.7 lateral contract (docs/reference/LATERAL.md, D180/D187). Slice 1 stores and compiles these;
// nothing in the tick reads them yet.
// The driver's side of the road. Core owns it so the compiled Scenario can carry it; the model's
// Network uses the same enum (hard rule 3).
enum class DrivingSide { left, right };
// §9: optional behaviour keys on either model. All absent switches the lateral layer off; any one
// present requires all three.
struct LateralParameters {
    std::optional<double> lateralMinDistanceStanding, lateralMinDistanceAt50, maxLateralSpeed;
    bool operator==(const LateralParameters&) const = default;
};
// §3: parallel to Scenario::segments. `carriageway` groups the lane sections of one Link (stations
// `linkStationStart + position`); each Connector path is its own carriageway. `centre` is the lane
// centre's offset (y, positive to the left of travel) from the carriageway's reference line, 0 on a
// Connector path. Width runs linearly from `startWidth` to `endWidth`; they differ only on a
// Connector path, where a surplus lane tapers to 0 (D187).
struct SegmentCrossSection {
    std::size_t carriageway{};
    double linkStationStart{}, centre{}, startWidth{}, endWidth{};
    bool operator==(const SegmentCrossSection&) const = default;
};
}
