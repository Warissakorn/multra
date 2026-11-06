#pragma once
// M10.4 E1 (MODE_CHOICE §3, §5, D477-D478): a multinomial logit split of a P-A table over alternatives,
// V_k = ASC_k + theta_k c_k, P_k = exp(V_k) / sum over the available exp(V_m) (Ortuzar and Willumsen 2011,
// Eq. 7.9, beta normalised to 1), and the directional split of P-A trips into a period's OD (§5.4.4).
// Pure arithmetic over dense zone indices.
#include "trip_distribution.hpp"
#include <vector>

namespace multra {
// Shares over `utilities`; a NaN or -infinity utility is an unavailable alternative (share 0). Computed as
// exp(V - max V), so large costs cannot overflow. No available alternative: every share 0.
std::vector<double> logitShares(const std::vector<double>& utilities);

struct ModeAlternativeInput {
    double constant{}, timeCoefficient{};   // theta per minute, <= 0
    DistributionMatrix costs;               // minutes, [origin][destination]; +infinity where unavailable
};
struct ModeSplitIssue { std::size_t origin{}, destination{}; }; // trips > 0 and no available alternative
struct ModeSplit {
    std::vector<DistributionMatrix> persons; // one per alternative, in the given order
    std::vector<ModeSplitIssue> issues;      // non-empty: nothing else is filled
};
ModeSplit splitModes(const DistributionMatrix& trips, const std::vector<ModeAlternativeInput>&);
// fPA T + fAP T^T: the P-A trips travelling production to attraction and back in one period.
DistributionMatrix directionalOd(const DistributionMatrix& trips, double fPA, double fAP);
}
