#include "zones_od.hpp"
#include "../model/network/zones.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace trafficsim {
namespace {
const Json& item(const Json& value, const char* name) {
    if (!value.is_object() || !value.contains(name) || value.at(name).is_null())
        throw std::invalid_argument(std::string("Missing field: ") + name);
    return value.at(name);
}
std::string text(const Json& value, const char* name) {
    const auto& v = item(value, name);
    if (!v.is_string()) throw std::invalid_argument(std::string("Expected text: ") + name);
    return v.get<std::string>();
}
double number(const Json& value, const char* name) {
    const auto& v = item(value, name);
    if (!v.is_number()) throw std::invalid_argument(std::string("Expected number: ") + name);
    return v.get<double>();
}
const Json& list(const Json& value, const char* name) {
    const auto& v = item(value, name);
    if (!v.is_array()) throw std::invalid_argument(std::string("Expected array: ") + name);
    return v;
}
std::string optionalText(const Json& value, const char* name) { return present(value, name) ? text(value, name) : std::string{}; }
bool blank(const std::string& s) { return s.find_first_not_of(" \t\n\r") == std::string::npos; }
bool positive(const OdMatrix& m) {
    return std::any_of(m.trips.begin(), m.trips.end(), [](const auto& row) {
        return std::any_of(row.begin(), row.end(), [](double v) { return v > 0; }); });
}
}
void rejectZonesAndOdBefore29(const Json& j) {
    if (!j.is_object()) return;
    for (const char* key : {"zones", "zoneConnectors"})
        if (j.contains("network") && j.at("network").is_object() && j.at("network").contains(key))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", std::string("network.") + key}});
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("odMatrices"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.odMatrices"}});
}
void parseZones(const Json& value, Network& network) {
    if (present(value, "zones")) for (const auto& z : list(value, "zones")) {
        const auto path = "zones[" + std::to_string(network.zones.size()) + "]";
        requireKnownFields(z, {"id", "name", "polygon"}, path);
        Zone zone{text(z, "id"), optionalText(z, "name"), {}};
        for (const auto& p : list(z, "polygon")) {
            requireKnownFields(p, {"x", "y"}, path + ".polygon");
            zone.polygon.push_back({number(p, "x"), number(p, "y")});
        }
        network.zones.push_back(std::move(zone));
    }
    if (present(value, "zoneConnectors")) for (const auto& c : list(value, "zoneConnectors")) {
        const auto path = "zoneConnectors[" + std::to_string(network.zoneConnectors.size()) + "]";
        requireKnownFields(c, {"id", "zoneId", "linkId", "direction", "share"}, path);
        network.zoneConnectors.push_back({text(c, "id"), text(c, "zoneId"), text(c, "linkId"), text(c, "direction"),
                                          present(c, "share") ? number(c, "share") : 1.0});
    }
}
std::vector<OdMatrix> parseOdMatrices(const Json& definition) {
    std::vector<OdMatrix> result;
    if (!present(definition, "odMatrices")) return result;
    for (const auto& m : list(definition, "odMatrices")) {
        const auto path = "definition.odMatrices[" + std::to_string(result.size()) + "]";
        // The definition has no key check of its own, so the matrix keeps its own whitelist.
        requireKnownFields(m, {"id", "name", "startTime", "endTime", "vehicleTypeId", "compositionId", "zones", "trips"}, path);
        OdMatrix matrix{text(m, "id"), optionalText(m, "name"), number(m, "startTime"), number(m, "endTime"),
                        optionalText(m, "vehicleTypeId"), optionalText(m, "compositionId"), {}, {}};
        for (const auto& z : list(m, "zones")) {
            if (!z.is_string()) throw std::invalid_argument("Expected strings: zones");
            matrix.zones.push_back(z.get<std::string>());
        }
        for (const auto& row : list(m, "trips")) {
            if (!row.is_array()) throw std::invalid_argument("Expected array: trips");
            auto& cells = matrix.trips.emplace_back();
            for (const auto& cell : row) {
                if (!cell.is_number()) throw std::invalid_argument("Expected number: trips");
                cells.push_back(cell.get<double>());
            }
        }
        result.push_back(std::move(matrix));
    }
    return result;
}
void addZonesJson(const Network& n, Json& network) {
    if (!n.zones.empty()) {
        Json zones = Json::array();
        for (const auto& z : n.zones) {
            Json polygon = Json::array();
            for (const auto& p : z.polygon) polygon.push_back({{"x", p.x}, {"y", p.y}});
            zones.push_back({{"id", z.id}, {"name", z.name}, {"polygon", polygon}});
        }
        network["zones"] = zones;
    }
    if (!n.zoneConnectors.empty()) {
        Json connectors = Json::array();
        for (const auto& c : n.zoneConnectors) {
            connectors.push_back({{"id", c.id}, {"zoneId", c.zoneId}, {"linkId", c.linkId}, {"direction", c.direction}});
            if (c.share != 1) connectors.back()["share"] = c.share;
        }
        network["zoneConnectors"] = connectors;
    }
}
void addOdMatrixJson(const AuthoringDefinition& d, Json& definition) {
    if (d.odMatrices.empty()) return;
    Json matrices = Json::array();
    for (const auto& m : d.odMatrices) {
        Json matrix = {{"id", m.id}, {"name", m.name}, {"startTime", m.startTime}, {"endTime", m.endTime},
                       {"zones", m.zones}, {"trips", m.trips}};
        if (!m.vehicleTypeId.empty()) matrix["vehicleTypeId"] = m.vehicleTypeId;
        if (!m.compositionId.empty()) matrix["compositionId"] = m.compositionId;
        matrices.push_back(std::move(matrix));
    }
    definition["odMatrices"] = matrices;
}
bool usesZonesOrOd(const ProjectDocument& d) {
    return !d.network.zones.empty() || !d.network.zoneConnectors.empty() || (d.definition && !d.definition->odMatrices.empty());
}
bool hasOdDemand(const AuthoringDefinition& d) {
    return std::any_of(d.odMatrices.begin(), d.odMatrices.end(), positive);
}
std::vector<ValidationIssue> odMatrixIssues(const ProjectDocument& doc) {
    std::vector<ValidationIssue> issues;
    if (!doc.definition || !usesZonesOrOd(doc)) return issues;
    const auto& d = *doc.definition;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    // §3 reserves the prefix for generated ids. The rule reads the content: a document without
    // zones, connectors or matrices may keep an authored `od:` id.
    for (std::size_t i = 0; i < d.routes.size(); ++i)
        if (d.routes[i].id.starts_with("od:")) add("INVALID_ID", "routes[" + std::to_string(i) + "].id");
    for (std::size_t i = 0; i < d.inputs.size(); ++i)
        if (d.inputs[i].id.starts_with("od:")) add("INVALID_ID", "inputs[" + std::to_string(i) + "].id");
    std::set<std::string> zones, ids;
    for (const auto& z : doc.network.zones) zones.insert(z.id);
    for (std::size_t i = 0; i < d.odMatrices.size(); ++i) {
        const auto& m = d.odMatrices[i];
        const auto path = "odMatrices[" + std::to_string(i) + "]";
        if (zoneIdInvalid(m.id)) add("INVALID_ID", path + ".id");
        else if (!ids.insert(m.id).second) add("DUPLICATE_ID", path + ".id");
        const bool type = !m.vehicleTypeId.empty(), composition = !m.compositionId.empty();
        if (type == composition || (type && blank(m.vehicleTypeId)) || (composition && blank(m.compositionId)))
            add("OD_DEMAND_CLASS", path);
        const std::set<std::string> distinct(m.zones.begin(), m.zones.end());
        bool shaped = !m.zones.empty() && distinct.size() == m.zones.size() && m.trips.size() == m.zones.size();
        for (const auto& row : m.trips) shaped = shaped && row.size() == m.zones.size();
        if (!shaped) add("OD_MATRIX_SHAPE", path);
        for (std::size_t z = 0; z < m.zones.size(); ++z)
            if (!zones.contains(m.zones[z])) add("UNKNOWN_ZONE", path + ".zones[" + std::to_string(z) + "]");
        for (std::size_t o = 0; o < m.trips.size(); ++o)
            for (std::size_t t = 0; t < m.trips[o].size(); ++t)
                if (!std::isfinite(m.trips[o][t]) || m.trips[o][t] < 0)
                    add("OD_CELL_INVALID", path + ".trips[" + std::to_string(o) + "][" + std::to_string(t) + "]");
        // The same bounds an input's interval has, so shortening the duration or changing the
        // time step under a matrix is refused as it is under an input.
        if (!std::isfinite(m.startTime) || !std::isfinite(m.endTime) || m.startTime < 0 || m.startTime >= m.endTime ||
            m.endTime > d.duration || !onTimeGrid(m.startTime, d.timeStep) || !onTimeGrid(m.endTime, d.timeStep))
            add("OD_PERIOD_INVALID", path);
        // Two matrices of one class at once would add silently.
        for (std::size_t j = 0; j < i; ++j) {
            const auto& o = d.odMatrices[j];
            const bool same = (type && o.vehicleTypeId == m.vehicleTypeId) || (composition && o.compositionId == m.compositionId);
            if (same && m.startTime < o.endTime && o.startTime < m.endTime) { add("OD_PERIOD_OVERLAP", path); break; }
        }
    }
    return issues;
}
bool definitionNamesVehicleType(const AuthoringDefinition& d, const std::string& id) {
    return std::any_of(d.inputs.begin(), d.inputs.end(), [&](const auto& i) { return i.vehicleTypeId == id; }) ||
           std::any_of(d.routingDecisions.begin(), d.routingDecisions.end(), [&](const auto& x) {
               return std::any_of(x.typeRules.begin(), x.typeRules.end(), [&](const auto& r) { return r.vehicleTypeId == id; }); }) ||
           std::any_of(d.odMatrices.begin(), d.odMatrices.end(), [&](const auto& m) { return m.vehicleTypeId == id; });
}
bool definitionNamesComposition(const AuthoringDefinition& d, const std::string& id) {
    return std::any_of(d.inputs.begin(), d.inputs.end(), [&](const auto& i) { return i.compositionId == id; }) ||
           std::any_of(d.odMatrices.begin(), d.odMatrices.end(), [&](const auto& m) { return m.compositionId == id; });
}
}
