#include "nodes.hpp"

namespace trafficsim {
std::optional<double> weightedDelay(const std::vector<std::pair<std::uint64_t, std::optional<double>>>& rows) {
    double weighted = 0; std::uint64_t vehicles = 0;
    for (const auto& [v, d] : rows) if (v && d) { weighted += static_cast<double>(v) * *d; vehicles += v; }
    if (!vehicles) return std::nullopt;
    return weighted / static_cast<double>(vehicles);
}
std::vector<NodeRow> nodeRows(const std::vector<NodeSpec>& specs, const std::vector<SectionRow>& turns,
                              const std::vector<std::pair<double, double>>& queues) {
    std::vector<NodeRow> rows;
    std::size_t turnAt = 0, legAt = 0;
    for (const auto& spec : specs) {
        NodeRow row{spec.name, spec.controlType, 0, {}, 0, {}, {}, spec.conflicts, spec.issues};
        for (const auto& leg : spec.legs) {
            const auto& q = queues.at(legAt++);
            row.legs.push_back({leg.name, 0, {}, 0, q.first, q.second});
        }
        std::vector<std::vector<std::pair<std::uint64_t, std::optional<double>>>> byLeg(spec.legs.size());
        std::vector<std::pair<std::uint64_t, std::optional<double>>> all;
        for (const auto& turn : spec.turns) {
            const auto& s = turns.at(turnAt++);
            row.turns.push_back({turn.section.name, s.vehicles, s.meanDelay, s.meanTravelTime, s.unfinished});
            auto& leg = row.legs.at(turn.leg);
            leg.vehicles += s.vehicles; leg.unfinished += s.unfinished;
            row.vehicles += s.vehicles; row.unfinished += s.unfinished;
            byLeg[turn.leg].push_back({s.vehicles, s.meanDelay}); all.push_back({s.vehicles, s.meanDelay});
        }
        for (std::size_t l = 0; l < row.legs.size(); ++l) row.legs[l].delay = weightedDelay(byLeg[l]);
        row.delay = weightedDelay(all);
        rows.push_back(std::move(row));
    }
    return rows;
}
}
