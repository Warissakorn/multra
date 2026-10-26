#include "zone_codec.hpp"
#include "../core/validate.hpp"
#include "document.hpp"
#include "study_period_codec.hpp"
#include "../model/network/polygon.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
constexpr const char* kZones = "zones";
constexpr const char* kConnectors = "zoneConnectors";
constexpr const char* kMatrices = "odMatrices";
constexpr const char* kLots = "parkingLots";
[[noreturn]] void invalid() { throw std::invalid_argument("EDIT_ZONE_INVALID"); }
void exactKeys(const Json& j, const std::vector<const char*>& keys) {
    if (!j.is_object() || j.size() != keys.size()) invalid();
    for (const auto* key : keys) if (!j.contains(key)) invalid();
}
std::string text(const Json& j, const char* key) { if (!j.at(key).is_string()) invalid(); return j.at(key).get<std::string>(); }
double number(const Json& j) { if (!j.is_number()) invalid(); return j.get<double>(); }
const Json& array(const Json& j) { if (!j.is_array()) invalid(); return j; }
const char* directionName(ZoneDirection d) { return d == ZoneDirection::origin ? "origin" : "destination"; }

Zone parseZone(const Json& j) {
    exactKeys(j, {"id", "name", "polygon", "attributes"});
    Zone z{text(j, "id"), text(j, "name"), {}, {}};
    for (const auto& p : array(j.at("polygon"))) {
        if (!p.is_array() || p.size() != 2) invalid();
        z.polygon.push_back({number(p[0]), number(p[1])});
    }
    if (!j.at("attributes").is_object()) invalid();
    for (const auto& [name, value] : j.at("attributes").items()) z.attributes[name] = number(value);
    return z;
}
ZoneConnector parseConnector(const Json& j) {
    exactKeys(j, {"id", "zoneId", "linkId", "direction", "share"});
    const auto direction = text(j, "direction");
    if (direction != "origin" && direction != "destination") invalid();
    return {text(j, "id"), text(j, "zoneId"), text(j, "linkId"),
            direction == "origin" ? ZoneDirection::origin : ZoneDirection::destination, number(j.at("share"))};
}
OdMatrix parseMatrix(const Json& j, const std::string& path) {
    // S1.1 (STUDY_PERIODS §5): its own times or a period, never both and never neither.
    if (j.is_object() && j.contains("periodId") == (j.contains("startTime") || j.contains("endTime")))
        throw ValidationError({{"OD_PERIOD_AMBIGUOUS", path}});
    // S1.2 (§5, §6): a composition or a mode, never both and never neither.
    if (j.is_object() && j.contains("modeId") == j.contains("compositionId")) throw ValidationError({{"OD_TYPES_AMBIGUOUS", path}});
    const bool period = j.contains("periodId"), mode = j.contains("modeId");
    std::vector<const char*> keys{"id", "zoneIds", "cells", mode ? "modeId" : "compositionId"};
    if (period) keys.push_back("periodId"); else { keys.push_back("startTime"); keys.push_back("endTime"); }
    exactKeys(j, keys);
    OdMatrix m;
    m.id = text(j, "id");
    if (period) m.periodId = text(j, "periodId"); else { m.startTime = number(j.at("startTime")); m.endTime = number(j.at("endTime")); }
    if (mode) m.modeId = text(j, "modeId"); else m.compositionId = text(j, "compositionId");
    for (const auto& id : array(j.at("zoneIds"))) { if (!id.is_string()) invalid(); m.zoneIds.push_back(id.get<std::string>()); }
    for (const auto& row : array(j.at("cells"))) {
        m.cells.emplace_back();
        for (const auto& cell : array(row)) m.cells.back().push_back(number(cell));
    }
    return m;
}
std::uint32_t count(const Json& j) {
    if (!j.is_number_unsigned() || j.get<std::uint64_t>() > 0xffffffffULL) invalid();
    return static_cast<std::uint32_t>(j.get<std::uint64_t>());
}
ParkingLot parseLot(const Json& j) {
    exactKeys(j, {"id", "zoneId", "capacity", "initialOccupancy", "initialCompositionId", "gateServiceTime", "dwell"});
    ParkingLot lot{text(j, "id"), text(j, "zoneId"), count(j.at("capacity")), count(j.at("initialOccupancy")),
                   text(j, "initialCompositionId"), number(j.at("gateServiceTime")), {}};
    for (const auto& p : array(j.at("dwell"))) {
        if (!p.is_array() || p.size() != 2) invalid();
        lot.dwell.push_back({number(p[0]), number(p[1])});
    }
    return lot;
}
template<class T, class F> void parseArray(const Json& document, const char* key, int schema, std::vector<T>& out, F parse) {
    if (!document.contains(key)) return;
    if (schema < 31) throw std::invalid_argument(std::string("UNSUPPORTED_FIELD: ") + key);
    const auto& values = document.at(key);
    if (!values.is_array() || values.empty()) invalid(); // never written empty
    try { for (const auto& value : values) out.push_back(parse(value)); }
    catch (const Json::exception&) { invalid(); }
}
}

