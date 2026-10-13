#include "od_expansion.hpp"
#include "demand_catalog.hpp"
#include "run.hpp"
#include <algorithm>

namespace trafficsim {
namespace {
// One connector pair's chains (§3): the fewest-object chains some lane can drive, which chain
// each lane of the origin Link carries, and the chains in use, in the search's order.
struct OdPair {
    std::vector<std::vector<std::string>> chains;
    bool stoppedAtLimit{};
    std::vector<std::optional<std::size_t>> assign; // per lane of the origin Link
    std::vector<std::size_t> used;
};
OdPair resolvePair(const Network& network, const ConnectorPathTable& table, const std::string& from, const std::string& to) {
    OdPair pair;
    if (from == to) pair.chains = {{from}};
    else {
        auto found = routeShortestChainsBounded(network, table, from, to);
        pair.stoppedAtLimit = found.stoppedAtLimit;
        // A level whose chains are all undrivable is the answer: no fallback to a longer one. An
        // undrivable chain has no lane family, so no lane is assigned to it below.
        pair.chains = std::move(found.chains);
    }
    std::size_t lanes = 0;
    for (const auto& link : network.links) if (link.id == from) lanes = link.lanes.size();
    pair.assign.assign(lanes, std::nullopt);
    std::vector<std::vector<FamilyChain>> families;
    for (const auto& chain : pair.chains) families.push_back(routeLaneFamily(network, table, chain));
    // A lane takes the first chain it drives in full, else the first it reaches as a kept stub.
    for (const bool stub : {false, true})
        for (std::size_t lane = 0; lane < lanes; ++lane) {
            if (pair.assign[lane]) continue;
            for (std::size_t j = 0; j < families.size() && !pair.assign[lane]; ++j)
                for (const auto& member : families[j])
                    if (member.lane == lane && member.stub == stub) { pair.assign[lane] = j; break; }
        }
    for (const auto& j : pair.assign) if (j) pair.used.push_back(*j);
    std::sort(pair.used.begin(), pair.used.end());
    pair.used.erase(std::unique(pair.used.begin(), pair.used.end()), pair.used.end());
    return pair;
}
std::set<std::string> idsOf(const auto& items) {
    std::set<std::string> ids;
    for (const auto& item : items) ids.insert(item.id);
    return ids;
}
// The generated object a built id comes from: the longest generated id that is the whole id or
// a prefix of it ending at '/' (compositions add /type-, intervals and lanes their own parts).
const OdCell* originOf(const std::map<std::string, OdCell>& origins, const std::string& id) {
    const OdCell* best = nullptr; std::size_t length = 0;
    for (const auto& [key, cell] : origins)
        if (key.size() > length && id.starts_with(key) && (id.size() == key.size() || id[key.size()] == '/')) {
            best = &cell; length = key.size();
        }
    return best;
}
std::size_t indexIn(const std::string& path, const std::string& name) {
    return std::stoul(path.substr(name.size() + 1));
}
// Usable: its base types and every interval's are non-empty, distinct, with finite shares above
// 0 (D151), and, when the types are known, all of them known. The catalog's own check, on a copy
// holding this one composition.
bool usableComposition(const AuthoringDefinition& d, const Composition& c, const std::vector<VehicleType>* types) {
    if (types && !validComposition(c, *types)) return false;
    auto single = d;
    single.compositions = {c}; single.externalCompositions = false; single.inputs.clear();
    single.vehicleTypeNames.clear(); single.externalVehicleTypes = !types;
    if (types) single.vehicleTypes = *types;
    return ownedCatalogIssues(single).empty();
}
}
std::optional<GeneratedRef> generatedRef(const std::string& path) {
    // "routes[i]..." and "inputs[i]...", or a conflict zone's "conflictZones[z] routes[i]".
    for (const auto* name : {"routes", "inputs"}) {
        const auto key = std::string(name) + "[";
        std::size_t at = std::string::npos;
        if (path.starts_with(key)) at = 0;
        else if (const auto inner = path.find(" " + key); inner != std::string::npos) at = inner + 1;
        if (at == std::string::npos) continue;
        try { return GeneratedRef{name[0] == 'r', at, std::stoul(path.substr(at + key.size()))}; }
        catch (const std::exception&) { return std::nullopt; }
    }
    return std::nullopt;
}
OdClasses ownedOdClasses(const AuthoringDefinition& d) {
    OdClasses classes;
    if (!d.externalVehicleTypes) classes.vehicleTypes = idsOf(d.vehicleTypes);
    if (!d.externalCompositions) {
        classes.compositions.emplace();
        for (const auto& c : d.compositions)
            if (usableComposition(d, c, d.externalVehicleTypes ? nullptr : &d.vehicleTypes)) classes.compositions->insert(c.id);
    }
    return classes;
}
OdClasses resolvedOdClasses(const AuthoringDefinition& d, const std::filesystem::path& data, InputManifest* manifest) {
    if (d.odMatrices.empty()) return {};
    const bool compositions = std::any_of(d.odMatrices.begin(), d.odMatrices.end(), [](const auto& m) { return !m.compositionId.empty(); });
    const auto catalog = resolveDemandCatalog(d, data, compositions, manifest);
    OdClasses classes;
    classes.vehicleTypes = idsOf(catalog.vehicleTypes);
    classes.compositions.emplace();
    for (const auto& c : catalog.compositions)
        if (usableComposition(d, c, &catalog.vehicleTypes)) classes.compositions->insert(c.id);
    return classes;
}
OdExpansion expandOd(const Network& network, const AuthoringDefinition& authored, const OdClasses& classes) {
    OdExpansion out{authored, {}, {}, {}};
    if (authored.odMatrices.empty()) return out;
    auto& def = out.definition;
    def.odMatrices.clear(); // expanded once; a second call has nothing left to add
    std::set<std::pair<std::string, std::string>> reported;
    const auto add = [&](const char* code, const std::string& path) {
        if (reported.insert({code, path}).second) out.issues.push_back({code, path});
    };
    // An engine source starts on a segment with no predecessor (UNSUPPORTED_INTERNAL_INPUT), so
    // every origin connector on a Link a Connector enters is refused, carrying flow or not (D151).
    std::set<std::string> internal;
    for (std::size_t i = 0; i < network.zoneConnectors.size(); ++i) {
        const auto& c = network.zoneConnectors[i];
        if (c.direction != "origin") continue;
        if (std::any_of(network.connectors.begin(), network.connectors.end(), [&](const auto& x) { return x.to.linkId == c.linkId; })) {
            add("OD_ORIGIN_LINK_INTERNAL", "zoneConnectors[" + std::to_string(i) + "]");
            internal.insert(c.id);
        }
    }
    const auto connectorsOf = [&](const std::string& zone, const char* direction) {
        std::vector<const ZoneConnector*> result;
        for (const auto& c : network.zoneConnectors) if (c.zoneId == zone && c.direction == direction) result.push_back(&c);
        return result;
    };
    const auto shareSum = [](const std::vector<const ZoneConnector*>& all) {
        double sum = 0; for (const auto* c : all) sum += c->share; return sum;
    };
    const ConnectorPathTable table(network);
    std::map<std::pair<std::string, std::string>, OdPair> memo;
    const auto pairOf = [&](const ZoneConnector& c, const ZoneConnector& k) -> const OdPair& {
        const auto key = std::make_pair(c.linkId, k.linkId);
        auto it = memo.find(key);
        if (it == memo.end()) it = memo.emplace(key, resolvePair(network, table, c.linkId, k.linkId)).first;
        return it->second;
    };
    for (std::size_t m = 0; m < authored.odMatrices.size(); ++m) {
        const auto& matrix = authored.odMatrices[m];
        const auto path = "odMatrices[" + std::to_string(m) + "]";
        const bool type = !matrix.vehicleTypeId.empty();
        const auto& known = type ? classes.vehicleTypes : classes.compositions;
        if (known && !known->contains(type ? matrix.vehicleTypeId : matrix.compositionId)) {
            add("OD_UNKNOWN_CLASS", path + (type ? ".vehicleTypeId" : ".compositionId"));
            continue;
        }
        const double span = matrix.endTime - matrix.startTime;
        for (std::size_t o = 0; o < matrix.zones.size(); ++o)
            for (std::size_t d = 0; d < matrix.zones.size(); ++d) {
                const double trips = matrix.trips[o][d];
                if (!(trips > 0)) continue;
                const auto cell = path + ".trips[" + std::to_string(o) + "][" + std::to_string(d) + "]";
                if (o == d) { add("OD_INTRAZONAL_TRIPS", cell); continue; }
                const auto origins = connectorsOf(matrix.zones[o], "origin");
                const auto destinations = connectorsOf(matrix.zones[d], "destination");
                if (origins.empty()) add("OD_ZONE_NO_CONNECTOR", path + ".zones[" + std::to_string(o) + "]");
                if (destinations.empty()) add("OD_ZONE_NO_CONNECTOR", path + ".zones[" + std::to_string(d) + "]");
                if (origins.empty() || destinations.empty()) continue;
                const double rate = trips * 3600 / span, originSum = shareSum(origins), destinationSum = shareSum(destinations);
                const auto vph = [&](const ZoneConnector* c, const ZoneConnector* k) {
                    return rate * c->share / originSum * k->share / destinationSum;
                };
                // Any failing pair suppresses the whole cell (D151): it is never half generated.
                bool blocked = false;
                for (const auto* c : origins) for (const auto* k : destinations) {
                    if (!(vph(c, k) > 0)) continue;
                    if (internal.contains(c->id)) { blocked = true; continue; }
                    const auto& pair = pairOf(*c, *k);
                    if (!pair.used.empty()) continue;
                    add(pair.stoppedAtLimit ? "OD_PATH_SEARCH_LIMIT" : "OD_PAIR_UNREACHABLE", cell);
                    blocked = true;
                }
                if (blocked) continue;
                const OdCell origin{m, o, d};
                for (const auto* c : origins) for (const auto* k : destinations) {
                    const double flow = vph(c, k);
                    if (!(flow > 0)) continue;
                    const auto& pair = pairOf(*c, *k);
                    std::size_t carrying = 0;
                    for (const auto& j : pair.assign) carrying += j.has_value();
                    for (std::size_t u = 0; u < pair.used.size(); ++u) {
                        const auto j = pair.used[u];
                        const auto suffix = c->id + ">" + k->id + "/path-" + std::to_string(u);
                        const auto routeId = "od:route/" + suffix;
                        if (out.routeOrigin.emplace(routeId, origin).second) def.routes.push_back({routeId, pair.chains[j]});
                        VehicleInput input;
                        input.id = "od:" + matrix.id + "/" + matrix.zones[o] + ">" + matrix.zones[d] + "/" + suffix;
                        input.routeId = routeId;
                        input.vehicleTypeId = matrix.vehicleTypeId;
                        input.compositionId = matrix.compositionId;
                        input.startTime = matrix.startTime; input.endTime = matrix.endTime;
                        std::size_t lanes = 0;
                        for (const auto& a : pair.assign) {
                            const bool mine = a && *a == j;
                            lanes += mine; input.laneShares.push_back(mine ? 1 : 0);
                        }
                        input.vehiclesPerHour = flow * static_cast<double>(lanes) / static_cast<double>(carrying);
                        // No linkId and no routingDecisionId: no routing decision acts on an OD trip.
                        out.inputOrigin.emplace(input.id, origin);
                        def.inputs.push_back(std::move(input));
                    }
                }
            }
    }
    return out;
}
AuthoringDefinition withOdDemand(const Network& n, const AuthoringDefinition& d, const OdClasses& c) { return expandOd(n, d, c).definition; }
std::vector<ValidationIssue> odRunIssues(const Network& n, const AuthoringDefinition& d, const OdClasses& c) { return expandOd(n, d, c).issues; }
void addOdRunRows(const Network& network, const AuthoringDefinition& authored, const OdExpansion& od, std::vector<Diagnostic>& rows) {
    for (const auto& issue : od.issues) {
        std::string object;
        if (issue.path.starts_with("zoneConnectors[")) object = objectIdForPath(network, issue.path);
        else if (issue.path.starts_with("odMatrices[")) object = authored.odMatrices.at(indexIn(issue.path, "odMatrices")).id;
        rows.push_back({issue.code, issue.path, std::move(object), {}, DiagnosticSeverity::runtime});
    }
}
void remapOdRows(const Network& network, const ScenarioDefinition& checked, const AuthoringDefinition& authored,
                 const OdExpansion& od, std::vector<Diagnostic>& rows) {
    if (od.routeOrigin.empty() && od.inputOrigin.empty()) return;
    // Route checks run on `checked` itself; every other row indexes the scenario it builds, whose
    // routes and inputs are split per lane and period. The code says which array a path indexes.
    std::optional<Scenario> built;
    try { built = buildScenario(network, checked); } catch (const std::exception&) {}
    std::vector<Diagnostic> result;
    std::set<std::tuple<std::string, std::string, std::string>> seen;
    for (auto& row : rows) {
        std::string id;
        const auto ref = generatedRef(row.path);
        if (ref) {
            const bool ownArray = row.code == "UNSUPPORTED_ROUTE_TOPOLOGY" || row.code == "AMBIGUOUS_ROUTE_STEP";
            if (ownArray) { if (ref->route && ref->index < checked.routes.size()) id = checked.routes[ref->index].id; }
            else if (built) {
                if (ref->route && ref->index < built->routes.size()) id = built->routes[ref->index].id;
                if (!ref->route && ref->index < built->inputs.size()) id = built->inputs[ref->index].id;
            }
        }
        const OdCell* cell = id.starts_with("od:") ? originOf(ref->route ? od.routeOrigin : od.inputOrigin, id) : nullptr;
        if (cell) {
            // Whatever names the place (a conflict zone) stays; the generated object becomes its cell.
            row.path = row.path.substr(0, ref->at) + "odMatrices[" + std::to_string(cell->matrix) + "].trips[" +
                       std::to_string(cell->origin) + "][" + std::to_string(cell->destination) + "]";
            row.objectId = authored.odMatrices.at(cell->matrix).id;
            row.selectId.clear();
            // Several generated pieces of one cell would say the same thing; once is enough.
            if (!seen.insert({row.code, row.path, row.objectId}).second) continue;
        } else if (row.objectId.starts_with("od:")) {
            row.objectId.clear(); // an authored row named through a generated index: never name od:
        }
        result.push_back(std::move(row));
    }
    rows = std::move(result);
}
}
