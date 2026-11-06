#include "trip_generation_run.hpp"
#include "input_manifest.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <map>
#include <numeric>

namespace multra {
namespace {
TripSide resolveSide(const TripRateSide& s, const std::map<std::string, std::size_t>& index) {
    TripSide side{s.method, s.constant, {}, {}};
    for (const auto& c : s.categories) side.categories.push_back({index.at(c.attribute), c.rate});
    for (const auto& t : s.terms) side.terms.push_back({index.at(t.attribute), t.coefficient});
    return side;
}
void collect(const TripRateSide& s, std::vector<std::string>& names) {
    const auto add = [&](const std::string& n) { if (std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n); };
    for (const auto& c : s.categories) add(c.attribute);
    for (const auto& t : s.terms) add(t.attribute);
}
}
TripGenerationRun runTripGeneration(const ProjectDocument& d, const std::filesystem::path& data, const std::string& id,
                                    InputManifest* manifest, const std::function<void(TripGenerationStage)>& stage) {
    const auto enter = [&](TripGenerationStage s) { if (stage) stage(s); };
    enter(TripGenerationStage::rateSet);
    const auto sets = resolveTripRateSets(data, manifest);
    const auto found = std::find_if(sets.begin(), sets.end(), [&](const auto& s) { return s.set.id == id; });
    if (found == sets.end()) throw ValidationError({{"TRIP_RATE_SET_UNKNOWN", "tripRateSets"}});
    TripGenerationRun run;
    run.revision = d.revision; run.set = found->set; run.setPath = found->path;

    enter(TripGenerationStage::zones);
    const auto& zones = d.zones.zones;
    if (zones.empty()) throw ValidationError({{"TRIP_GENERATION_NO_ZONES", "zones"}});
    std::vector<std::size_t> order(zones.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return zones[a].id < zones[b].id; });
    std::vector<std::string> names;
    for (const auto& p : run.set.purposes) { collect(p.productions, names); collect(p.attractions, names); }
    std::map<std::string, std::size_t> index;
    for (std::size_t a = 0; a < names.size(); ++a) index[names[a]] = a;
    std::vector<ValidationIssue> missing;
    std::vector<std::vector<double>> values;
    for (const auto z : order) {
        std::vector<double> row;
        for (const auto& name : names) {
            const auto v = zones[z].attributes.find(name);
            if (v == zones[z].attributes.end()) missing.push_back({"TRIP_RATE_ATTRIBUTE_MISSING", "zones[" + std::to_string(z) + "].attributes." + name});
            row.push_back(v == zones[z].attributes.end() ? 0.0 : v->second);
        }
        values.push_back(std::move(row));
        run.zoneIds.push_back(zones[z].id);
    }
    if (!missing.empty()) {
        std::sort(missing.begin(), missing.end(), [](const auto& a, const auto& b) { return a.path < b.path; });
        throw ValidationError(std::move(missing));
    }

    enter(TripGenerationStage::generation);
    std::vector<TripPurpose> purposes;
    for (const auto& p : run.set.purposes) purposes.push_back({p.id, resolveSide(p.productions, index), resolveSide(p.attractions, index)});
    auto result = generateTrips(values, purposes);
    std::vector<ValidationIssue> negative, unbalanced;
    for (const auto& i : result.issues) {
        if (i.code == TripIssue::Code::negative)
            negative.push_back({"TRIP_GENERATION_NEGATIVE", "zones[" + std::to_string(order[*i.zone]) + "]." + purposes[i.purpose].id +
                                                            (i.attractions ? ".attractions" : ".productions")});
        else unbalanced.push_back({"TRIP_BALANCE_IMPOSSIBLE", run.setPath + ".purposes[" + std::to_string(i.purpose) + "]"});
    }
    if (!negative.empty()) throw ValidationError(std::move(negative));
    enter(TripGenerationStage::balancing);
    if (!unbalanced.empty()) throw ValidationError(std::move(unbalanced));
    run.purposes = std::move(result.purposes);
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    return run;
}
}
