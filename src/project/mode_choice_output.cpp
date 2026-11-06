#include "mode_choice_output.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <sstream>

namespace multra {
namespace {
constexpr const char* kMeasure = "Mode choice (multinomial logit per purpose, P_k = exp(V_k) / sum exp(V_m) over the available "
    "alternatives, V = constant + coefficient x free-flow minutes; Ortuzar and Willumsen 2011, Eq. 7.9) on the gravity model's "
    "production-attraction trips, turned into each study period's OD by the project's period factors (Section 5.4.4) and into "
    "vehicles by each alternative's occupancy";
constexpr const char* kUnit = "Persons and vehicles are in the rate set's unit; they are trips in the period only if the rate set "
    "counts daily person trips";
double total(const DistributionMatrix& m) { double s = 0; for (const auto& r : m) for (const double x : r) s += x; return s; }
Json costs(const DistributionMatrix& m) {
    Json rows = Json::array();
    for (const auto& r : m) { Json cells = Json::array(); for (const double c : r) cells.push_back(std::isfinite(c) ? Json(c) : Json(nullptr)); rows.push_back(cells); }
    return rows;
}
}
Json modeChoiceJson(const ModeChoiceRun& r) {
    const auto& g = r.distribution.generation;
    Json source = Json::object();
    if (!r.set.citation.empty()) source["citation"] = r.set.citation;
    if (!r.set.table.empty()) source["table"] = r.set.table;
    Json j{{"validated", false}, {"measure", kMeasure}, {"unit", kUnit}, {"revision", g.revision},
           {"rateSet", {{"id", g.set.id}, {"unit", g.set.unit}}}, {"deterrenceSet", r.distribution.deterrence.id},
           {"modeChoiceSet", {{"id", r.set.id}, {"name", r.set.name}, {"source", source}}},
           {"zoneIds", g.zoneIds}, {"purposes", Json::array()}, {"periods", Json::array()}, {"advisories", Json::array()},
           {"inputManifest", g.inputManifest}};
    for (const auto& p : r.purposes) {
        Json alternatives = Json::array();
        for (const auto& a : p.alternatives)
            alternatives.push_back({{"id", a.alternative.id}, {"modeId", a.alternative.modeId ? Json(*a.alternative.modeId) : Json(nullptr)},
                                    {"persons", total(a.persons)}, {"costs", costs(a.costs)}, {"trips", a.persons}});
        j["purposes"].push_back({{"id", p.id}, {"alternatives", alternatives}});
    }
    for (const auto& s : r.periods) {
        Json alternatives = Json::array(), modes = Json::array();
        for (const auto& a : s.alternatives)
            alternatives.push_back({{"id", a.id}, {"modeId", a.modeId ? Json(*a.modeId) : Json(nullptr)}, {"persons", a.persons},
                                    {"vehicles", a.vehicles ? Json(*a.vehicles) : Json(nullptr)}});
        for (const auto& m : s.modes) modes.push_back({{"modeId", m.modeId}, {"vehicles", m.vehicles}, {"total", total(m.vehicles)}});
        j["periods"].push_back({{"id", s.id}, {"alternatives", alternatives}, {"modes", modes}});
    }
    for (const auto& a : r.advisories) j["advisories"].push_back({{"code", a.code}, {"path", a.path}});
    return j;
}
std::string modeChoiceCsv(const ModeChoiceRun& r) {
    const auto& g = r.distribution.generation;
    std::ostringstream out;
    out << "# MULTRA - not yet validated. " << kMeasure << ".\n";
    out << "# " << kUnit << ".\n";
    out << "# Rate set: " << Json(g.set.id).dump() << "; deterrence set: " << Json(r.distribution.deterrence.id).dump()
        << "; mode-choice set: " << Json(r.set.id).dump() << "; unit: " << Json(g.set.unit).dump() << ".\n";
    for (const auto& a : r.advisories) out << "# " << a.code << " (" << a.path << ").\n";
    out << "period,alternative,originId,destinationId,persons,vehicles\n";
    for (const auto& s : r.periods)
        for (const auto& a : s.alternatives) {
            for (std::size_t i = 0; i < g.zoneIds.size(); ++i)
                for (std::size_t k = 0; k < g.zoneIds.size(); ++k)
                    out << csvQuoted(s.id) << ',' << csvQuoted(a.id) << ',' << csvQuoted(g.zoneIds[i]) << ',' << csvQuoted(g.zoneIds[k]) << ','
                        << csvNumber(a.persons[i][k]) << ',' << csvNumber(a.vehicles ? std::optional<double>((*a.vehicles)[i][k]) : std::nullopt) << '\n';
            out << csvQuoted(s.id) << ',' << csvQuoted(a.id) << ",totals,," << csvNumber(total(a.persons)) << ','
                << csvNumber(a.vehicles ? std::optional<double>(total(*a.vehicles)) : std::nullopt) << '\n';
        }
    return out.str();
}
}
