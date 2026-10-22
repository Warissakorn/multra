#include "graph.hpp"
#include <algorithm>

namespace multra {
double mesoWaveSpeed(const MesoLink& l) {
    if (!(l.freeSpeed > 0) || !(l.jamDensity > 0)) return 0;
    const double q = l.capacityPerLane / 3600.0, kc = q / l.freeSpeed, kj = l.jamDensity / 1000.0; // per lane, SI
    return kj > kc ? q / (kj - kc) : 0;
}
double mesoLinkTimeLimit(const MesoLink& l) {
    const double w = mesoWaveSpeed(l);
    return w > 0 ? std::min(l.length / l.freeSpeed, l.length / w) : l.length / l.freeSpeed;
}
std::size_t mesoShortestLink(const MesoGraph& g) {
    std::size_t best = g.links.size();
    for (std::size_t i = 0; i < g.links.size(); ++i) {
        const auto& l = g.links[i];
        if (l.freeSpeed <= 0) continue;
        if (best == g.links.size() || mesoLinkTimeLimit(l) < mesoLinkTimeLimit(g.links[best])) best = i;
    }
    return best;
}
double mesoStepLimit(const MesoGraph& g) {
    const auto i = mesoShortestLink(g);
    return i == g.links.size() ? 0.0 : mesoLinkTimeLimit(g.links[i]);
}
}
