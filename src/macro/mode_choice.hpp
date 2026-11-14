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
// M10.4 N1 (NESTED_LOGIT §2, D481): a two-level nested logit (Ortuzar and Willumsen 2011, Eq. 7.19/7.23 with
// beta = 1, phi_n = mu_n in (0, 1]). nestOf[k] is k's nest or -1 (a branch of its own at the root); a nest
// with no available member drops out. W_n = m_n + phi_n ln sum exp((V_k - m_n)/phi_n), P(n) = logit over W,
// P(k) = P(n) exp((V_k - m_n)/phi_n) / sum. At phi = 1, or with every nestOf -1, this is logitShares.
std::vector<double> nestedLogitShares(const std::vector<double>& utilities, const std::vector<int>& nestOf, const std::vector<double>& phi);
struct ModeNests {
    std::vector<int> nestOf;    // per alternative; empty: the MNL
    std::vector<double> phi;    // per nest
};
struct ModeSplitIssue { std::size_t origin{}, destination{}; }; // trips > 0 and no available alternative
struct ModeSplit {
    std::vector<DistributionMatrix> persons; // one per alternative, in the given order
    std::vector<ModeSplitIssue> issues;      // non-empty: nothing else is filled
};
// D548 (MODE_CHOICE_LOGSUM §3): the composite cost of one purpose's alternatives sharing lambda = -theta,
// K_ij = -ln(sum over branches exp W) / lambda (Ortuzar and Willumsen 2011, Eq. 6.10; W as nestedLogitShares,
// the MNL without nests); +infinity where no alternative is available.
double logsum(const std::vector<double>& utilities, const std::vector<int>& nestOf, const std::vector<double>& phi);
DistributionMatrix compositeCost(const std::vector<ModeAlternativeInput>&, const ModeNests&, double lambda);
ModeSplit splitModes(const DistributionMatrix& trips, const std::vector<ModeAlternativeInput>&, const ModeNests& = {});
// fPA T + fAP T^T: the P-A trips travelling production to attraction and back in one period.
DistributionMatrix directionalOd(const DistributionMatrix& trips, double fPA, double fAP);
}
