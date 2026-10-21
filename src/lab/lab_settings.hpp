#pragma once
// D165 (docs/reference/BEHAVIOUR_LAB.md §2): everything the behaviour lab lets a developer change.
// Plain values, no Qt: the window edits a copy and hands it to LabRun::apply.
#include "../core/types.hpp"
#include "../model/network/network.hpp"
#include <filesystem>

namespace multra::lab {
// The vehicle being tuned: its own type and behaviour, so nothing else on the ring shares them.
struct FocusSettings {
    VehicleType type;
    DriverBehaviour behaviour;
    bool operator==(const FocusSettings&) const = default;
};
// Everyone else. Cars and heavy vehicles share one behaviour; `heavyShare` of them are heavy.
struct TrafficSettings {
    int vehicleCount{12};     // on the tuned vehicle's loop
    int crossVehicleCount{6}; // on each cross-traffic loop (Urban circuit)
    double heavyShare{0.1};
    VehicleType car, heavy;
    DriverBehaviour behaviour;
    bool operator==(const TrafficSettings&) const = default;
};
// Oval: a stadium ring, two straights joined by two half circles, optionally one fixed-time signal
// half way along the lower straight. Urban circuit (D166): a block of `straightLength` x
// `blockDepth` with two sweeping corners, two straight-through and two left-turn signalised
// intersections, and three cross-traffic loops (BEHAVIOUR_LAB.md §2).
enum class Layout { oval, urban };
struct RoadSettings {
    Layout layout{Layout::urban};
    double straightLength{400}, curveRadius{60}, laneWidth{3.5}, blockDepth{240};
    int lanes{1};
    bool signal{};
    // Main approach green, amber and red; the cross approach is green for red - amber - 2 allRed.
    double green{30}, red{30}, amber{3}, allRed{2};
    DrivingSide drivingSide{DrivingSide::left};
    bool operator==(const RoadSettings&) const = default;
};
struct LabSettings {
    FocusSettings focus;
    TrafficSettings traffic;
    RoadSettings road;
    bool rescaleDesiredSpeeds{true};
    bool operator==(const LabSettings&) const = default;
};
// Ids the lab gives its own types and behaviours; the project's ids never reach the lab run.
inline constexpr const char* kFocusType = "lab-focus";
inline constexpr const char* kFocusBehaviour = "lab-focus-behaviour";
inline constexpr const char* kCarType = "lab-car";
inline constexpr const char* kHeavyType = "lab-heavy";
inline constexpr const char* kTrafficBehaviour = "lab-traffic-behaviour";
// Starting values: the catalog's car, heavy vehicle and their behaviours (data/vehicle-types,
// data/driver-behaviour), or the given project types when the editor passes its own catalog.
LabSettings defaultSettings(const std::vector<VehicleType>& types, const std::vector<DriverBehaviour>& behaviours);
LabSettings defaultSettings(const std::filesystem::path& dataDirectory);
// Preserve valid explicit axle proportions when the lab changes body length.
// Missing geometry retains defaults; malformed geometry is left for core validation.
void rescaleAxles(VehicleType& type);
// B-5 (SPEED_DISTRIBUTION §3): the Lab edits min/max only, so a type's curve follows them.
void fitSpeedCurves(LabSettings& settings);
}
