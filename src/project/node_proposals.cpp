#include "node_proposals.hpp"
#include "meso_output.hpp"
#include "../model/demand/coarse_compile.hpp"
#include "../model/network/coarse_nodes.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace multra {
namespace {
struct Hot { ProposalLeg leg; bool fromJunction{}, toJunction{}; };
struct LegEnd { std::string source; double station{}; bool insideAfter{}; }; // inside the polygon past `station`?
std::string endName(const std::string& source, double station) {
    std::ostringstream s; s.imbue(std::locale::classic()); s << source << '@' << station; return s.str();
}
std::vector<NodeProposal> propose(const Network& n, ProposalLevel level, std::uint64_t revision, const std::vector<Hot>& hot) {
    std::vector<NodeProposal> out;
    std::vector<bool> covered(hot.size());
    for (const auto& node : n.nodes) {
        std::vector<LegEnd> ends;
        for (const auto& l : nodeLinkLegs(n, node)) ends.push_back({l.linkId, l.station, l.in});
        for (const auto& r : nodeRoadLegs(n, node)) ends.push_back({r.roadId, r.station, r.atEnd});
        NodeProposal p{level, revision, node.id, {}, {}};
        for (std::size_t h = 0; h < hot.size(); ++h) {
            const auto& leg = hot[h].leg;
            // Macro keeps a detailed Node's Links whole and Meso cuts them at the boundary, so the arc
            // need only reach the crossing from outside: its range touches the inside side.
            const auto lo = std::min(leg.fromStation, leg.toStation), hi = std::max(leg.fromStation, leg.toStation);
            const bool isLeg = std::any_of(ends.begin(), ends.end(), [&](const LegEnd& e) {
                return e.source == leg.source && (e.insideAfter ? hi >= e.station - roadEndTolerance : lo <= e.station + roadEndTolerance);
            });
            if (isLeg) { p.legs.push_back(leg); covered[h] = true; }
        }
        if (!p.legs.empty()) out.push_back(std::move(p));
    }
    std::map<std::string, NodeProposal> loose;
    for (std::size_t h = 0; h < hot.size(); ++h) {
        if (covered[h]) continue;
        const auto& x = hot[h];
        for (const auto& [junction, station] : {std::pair{x.fromJunction, x.leg.fromStation}, std::pair{x.toJunction, x.leg.toStation}}) {
            if (!junction) continue;
            const auto name = endName(x.leg.source, station);
            auto& p = loose.try_emplace(name, NodeProposal{level, revision, {}, name, {}}).first->second;
            p.legs.push_back(x.leg);
        }
    }
    for (auto& [name, p] : loose) out.push_back(std::move(p));
    return out;
}
}
std::vector<NodeProposal> macroProposals(const ProjectDocument& d, const MacroRun& r) {
    if (r.revision != d.revision) return {};
    const auto& g = r.graph;
    std::vector<bool> junction(g.nodes.size());
    for (const auto& a : g.links)
        if (a.kind == MacroArc::connector) { junction.at(a.from) = true; junction.at(a.to) = true; }
    std::vector<Hot> hot;
    for (std::size_t k = 0; k < g.links.size() && k < r.assignment.load.size(); ++k) {
        const auto& a = g.links[k];
        if (a.kind != MacroArc::link || a.capacity <= 0) continue;
        const double vc = r.assignment.load[k] / a.capacity;
        if (vc >= 1.0) hot.push_back({{a.source, a.fromStation, a.toStation, vc, 0}, junction.at(a.from), junction.at(a.to)});
    }
    return propose(d.network, ProposalLevel::macro, r.revision, hot);
}
std::vector<NodeProposal> mesoProposals(const ProjectDocument& d, const MesoRun& r) {
    if (r.revision != d.revision) return {};
    const auto& g = r.graph;
    const auto junction = [&](std::size_t i) { return i < g.nodes.size() && !g.nodes[i].turns.empty() && g.nodes[i].nodeId.empty(); };
    std::vector<Hot> hot;
    for (std::size_t l = 0; l < g.links.size() && l < r.loading.receiving.size(); ++l) {
        std::size_t spill = 0;
        for (const auto& s : mesoLinkSlices(r, l)) spill += s.spillBack ? 1 : 0;
        const auto& a = g.links[l];
        if (spill) hot.push_back({{a.source, a.fromStation, a.toStation, std::nullopt, spill}, junction(a.from), junction(a.to)});
    }
    return propose(d.network, ProposalLevel::meso, r.revision, hot);
}
}
