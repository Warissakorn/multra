#include "trip_distribution_output.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <sstream>

namespace multra {
namespace {
constexpr const char* kMeasure = "Trip distribution (doubly-constrained gravity model, T_ij = a_i b_j exp(-beta c_ij), balanced by "
    "row-first Furness passes): production-attraction trips in the rate set's unit; costs are free-flow shortest-path minutes "
    "(Ortuzar and Willumsen 2011, Eq. 5.7 and Example 5.4)";
Json number(double x) { return std::isfinite(x) ? Json(x) : Json(nullptr); }
std::optional<double> finite(double x) { return std::isfinite(x) ? std::optional<double>(x) : std::nullopt; }
}
Json tripDistributionJson(const TripDistributionRun& r) {
    const auto& g = r.generation;
    Json source = Json::object();
    if (!r.deterrence.citation.empty()) source["citation"] = r.deterrence.citation;
    if (!r.deterrence.table.empty()) source["table"] = r.deterrence.table;
    Json j{{"validated", false}, {"measure", kMeasure}, {"revision", g.revision},
           {"rateSet", {{"id", g.set.id}, {"unit", g.set.unit}}},
           {"deterrenceSet", {{"id", r.deterrence.id}, {"name", r.deterrence.name}, {"source", source}, {"function", r.deterrence.function},
                              {"costUnit", r.deterrence.costUnit}}},
           {"skim", {{"kind", "freeFlow"}, {"mode", r.mode}, {"unit", "min"},
                      {"costMatrix", r.costMatrix.empty() ? Json(nullptr) : Json(r.costMatrix)}}},
           {"zoneIds", g.zoneIds}, {"costs", Json::array()}, {"purposes", Json::array()}, {"inputManifest", g.inputManifest}};
    for (const auto& row : r.costs) {
        Json cells = Json::array();
        for (const double c : row) cells.push_back(number(c));
        j["costs"].push_back(std::move(cells));
    }
    for (const auto& p : r.purposes) {
        const auto& d = p.result;
        j["purposes"].push_back({{"id", p.id}, {"beta", p.beta}, {"intrazonalShare", p.intrazonalShare},
            {"passes", d.passes}, {"error", d.error}, {"converged", d.converged}, {"meanCost", p.meanCost},
            {"bands", {{"width", r.bandWidth}, {"trips", p.bands}}},
            {"a", d.a}, {"b", d.b}, {"intrazonal", d.intrazonal}, {"trips", d.trips}});
    }
    return j;
}
std::string tripDistributionCsv(const TripDistributionRun& r) {
    const auto& g = r.generation;
    std::ostringstream out;
    out << "# MULTRA - not yet validated. " << kMeasure << ".\n";
    out << "# Rate set: " << Json(g.set.id).dump() << "; deterrence set: " << Json(r.deterrence.id).dump() << "; unit: "
        << Json(g.set.unit).dump() << "; skim mode: " << Json(r.mode).dump()
        << "; cost matrix: " << (r.costMatrix.empty() ? std::string("none") : Json(r.costMatrix).dump()) << ".\n";
    for (const auto& p : r.purposes)
        out << "# " << Json(p.id).dump() << ": beta " << p.beta << " per min; passes " << p.result.passes << "; error " << p.result.error
            << (p.result.converged ? "" : "; DISTRIBUTION_NOT_CONVERGED") << ".\n";
    out << "purpose,originId,destinationId,trips,cost\n";
    for (const auto& p : r.purposes) {
        double total = 0;
        for (std::size_t i = 0; i < g.zoneIds.size(); ++i)
            for (std::size_t k = 0; k < g.zoneIds.size(); ++k) {
                total += p.result.trips[i][k];
                out << csvQuoted(p.id) << ',' << csvQuoted(g.zoneIds[i]) << ',' << csvQuoted(g.zoneIds[k]) << ','
                    << csvNumber(p.result.trips[i][k]) << ',' << csvNumber(finite(r.costs[i][k])) << '\n';
            }
        out << csvQuoted(p.id) << ",totals,," << csvNumber(total) << ',' << csvNumber(p.meanCost) << '\n';
    }
    return out.str();
}
}
