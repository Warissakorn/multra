#pragma once
// M6 kind U's projects (D542, docs/reference/M6_U_RUN.md §1-§2): a left-hand T whose minor left turn
// (rank 2, the kerb-side turn) merges into the near major lane under Stop. Built in memory through
// the editor's own commands, after tools/t_junction_network.hpp; not shipped. gapTime is HCM 7's
// base t_c = 6.2 s minus tau, the time a major car takes to clear the area at 50 km/h (§1.3).
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace multra::m6u {
inline constexpr double kMinorDemand = 1800, kDuration = 4500, kWarmup = 900;
inline constexpr double kTc = 6.2, kTf = 3.3, kMajorSpeed = 50 / 3.6, kCarLength = 4.5, kHeadway = 0.1;
inline const std::vector<double> kLevels{200, 400, 600, 800, 1000};
// §1.3: the earlier major car holds the area until its rear clears it.
inline double tau(double areaLength) { return (areaLength + kCarLength) / kMajorSpeed; }
inline double gapTime(double areaLength) { return kTc - tau(areaLength); }

struct Junction { ProjectDocument document; std::string area, minorRoute, nearRoute, farRoute; double areaLength{}; };

inline Junction junction(double vc) {
    constexpr double w = 3.5, median = 1, far = 150, minorEnd = 12, join = 20;
    constexpr double centre = (w + median) / 2;
    // The left-hand mirror of the T-junction fixture: the near lane runs east on the north side,
    // the minor road arrives from the north heading south.
    const auto p = [](double x, double y) { return Point{x, -y}; };
    const auto lane0 = [](const ProjectDocument& d, const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.front().id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    Junction j;
    auto& d = j.document;
    d.network.id = "m6-u-junction";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const auto nearLink = addLink(d, {p(-far, -centre), p(far, -centre)}, 1, w);
    const auto farLink = addLink(d, {p(far, centre), p(-far, centre)}, 1, w);
    const auto minor = addLink(d, {p(0, -far), p(0, -minorEnd)}, 1, w);
    editableLink(d, nearLink).name = "Major near lane";
    editableLink(d, farLink).name = "Major far lane";
    editableLink(d, minor).name = "Minor approach";
    const auto turn = addConnector(d, {minor, lane0(d, minor)}, {nearLink, lane0(d, nearLink), far + join});
    editableConnector(d, turn).name = "Minor left turn";
    const auto areas = takeOverMergesOf(d, turn, {kTc, kHeadway});
    if (areas.size() != 1) throw std::logic_error("M6U_MERGE");
    j.area = areas.front();
    const auto& a = *std::find_if(d.network.rightOfWay.conflictAreas.begin(), d.network.rightOfWay.conflictAreas.end(),
                                  [&](const auto& x) { return x.id == j.area; });
    const bool firstIsMinor = a.first.path.connectorId == turn;
    const auto& major = firstIsMinor ? a.second : a.first;
    j.areaLength = major.exitStation - major.entryStation;
    setConflictControl(d, j.area, "Minor left turn joins the near lane",
                       firstIsMinor ? ConflictPriority::firstYields : ConflictPriority::secondYields, gapTime(j.areaLength), kHeadway);
    setAreaControl(d, j.area, StopMode::stop);
    j.nearRoute = putRoute(d, {"", {nearLink}});
    j.farRoute = putRoute(d, {"", {farLink}});
    j.minorRoute = putRoute(d, {"", {minor, turn, nearLink}});
    putInput(d, {"", j.nearRoute, "car", vc, 0, kDuration, {}});
    putInput(d, {"", j.farRoute, "car", vc, 0, kDuration, {}});
    putInput(d, {"", j.minorRoute, "car", kMinorDemand, 0, kDuration, {}});
    changeRunSettings(d, kDuration, 0.1);
    changeEvaluationPeriod(d, kWarmup, kDuration, 0);
    changeRandomStreams(d, RandomStreams::perInput);
    return j;
}
}
