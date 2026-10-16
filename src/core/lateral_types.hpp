#pragma once
#include <cstdint>
#include <optional>
#include <vector>

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
// §3: parallel to Scenario::segments. `carriageway` groups the lane sections of one Link; each
// Connector path is its own carriageway. `linkStationStart` is metres along the lane's own polyline.
// `referenceStart`/`referenceEnd` are the section's ends on the Link's reference line, the one
// station lanes share on a curve; a position maps linearly between them (D188). `centre` is the lane
// centre's offset (y, positive to the left of travel) from the reference line, 0 on a Connector
// path. Width runs linearly from `startWidth` to `endWidth`; they differ only on a Connector path,
// where a surplus lane tapers to 0 (D187).
struct SegmentCrossSection {
    std::size_t carriageway{};
    double linkStationStart{}, centre{}, startWidth{}, endWidth{};
    double referenceStart{}, referenceEnd{};
    bool operator==(const SegmentCrossSection&) const = default;
};
// Slice 3b (D202): one side of a conflict zone as it lies on one segment of its chain, in that
// segment's metres: the area is [lo, hi] (from `entry` on the chain's first segment, to `exit` on its
// last, the whole segment between). `first` marks the chain's first segment, where approach counts.
struct SegmentZoneSide {
    std::size_t zone{}; bool major{}, first{};
    double lo{}, hi{};
};
// §5-§6 (D188): resolved once per run when the switch is on. `neighbours[s]` lists, in ascending
// segment index, the other lane sections of segment s's carriageway whose reference ranges overlap.
// `zoneSides[s]` (D202) lists the zone sides whose chain holds segment s, in zone order, major
// first; empty without a zone.
struct LateralIndex {
    bool on{};
    std::vector<std::vector<std::size_t>> neighbours;
    std::vector<std::vector<SegmentZoneSide>> zoneSides;
};
}
