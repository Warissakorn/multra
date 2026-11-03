#include "utdf_signals.hpp"
#include "../model/demand/nema.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>

namespace multra::utdf {
namespace {
std::string number(double v, int decimals, bool fixed) {
    std::ostringstream s; s.imbue(std::locale::classic());
    s << std::fixed << std::setprecision(decimals) << v;
    auto t = s.str();
    if (!fixed && t.find('.') != std::string::npos) {
        while (t.back() == '0') t.pop_back();
        if (t.back() == '.') t.pop_back();
    }
    if (t.find_first_not_of("-0.") == std::string::npos && t.front() == '-') t.erase(0, 1);
    return t;
}
}
NodeSignal nodeSignal(const ProjectDocument& d, const WrittenNode& w) {
    NodeSignal out;
    const auto& m = w.layout.membership;
    if (m.control != SectionControl::signalised) return out;
    out.kind = NodeSignal::Kind::invalid;
    std::set<std::string> controllers;
    for (const auto& id : m.signalHeads)
        for (const auto& h : d.network.signalHeads) if (h.id == id) controllers.insert(h.controllerId);
    if (controllers.size() != 1 || controllers.begin()->empty() || !d.definition) {
        out.issues.push_back({"NEMA_HEAD_GROUP", "the Node's heads must all belong to one signal controller"});
        return out;
    }
    const auto& list = d.definition->signalControllers;
    const auto c = std::find_if(list.begin(), list.end(), [&](const auto& x) { return x.id == *controllers.begin(); });
    if (c == list.end()) { out.issues.push_back({"NEMA_HEAD_GROUP", *controllers.begin()}); return out; }
    out.controller = &*c;
    out.issues = nemaCheck(*c, {d.network, w.layout});
    if (!out.issues.empty()) return out;
    for (const auto& movement : w.layout.movements) {
        const auto programs = turnSignalPrograms(d.network, m, m.turns.at(movement.turn));
        for (const auto& g : c->groups) if (programs.count(signalGroupProgramId(c->id, g.number))) out.groups[movement.turn] = g.number;
    }
    const auto timing = timingRecords(*c, w.number);
    if (!timing.issues.empty()) { out.issues = timing.issues; return out; }
    out.kind = NodeSignal::Kind::valid;
    return out;
}
TimingRecords timingRecords(const SignalController& c, int nodeNumber) {
    TimingRecords out;
    const double cycle = c.cycle;
    const auto wrap = [&](double x) { double r = std::fmod(x, cycle); if (r < 0) r += cycle; return std::abs(r - cycle) < 1e-9 ? 0. : r; };
    std::map<int, const SignalGroup*> groups;
    for (const auto& g : c.groups) if (g.number >= 1 && g.number <= 8) groups[g.number] = &g;
    std::map<int, double> start, end, yield, green, split, allRed;
    for (const auto& [n, g] : groups) { start[n] = wrap(g->greenStart); yield[n] = wrap(g->greenEnd); green[n] = signalGroupGreen(c, *g); }
    for (int ring = 0; ring < 2; ++ring) {
        std::vector<int> order;
        for (const auto& [n, g] : groups) { (void)g; if ((n - 1) / 4 == ring) order.push_back(n); }
        std::sort(order.begin(), order.end(), [&](int a, int b) { return start[a] < start[b]; });
        for (std::size_t i = 0; i < order.size(); ++i) {
            const int n = order[i], next = order[(i + 1) % order.size()];
            end[n] = start[next];
            double s = wrap(end[n] - start[n]); if (s < 1e-9) s = cycle;
            split[n] = s;
            double clear = wrap(end[n] - yield[n]);
            if (order.size() == 1 && clear < 1e-9) clear = cycle - green[n];
            allRed[n] = clear - groups[n]->amber;
            if (allRed[n] < -1e-6) out.issues.push_back({"NEMA_TIMING", c.id + ".groups[" + std::to_string(n) + "]"});
            allRed[n] = std::max(0., allRed[n]);
        }
    }
    if (!out.issues.empty()) return out;
    const auto two = groups.count(2) ? start[2] : 0.;
    out.timeplans = {{"Control Type", "0"}, {"Cycle Length", number(cycle, 1, true)}, {"Lock Timings", "0"},
                     {"Referenced To", "0"}, {"Reference Phase", "206"}, {"Offset", number(wrap(two - c.offset), 1, true)},
                     {"Master", "0"}, {"Yield", "0"}, {"Node 0", std::to_string(nodeNumber)}, {"Node 1", "0"}};
    const auto row = [&](const std::string& name, auto value) {
        PhaseRecord r{name, {}};
        for (const auto& [n, g] : groups) { (void)g; r.second[static_cast<std::size_t>(n - 1)] = value(n); }
        out.phases.push_back(std::move(r));
    };
    const auto partner = [](int n) { const int base = (n - 1) / 2 * 2 + 1; return n == base ? base + 1 : base; };
    const auto t = [](double v) { return number(v, 1, false); };
    const auto f = [](double v) { return number(v, 1, true); };
    // BRP is written for all eight phases, as Synchro 10.1 and u0-handwritten-v3.csv do; an absent
    // phase keeps its nominal position (odd leads).
    PhaseRecord brp{"BRP", {}};
    for (int n = 1; n <= 8; ++n) {
        const int barrier = (n - 1) % 4 < 2 ? 1 : 2, ring = (n - 1) / 4 + 1, p = partner(n);
        int position = n % 2 == 1 ? 1 : 2;
        if (groups.count(n) && groups.count(p)) position = std::abs(wrap(end[n] - start[p])) < 1e-6 ? 1 : 2;
        brp.second[static_cast<std::size_t>(n - 1)] = std::to_string(barrier * 100 + ring * 10 + position);
    }
    out.phases.push_back(std::move(brp));
    row("MinGreen", [&](int n) { return t(green[n]); });
    row("MaxGreen", [&](int n) { return t(green[n]); });
    for (const auto* name : {"VehExt", "TimeBeforeReduce", "TimeToReduce", "MinGap"}) row(name, [](int) { return std::string("0.0"); });
    row("Yellow", [&](int n) { return f(groups[n]->amber); });
    row("AllRed", [&](int n) { return f(allRed[n]); });
    row("Recall", [](int) { return std::string("3"); });
    out.phases.push_back({"Walk", {}}); out.phases.push_back({"DontWalk", {}});
    row("PedCalls", [](int) { return std::string("0"); });
    row("MinSplit", [&](int n) { return t(split[n]); });
    row("DualEntry", [](int) { return std::string("0"); });
    row("InhibitMax", [](int) { return std::string("1"); });
    row("Start", [&](int n) { return t(start[n]); });
    row("End", [&](int n) { return t(end[n]); });
    row("Yield", [&](int n) { return t(yield[n]); });
    row("Yield170", [&](int n) { return t(yield[n]); });
    row("LocalStart", [&](int n) { return t(start[n]); });
    row("LocalYield", [&](int n) { return t(yield[n]); });
    row("LocalYield170", [&](int n) { return t(yield[n]); });
    row("ActGreen", [&](int n) { return t(green[n]); });
    return out;
}
}
