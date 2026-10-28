#include "permitted.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
constexpr double kStartUp = 2.0, kExtension = 2.0; // l_1 and e (Exhibit 31-12, PDF p. 2986)
}
double permittedSaturation(double opposingFlow) {
    const double vo = opposingFlow > 0 ? opposingFlow : 0.1;
    return vo * std::exp(-vo * kCriticalHeadway / 3600) / (1 - std::exp(-vo * kFollowUpHeadway / 3600));
}
double opposingArrivals(double permittedGreen, double unblockedGreen, double firstBlock) {
    return std::max(0.0, 0.278 * (permittedGreen - unblockedGreen - firstBlock));
}
double queuedEquivalent(double opposingFarShare, double opposingArrivals) {
    const double p = opposingFarShare;
    const double e = p > 1e-12 ? (1 - std::pow(1 - p, opposingArrivals)) / p : opposingArrivals;
    return std::max(kProtectedFarEquivalent, e);
}
double modifiedEquivalent(double equivalent, double laneChange) { return (equivalent - 1) * laneChange + 1; }
PermittedGreen permittedGreen(double windowStart, double windowEnd, double opposingStart, double opposingQueue,
                              bool opensDisplay, double gapAfterProtected, bool endsDisplay) {
    PermittedGreen g;
    g.displayed = std::max(0.0, windowEnd - windowStart);
    g.unblockedDisplayed = std::max(0.0, windowEnd - std::max(windowStart, opposingStart + opposingQueue));
    g.startupLost = opensDisplay ? kStartUp : std::clamp(gapAfterProtected + kStartUp - kExtension, 0.0, kStartUp);
    g.extension = endsDisplay ? kExtension : 0;
    g.effective = std::max(0.0, g.displayed - g.startupLost + g.extension);           // Eq. 31-94
    g.unblocked = std::min(g.effective, g.unblockedDisplayed + g.extension);           // Eq. 31-95
    return g;
}
double firstBlockTime(double displayedPermitted, double effectivePermitted, double startupLost, double farShare,
                      double farFlow, double cycle, bool oneLane) {
    const double pl = farShare;
    // Eq. 31-96; as P_L -> 0 the bracket tends to g_p.
    const double reach = pl > 1e-12 ? (1 - pl) / (0.5 * pl) * (1 - std::pow(1 - pl, 0.5 * effectivePermitted)) : effectivePermitted;
    const double most = std::max(0.0, reach - startupLost);
    const double ltc = farFlow * cycle / 3600; // Eq. 31-99
    const double gf = oneLane ? displayedPermitted * std::exp(-0.860 * std::pow(ltc, 0.629))  // Eq. 31-97
                              : displayedPermitted * std::exp(-0.882 * std::pow(ltc, 0.717)); // Eq. 31-98
    return std::min(std::max(gf - startupLost, 0.0), most);
}
double sharedSneakers(double farShare) {
    const double pl = farShare;
    if (pl >= 0.999) return kSneakersPerCycle * pl;
    return pl / (1 - pl) * (1 - std::pow(pl, kSneakersPerCycle));
}
SharedPeriods sharedPeriods(double permittedGreen, double unblockedGreen, double firstBlock) {
    return {firstBlock, std::max(0.0, permittedGreen - unblockedGreen - firstBlock),
            std::max(0.0, std::min(permittedGreen - firstBlock, unblockedGreen))};
}
double sharedPermittedShare(const SharedPeriods& p, double permittedGreen, double farShare, double queuedEq,
                            double filteringEq, bool secondServed, double sneakersOverSaturation) {
    if (!(permittedGreen > 0)) return 0;
    const double second = secondServed ? p.second / (1 + farShare * (queuedEq - 1)) : 0;
    const double third = p.third / (1 + farShare * (filteringEq - 1));
    return (p.first + second + third + 3600 * sneakersOverSaturation) / permittedGreen;
}
double exclusivePermittedCapacity(double unblockedGreen, double permittedSaturation, double cycle, int lanes,
                                  double protectedGreen, double protectedSaturation) {
    if (!(cycle > 0)) return 0;
    return (protectedGreen * protectedSaturation + unblockedGreen * permittedSaturation + 3600 * kSneakersPerCycle) / cycle * lanes;
}
double sharedPermittedCapacity(double permittedGreen, double sharedSaturation, double farShare, double cycle,
                               double protectedGreen, double protectedSaturation) {
    if (!(cycle > 0)) return 0;
    return (protectedGreen * protectedSaturation + permittedGreen * sharedSaturation + 3600 * (1 + farShare)) / cycle;
}
}
