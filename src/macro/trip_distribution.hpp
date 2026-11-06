#pragma once
// M10.3 D1 (TRIP_DISTRIBUTION §3-§4, D472-D473): a doubly-constrained gravity model with
// exponential deterrence, T_ij = a_i b_j exp(-beta c_ij), balanced by row-first Furness passes
// (Ortuzar and Willumsen 2011, Eq. 5.7 and Example 5.4). Pure arithmetic over dense zone indices.
#include <cstdint>
#include <optional>
#include <vector>

namespace multra {
using DistributionMatrix = std::vector<std::vector<double>>; // [origin][destination]
struct DistributionSettings {
    double target{1e-3};              // largest relative trip-end error (§4); choice 4a
    int maxPasses{1000};
    std::optional<int> fixedPasses;   // tests only: exactly this many passes, then stop
};
// A refusal located by index (§4); the project layer turns it into a path.
struct DistributionIssue {
    enum class Code { unbalanced, rowImpossible, columnImpossible } code{};
    std::size_t zone{};               // the row or column; 0 for unbalanced
};
struct FurnessResult {
    DistributionMatrix trips;
    std::vector<double> a, b;         // products of the row and of the column corrections
    int passes{};
    double error{};                   // after the last pass
    bool converged{};
};
// One pass corrects every row to its target, then every column. A zero target zeroes its line and
// its factor. The prior's zeros stay zeros; feasibility is the caller's check.
FurnessResult furness(DistributionMatrix prior, const std::vector<double>& rowTargets, const std::vector<double>& columnTargets,
                      const DistributionSettings&);
// The largest relative row or column error over the nonzero targets.
double tripEndError(const DistributionMatrix&, const std::vector<double>& rowTargets, const std::vector<double>& columnTargets);

struct DistributionInput {
    DistributionMatrix costs;         // minutes; +infinity where unreachable
    std::vector<double> productions, attractions;
    double beta{};                    // per minute, > 0
    double intrazonalShare{};         // in [0, 1): T_ii = s min(P_i, A_i) set aside (choice 2a)
    bool useDiagonal{};               // false: f_ii = 0 (the diagonal leaves the gravity model)
};
struct DistributionResult {
    DistributionMatrix trips;         // the set-aside intrazonal trips included on the diagonal
    std::vector<double> a, b;         // the book's normalisation: b carries T / sum f (§3)
    std::vector<double> intrazonal;
    int passes{};
    double error{};
    bool converged{};
    std::vector<DistributionIssue> issues; // non-empty: nothing else is filled
};
DistributionResult distributeTrips(const DistributionInput&, const DistributionSettings&);
// sum T c / sum T over finite costs; 0 when there are no trips.
double meanCost(const DistributionMatrix& trips, const DistributionMatrix& costs);
// Trips by cost band: band 0 is [0, w], band k is (k w, (k+1) w]; as many bands as the largest
// finite cost with trips needs.
std::vector<double> tripLengthBands(const DistributionMatrix& trips, const DistributionMatrix& costs, double width);
}
