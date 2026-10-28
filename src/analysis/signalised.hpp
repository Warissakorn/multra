#pragma once
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// M13 A1 (docs/reference/NODE_ANALYSIS.md §3, D370, D371): the HCM 7th edition Chapter 19 core
// motorized vehicle methodology for one lane group under pretimed control, protected, in exclusive
// lanes, with no initial queue, at an isolated intersection. Pure: values in, values out. Page
// numbers are the PDF pages of the owner's copy (BOOKS §2). Analytical, not simulated; not validated.
namespace multra {
// HCM's right turn is the kerb turn and its left turn the far turn, for either driving side (as
// movementRank in node_detail.hpp reads Ch. 20).
enum class LaneGroupMovement { through, kerbTurn, farTurn };
struct SignalisedLaneGroup {
    int lanes{1};
    LaneGroupMovement movement{};
    double flowRate{};               // v, veh/h: the hourly volume over the peak hour factor (Steps 2-3)
    double baseSaturation{1900};     // s0, pc/h/ln (Exhibit 19-11, PDF p. 1430)
    double laneWidth{3.6576};        // m; 12 ft is HCM's default (Exhibit 19-11, PDF p. 1431)
    double heavyPercent{3}, gradePercent{};
    std::optional<double> parkingManeuvers; // per hour beside the lane group; absent: no parking
    double busesStopping{};          // per hour blocking the lane group
    bool cbd{};
    double effectiveGreen{}, cycle{}; // g and C, s
    double arrivalsOnGreen{};        // P (Eq. 19-15: Rp g/C)
    double period{0.25};             // T, h
    double incrementalFactor{0.5};   // k: 0.50 for pretimed phases (Step 8C, PDF p. 1476)
    double upstreamFiltering{1.0};   // I: 1.0 for an isolated intersection
    // A2: a shared lane group's s/s_th from Ch. 31 §2 (lane_flow.hpp). It replaces fLT and fRT, and
    // fLU is 1 for one shared lane (PDF p. 1469).
    std::optional<double> sharedShare;
    // A3: f_s for the through lanes beside a shared far-turn lane whose far turn is permitted (0.91,
    // Eq. 31-62, PDF p. 2959); 1 otherwise. It multiplies an exclusive lane group's saturation flow.
    double throughFactor{1};
};
// The factors of Eq. 19-8 (PDF p. 1465) that A1 computes; fLpb, fRpb, fwz, fms and fsp are 1.
// fShared is a shared lane group's s/s_th (A2), or the through lanes' f_s (A3), 1 otherwise.
struct SaturationFactors {
    double fw{1}, fHVg{1}, fp{1}, fbb{1}, fa{1}, fLU{1}, fLT{1}, fRT{1}, fShared{1};
    bool operator==(const SaturationFactors&) const = default;
};
struct SignalisedResult {
    SaturationFactors factors;
    double saturation{}, capacity{}, ratio{}, greenRatio{}, progression{};
    double uniformDelay{}, incrementalDelay{}, initialQueueDelay{}, delay{};
    bool operator==(const SignalisedResult&) const = default;
};
// The first input outside the method's stated range, as a code, else none:
// ANALYSIS_LANES (< 1), ANALYSIS_LANE_WIDTH (< 8 ft, Exhibit 19-20's note), ANALYSIS_HEAVY_VEHICLES
// (outside 0-50 %), ANALYSIS_GRADE (outside -4 to +10 %; both PDF p. 1467), ANALYSIS_PARKING (outside
// 0-180/h, PDF p. 1468), ANALYSIS_BUSES (outside 0-250/h, PDF p. 1469), ANALYSIS_TIMING (C <= 0, g <= 0
// or g > C), ANALYSIS_FLOW (v < 0, s0 <= 0, T <= 0, or a shared share outside (0, 1] or on more than
// one lane).
std::optional<std::string> signalisedRangeIssue(const SignalisedLaneGroup&);
// Exhibit 19-15 (PDF p. 1444): default lane utilization for exclusive lanes; more lanes than the
// exhibit shows take its smallest value for that movement (its note a).
double laneUtilization(LaneGroupMovement, int lanes);
SaturationFactors saturationFactors(const SignalisedLaneGroup&);
// Eq. 19-20 with y of Eq. 19-21 (PDF p. 1475).
double progressionFactor(double arrivalsOnGreen, double greenRatio, double ratio);
// Eq. 19-19 (PDF p. 1475).
double uniformDelay(double cycle, double greenRatio, double ratio, double progression);
// Eqs. 19-26 and 19-27 (PDF p. 1479), with cA = c when there is no initial queue.
double incrementalDelay(double period, double ratio, double factor, double filtering, double capacity);
// Steps 4-8 for one lane group; signalisedRangeIssue must be none.
SignalisedResult analyseSignalised(const SignalisedLaneGroup&);
// Eqs. 19-28 and 19-29 (PDF p. 1480): the flow-weighted mean of (flow, delay) pairs; none without flow.
std::optional<double> flowWeightedDelay(const std::vector<std::pair<double, double>>&);
// Exhibit 19-8 (PDF p. 1422): the first upper bound the delay does not exceed, else 'F'; a lane group
// over capacity (X > 1) is 'F' whatever its delay. Approaches and the intersection pass no ratio.
char signalisedLos(double delay, std::optional<double> ratio, const std::array<double, 5>& bounds);
}
