#pragma once
#include <optional>
#include <string>
#include <vector>

// M13 A1 (NODE_ANALYSIS §4, D370, D371), schema 58: what an analytical junction analysis needs and
// the Micro detail does not give. Lane groups, lanes and widths are derived, never stored. Absent
// fields are HCM 7 Exhibit 19-11's defaults (PDF p. 1430-1431) and are not written.
namespace multra {
// One turn's demand, named by its in-leg and out-leg Link ids.
struct NodeAnalysisTurn {
    std::string in, out; double vehiclesPerHour{};
    std::optional<double> heavyVehiclePercent{}; // absent: 3 %
    bool operator==(const NodeAnalysisTurn&) const = default;
};
// One in-leg's adjustments; an in-leg without a row has none.
struct NodeAnalysisApproach {
    std::string link; double grade{};          // %
    std::optional<double> parkingManeuvers{};  // per hour; absent: no parking
    double busesStopping{};                    // per hour
    std::optional<double> speedLimit{};        // km/h, posted (A2 queues, D373); absent: no queue
    bool operator==(const NodeAnalysisApproach&) const = default;
};
struct SaturationFlowSource {
    double value{}; std::string source;        // pc/h/ln, and where it came from
    bool operator==(const SaturationFlowSource&) const = default;
};
struct NodeAnalysis {
    bool cbd{};
    std::optional<SaturationFlowSource> baseSaturationFlow{}; // absent: 1,900 pc/h/ln
    std::optional<double> peakHourFactor{}; // absent: 0.92 at >= 1,000 veh/h entering, else 0.90
    std::optional<double> analysisPeriod{}; // h; absent: 0.25
    std::vector<NodeAnalysisApproach> approaches{};
    std::vector<NodeAnalysisTurn> turns{};
    bool operator==(const NodeAnalysis&) const = default;
};
}
