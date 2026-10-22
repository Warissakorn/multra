#include "graph.hpp"

namespace multra {
std::size_t mesoShortestLink(const MesoGraph& g) {
    std::size_t best = g.links.size();
    for (std::size_t i = 0; i < g.links.size(); ++i) {
        const auto& l = g.links[i];
        if (l.freeSpeed <= 0) continue;
        if (best == g.links.size() || l.length / l.freeSpeed < g.links[best].length / g.links[best].freeSpeed) best = i;
    }
    return best;
}
double mesoStepLimit(const MesoGraph& g) {
    const auto i = mesoShortestLink(g);
    return i == g.links.size() ? 0.0 : g.links[i].length / g.links[i].freeSpeed;
}
}
