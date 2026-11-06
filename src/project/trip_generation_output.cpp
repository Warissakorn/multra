#include "trip_generation_output.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace multra {
namespace {
constexpr const char* kMeasure = "Trip generation (cross-classification or zonal regression per purpose side): trip ends in the "
    "rate set's unit; attractions scaled to the productions' total (Ortuzar and Willumsen 2011, Eq. 4.14)";
}
Json tripGenerationJson(const TripGenerationRun& r) {
    Json source{{"citation", r.set.citation}};
    if (!r.set.table.empty()) source["table"] = r.set.table;
    Json j{{"validated", false}, {"measure", kMeasure}, {"revision", r.revision},
           {"rateSet", {{"id", r.set.id}, {"name", r.set.name}, {"source", source}, {"unit", r.set.unit}}},
           {"purposes", Json::array()}, {"inputManifest", r.inputManifest}};
    for (const auto& p : r.purposes) {
        Json zones = Json::array();
        for (std::size_t z = 0; z < r.zoneIds.size(); ++z)
            zones.push_back({{"zoneId", r.zoneIds[z]}, {"productions", p.productions[z]}, {"attractionsRaw", p.attractionsRaw[z]},
                             {"attractions", p.attractions[z]}});
        j["purposes"].push_back({{"id", p.id}, {"factor", p.factor ? Json(*p.factor) : Json(nullptr)},
            {"totals", {{"productions", p.totalProductions}, {"attractionsRaw", p.totalAttractionsRaw}, {"attractions", p.totalAttractions}}},
            {"zones", zones}});
    }
    return j;
}
std::string tripGenerationCsv(const TripGenerationRun& r) {
    std::ostringstream out;
    out << "# MULTRA - not yet validated. " << kMeasure << ".\n";
    out << "# Rate set: " << Json(r.set.id).dump() << "; unit: " << Json(r.set.unit).dump() << "; source: " << Json(r.set.citation).dump() << ".\n";
    out << "purpose,zoneId,productions,attractionsRaw,attractions,factor\n";
    for (const auto& p : r.purposes) {
        for (std::size_t z = 0; z < r.zoneIds.size(); ++z)
            out << csvQuoted(p.id) << ',' << csvQuoted(r.zoneIds[z]) << ',' << csvNumber(p.productions[z]) << ',' << csvNumber(p.attractionsRaw[z])
                << ',' << csvNumber(p.attractions[z]) << ",\n";
        out << csvQuoted(p.id) << ",totals," << csvNumber(p.totalProductions) << ',' << csvNumber(p.totalAttractionsRaw) << ','
            << csvNumber(p.totalAttractions) << ',' << csvNumber(p.factor) << '\n';
    }
    return out.str();
}
}
