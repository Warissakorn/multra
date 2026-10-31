#include "node_detail.hpp"
#include "coarse_nodes.hpp"
#include "right_of_way.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace multra {
std::string legArm(const Link& l) { return l.streetSegmentId ? *l.streetSegmentId : l.id; }
namespace {
const Link* linkOf(const Network& n, const std::string& id) {
    const auto found = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
    return found == n.links.end() ? nullptr : &*found;
}
}
NodeCuts nodeLegCuts(const Network& network, const Node& node, const std::map<std::string, std::vector<Point>>& full) {
    NodeCuts result;
    auto t = network; // measured on the uncut legs
    for (auto& l : t.links) if (const auto f = full.find(l.id); f != full.end()) l.geometry = f->second;
    const auto legs = nodeLinkLegs(t, node);
    try {
        for (const auto& leg : legs) {
            const auto& l = *linkOf(t, leg.linkId);
            const double length = polylineLength(l.geometry);
            double cut = leg.in ? std::numeric_limits<double>::infinity() : -std::numeric_limits<double>::infinity();
            bool met = false;
            for (const auto& other : legs) {
                const auto& m = *linkOf(t, other.linkId);
                if (legArm(m) == legArm(l)) continue;
                for (const auto& a : l.lanes)
                    for (const auto& b : m.lanes)
                        for (const auto& piece : surfaceOverlaps(t, {l.id, a.id, "", "", ""}, {m.id, b.id, "", "", ""})) {
                            if (piece.status == SurfaceOverlap::Status::unsupported) { result.reason = "FILL_NODE_CUT_UNMEASURED"; return result; }
                            if (piece.status != SurfaceOverlap::Status::overlap) continue;
                            // Only an overlap reaching this Node's side of the crossing; another junction's is not this one's.
                            if (leg.in ? piece.first.to < leg.station : piece.first.from > leg.station) continue;
                            met = true;
                            cut = leg.in ? std::min(cut, piece.first.from) : std::max(cut, piece.first.to);
                        }
            }
            if (!met) { result.cuts.push_back({l.id, leg.in, leg.in ? length : 0., l.geometry}); continue; }
            // The leg must still cross the boundary: an in-leg ends past its entry, an out-leg starts before its exit.
            if (leg.in ? !(cut > leg.station) : !(cut < leg.station)) { result.reason = "FILL_NODE_CUT_OUTSIDE"; return result; }
            cut = std::clamp(cut, 0., length);
            result.cuts.push_back({l.id, leg.in, cut, leg.in ? polylineSpan(l.geometry, 0, cut) : polylineSpan(l.geometry, cut, length)});
        }
    } catch (const std::exception&) {
        result.reason = "FILL_NODE_CUT_UNMEASURED";
        result.cuts.clear();
    }
    return result;
}
std::vector<TurnConnector> nodeTurnConnectors(const Network& n, const Node& node) {
    std::vector<TurnConnector> turns;
    const auto legs = nodeLinkLegs(n, node);
    const auto banned = [&](const std::string& in, const std::string& out) {
        if (!node.coarse) return false;
        const auto& b = node.coarse->bannedTurns;
        return std::find(b.begin(), b.end(), std::pair{in, out}) != b.end();
    };
    const double kerb = n.drivingSide == DrivingSide::left ? 1. : -1.; // left of travel is positive (offsetGeometry)
    for (const auto& in : legs) {
        if (!in.in) continue;
        const auto& from = *linkOf(n, in.linkId);
        const auto d1 = directionAlong(from.geometry, polylineLength(from.geometry), true);
        std::vector<std::pair<double, const Link*>> outs;
        for (const auto& out : legs) {
            if (out.in) continue;
            const auto& to = *linkOf(n, out.linkId);
            if (legArm(to) == legArm(from) || banned(legArm(from), legArm(to))) continue;
            const auto d2 = directionAlong(to.geometry, 0, false);
            // The sweep towards the kerb: the kerb turn has the largest value.
            outs.push_back({kerb * std::atan2(d1.x * d2.y - d1.y * d2.x, d1.x * d2.x + d1.y * d2.y), &to});
        }
        std::stable_sort(outs.begin(), outs.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        const int k = static_cast<int>(outs.size()), lanes = static_cast<int>(from.lanes.size());
        for (int i = 0; i < k; ++i) {
            const int first = i * lanes / k, last = ((i + 1) * lanes + k - 1) / k - 1;
            const int count = std::max(1, last - first + 1);
            const auto& to = *outs[static_cast<std::size_t>(i)].second;
            const int outLanes = static_cast<int>(to.lanes.size()), toCount = std::min(count, outLanes);
            const bool far = k > 1 && i == k - 1;
            turns.push_back({from.id, to.id, first, count, far ? outLanes - toCount : 0, toCount, outs[static_cast<std::size_t>(i)].first});
        }
    }
    return turns;
}
int movementRank(bool inMajor, bool outMajor, double sweep, std::size_t arms) {
    if (inMajor) return outMajor || sweep > 0 ? 1 : 2;
    if (!outMajor) return 3;
    return sweep > 0 ? 2 : arms == 3 ? 3 : 4;
}
SignalLayout signalLayout(const CoarseSignal& signal, double step) {
    SignalLayout layout;
    const auto steps = [&](double v) { return step > 0 ? v / step : v; };
    const auto onGrid = [&](double v) { return !(step > 0) || std::abs(steps(v) - std::round(steps(v))) < 1e-6; };
    double total = 0;
    layout.offGrid = !onGrid(signal.cycle);
    for (const auto& s : signal.stages) { total += s.green; layout.offGrid = layout.offGrid || !onGrid(s.green); }
    if (signal.stages.empty()) return layout;
    const auto n = static_cast<double>(signal.stages.size());
    const double share = (signal.cycle - total) / n;
    const double each = step > 0 ? std::floor(share / step + 1e-6) * step : share;
    const double last = signal.cycle - total - each * (n - 1);
    layout.intergreen = std::min(each, last);
    double at = 0;
    for (const auto& s : signal.stages) { layout.starts.push_back(at); at += s.green + each; }
    return layout;
}
TurnGreen turnGreen(const CoarseSignal& signal, const SignalLayout& layout, const std::pair<std::string, std::string>& turn) {
    TurnGreen out;
    const auto n = signal.stages.size();
    std::vector<bool> serves(n);
    for (std::size_t s = 0; s < n; ++s) {
        const auto& t = signal.stages[s].turns;
        serves[s] = std::find(t.begin(), t.end(), turn) != t.end();
    }
    const auto count = static_cast<std::size_t>(std::count(serves.begin(), serves.end(), true));
    if (count == 0) { out.reason = "FILL_NODE_TURN_UNSERVED"; return out; }
    std::size_t first = 0; // the run's first stage: served, with the one before it not
    if (count < n) while (!serves[first] || serves[(first + n - 1) % n]) ++first;
    for (std::size_t k = 0; k < count; ++k) {
        const auto s = (first + k) % n;
        if (!serves[s]) { out.reason = "FILL_NODE_STAGES_SPLIT"; return out; }
        out.stages.push_back(s);
    }
    const auto last = out.stages.back();
    out.start = layout.starts[first];
    out.end = layout.starts[last] + signal.stages[last].green;
    return out;
}
}
