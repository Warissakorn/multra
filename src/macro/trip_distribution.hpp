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
// D3 (§15, D476): doubly balances both trip ends; origin meets the productions only (Eq. 5.16,
// B_j = 1), the attractions being weights and the columns floating.
// D4 (§16, D482): destination meets the attractions only (A_i = 1, Eq. 5.14 with 5.18, p. 183), the
// productions being weights and the rows floating.
enum class DistributionConstraint { doubly, origin, destination };
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
// D3: one row normalisation of `weights` (A_j f_ij), no column pass: a_i = O_i / sum_k w_ik (0 for a
// zero target), b = 1, passes 1, the row error, converged. A zero row with a positive target is the
// caller's check.
FurnessResult originConstrained(DistributionMatrix weights, const std::vector<double>& rowTargets);
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
    DistributionConstraint constraint{DistributionConstraint::doubly}; // origin: T_ii = s P_i set aside (choice 4a); destination: s A_i (§16)
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