void addZoneLayerJson(const ZoneLayer& layer, Json& document) {
    if (!layer.zones.empty()) {
        auto& out = document[kZones] = Json::array();
        for (const auto& z : layer.zones) {
            Json polygon = Json::array();
            for (const auto& p : z.polygon) polygon.push_back({p.x, p.y});
            Json attributes = Json::object();
            for (const auto& [name, value] : z.attributes) attributes[name] = value;
            out.push_back({{"id", z.id}, {"name", z.name}, {"polygon", polygon}, {"attributes", attributes}});
        }
    }
    if (!layer.zoneConnectors.empty()) {
        auto& out = document[kConnectors] = Json::array();
        for (const auto& c : layer.zoneConnectors)
            out.push_back({{"id", c.id}, {"zoneId", c.zoneId}, {"linkId", c.linkId}, {"direction", directionName(c.direction)}, {"share", c.share}});
    }
    if (!layer.odMatrices.empty()) {
        auto& out = document[kMatrices] = Json::array();
        for (const auto& m : layer.odMatrices) {
            Json row{{"id", m.id}, {"zoneIds", m.zoneIds}, {"cells", m.cells}};
            if (m.modeId.empty()) row["compositionId"] = m.compositionId; else row["modeId"] = m.modeId; // S1.2
            if (m.periodId.empty()) { row["startTime"] = m.startTime; row["endTime"] = m.endTime; }
            else row["periodId"] = m.periodId; // S1.1
            out.push_back(std::move(row));
        }
    }
    if (!layer.parkingLots.empty()) {
        auto& out = document[kLots] = Json::array();
        for (const auto& lot : layer.parkingLots) {
            Json dwell = Json::array();
            for (const auto& p : lot.dwell) dwell.push_back({p.minutes, p.cumulative});
            out.push_back({{"id", lot.id}, {"zoneId", lot.zoneId}, {"capacity", lot.capacity},
                           {"initialOccupancy", lot.initialOccupancy}, {"initialCompositionId", lot.initialCompositionId},
                           {"gateServiceTime", lot.gateServiceTime}, {"dwell", dwell}});
        }
    }
}
ZoneLayer parseZoneLayer(const Json& document, int schema) {
    ZoneLayer layer;
    parseArray(document, kZones, schema, layer.zones, parseZone);
    parseArray(document, kConnectors, schema, layer.zoneConnectors, parseConnector);
    std::size_t matrix = 0;
    parseArray(document, kMatrices, schema, layer.odMatrices,
               [&](const Json& j) { return parseMatrix(j, "odMatrices[" + std::to_string(matrix++) + "]"); });
    parseArray(document, kLots, schema, layer.parkingLots, parseLot);
    return layer;
}
std::vector<ValidationIssue> zoneLayerIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    const auto& layer = d.zones;
    if (layer.empty()) return issues;
    // IDs share the network's uniqueness rule: none may repeat another object's id.
    auto used = documentIds(d);
    const auto id = [&](const std::string& value, const std::string& path) {
        if (value.empty() || !used.insert(value).second) issues.push_back({"ZONE_ID_INVALID", path + ".id"});
    };
    std::set<std::string> zones, links, periods;
    // S1.1 (STUDY_PERIODS §3): clock seconds, 0 <= start < end <= two days.
    for (std::size_t i = 0; i < layer.studyPeriods.size(); ++i) {
        const auto& p = layer.studyPeriods[i];const auto path = "studyPeriods[" + std::to_string(i) + "]";
        if (p.id.empty() || !used.insert(p.id).second) issues.push_back({"PERIOD_INVALID", path + ".id"});
        periods.insert(p.id);
        if (!std::isfinite(p.start) || p.start < 0 || p.start >= kClockLimit) issues.push_back({"PERIOD_INVALID", path + ".start"});
        if (!std::isfinite(p.end) || p.end > kClockLimit || !(p.end > p.start)) issues.push_back({"PERIOD_INVALID", path + ".end"});
    }
    for (const auto& l : d.network.links) links.insert(l.id);
    for (std::size_t i = 0; i < layer.zones.size(); ++i) {
        const auto& z = layer.zones[i];const auto path = "zones[" + std::to_string(i) + "]";
        id(z.id, path);zones.insert(z.id);
        if (!simplePolygon(z.polygon)) issues.push_back({"ZONE_POLYGON_INVALID", path + ".polygon"});
        for (const auto& [name, value] : z.attributes)
            if (name.empty() || !std::isfinite(value)) issues.push_back({"ZONE_ATTRIBUTE_INVALID", path + ".attributes." + name});
    }
    for (std::size_t i = 0; i < layer.zoneConnectors.size(); ++i) {
        const auto& c = layer.zoneConnectors[i];const auto path = "zoneConnectors[" + std::to_string(i) + "]";
        id(c.id, path);
        if (!zones.contains(c.zoneId)) issues.push_back({"ZONE_CONNECTOR_UNKNOWN_REF", path + ".zoneId"});
        if (!links.contains(c.linkId)) issues.push_back({"ZONE_CONNECTOR_UNKNOWN_REF", path + ".linkId"});
        if (!std::isfinite(c.share) || c.share <= 0) issues.push_back({"ZONE_CONNECTOR_SHARE_INVALID", path + ".share"});
    }
    for (std::size_t i = 0; i < layer.odMatrices.size(); ++i) {
        const auto& m = layer.odMatrices[i];const auto path = "odMatrices[" + std::to_string(i) + "]";
        id(m.id, path);
        if (!m.periodId.empty()) { if (!periods.contains(m.periodId)) issues.push_back({"UNKNOWN_PERIOD", path + ".periodId"}); }
        else if (!std::isfinite(m.startTime) || !std::isfinite(m.endTime) || m.startTime < 0 || m.endTime <= m.startTime)
            issues.push_back({"OD_PERIOD_INVALID", path});
        if (m.modeId.empty() && m.compositionId.empty() && m.vehicleTypeId.empty())
            issues.push_back({"OD_COMPOSITION_MISSING", path + ".compositionId"});
        std::set<std::string> seen;
        for (std::size_t k = 0; k < m.zoneIds.size(); ++k) {
            const auto at = path + ".zoneIds[" + std::to_string(k) + "]";
            if (!zones.contains(m.zoneIds[k])) issues.push_back({"OD_ZONE_UNKNOWN", at});
            else if (!seen.insert(m.zoneIds[k]).second) issues.push_back({"OD_ZONE_REPEATED", at});
        }
        const auto n = m.zoneIds.size();
        bool square = n > 0 && m.cells.size() == n;
        for (const auto& row : m.cells) square = square && row.size() == n;
        if (!square) { issues.push_back({"OD_CELLS_SHAPE", path + ".cells"});continue; }
        for (std::size_t r = 0; r < n; ++r)
            for (std::size_t c = 0; c < n; ++c)
                if (!std::isfinite(m.cells[r][c]) || m.cells[r][c] < 0)
                    issues.push_back({"OD_CELL_INVALID", path + ".cells[" + std::to_string(r) + "][" + std::to_string(c) + "]"});
    }
    std::set<std::string> lotZones;
    for (std::size_t i = 0; i < layer.parkingLots.size(); ++i) {
        const auto& lot = layer.parkingLots[i];const auto path = "parkingLots[" + std::to_string(i) + "]";
        const auto bad = [&](const std::string& field) { issues.push_back({"PARKING_LOT_INVALID", path + "." + field}); };
        id(lot.id, path);
        if (!zones.contains(lot.zoneId) || !lotZones.insert(lot.zoneId).second) bad("zoneId");
        if (lot.capacity < 1) bad("capacity");
        if (lot.initialOccupancy > lot.capacity) bad("initialOccupancy");
        if ((lot.initialOccupancy > 0) == lot.initialCompositionId.empty()) bad("initialCompositionId");
        if (!std::isfinite(lot.gateServiceTime) || lot.gateServiceTime < 0) bad("gateServiceTime");
        bool dwell = !lot.dwell.empty() && lot.dwell.back().cumulative == 1;
        for (std::size_t k = 0; k < lot.dwell.size(); ++k) {
            const auto& p = lot.dwell[k];
            dwell = dwell && std::isfinite(p.minutes) && p.minutes >= 0 && std::isfinite(p.cumulative) && p.cumulative >= 0 &&
                    (k == 0 || (p.minutes > lot.dwell[k - 1].minutes && p.cumulative > lot.dwell[k - 1].cumulative));
        }
        if (!dwell) issues.push_back({"PARKING_DWELL_INVALID", path + ".dwell"});
    }
    return issues;
}
}
