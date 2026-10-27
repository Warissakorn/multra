#include "object_csv.hpp"
#include "document.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <span>
#include <string_view>

namespace multra {
namespace {
struct Column {const char* heading;const char* path;};
constexpr Column links[]={
    {"id","/id"},{"name","/name"},{"level","/level"},{"displayType","/displayType"},
    {"laneOffset_m","/laneOffset"},{"referenceLength_m","/referenceLength"},{"laneCount","/laneCount"},
    {"geometry","/geometry"},{"lanes","/lanes"},{"boundaryMarkings","/boundaryMarkings"},
    {"behaviourType","/behaviourType"},{"linkType","/linkType"},{"roadId","/roadId"}};
constexpr Column connectors[]={
    {"id","/id"},{"name","/name"},{"fromLinkId","/from/linkId"},{"fromLaneId","/from/laneId"},
    {"fromStation_m","/from/station"},{"toLinkId","/to/linkId"},{"toLaneId","/to/laneId"},{"toStation_m","/to/station"},
    {"fromLaneCount","/fromLaneCount"},{"toLaneCount","/toLaneCount"},{"level","/level"},{"displayType","/displayType"},
    {"referenceLength_m","/referenceLength"},{"geometry","/geometry"},{"laneBlend","/laneBlend"},
    {"laneWidths","/laneWidths"},{"laneMarkings","/laneMarkings"},{"laneChangeSide","/laneChangeSide"},
    {"behaviourType","/behaviourType"},{"generated","/generated"}};
constexpr Column inputs[]={
    {"id","/id"},{"routeId","/routeId"},{"vehicleTypeId","/vehicleTypeId"},{"vehiclesPerHour","/vehiclesPerHour"},
    {"startTime_s","/startTime"},{"endTime_s","/endTime"},{"laneShares","/laneShares"},{"compositionId","/compositionId"},
    {"routingDecisionId","/routingDecisionId"},{"linkId","/linkId"},{"volumeFromCounts","/volumeFromCounts"},{"intervals","/intervals"}};
constexpr Column routes[]={
    {"id","/id"},{"segmentIds","/segmentIds"},{"destinationObjectId","/destination/objectId"},{"destinationStation_m","/destination/station"}};
constexpr Column decisions[]={
    {"id","/id"},{"name","/name"},{"linkId","/linkId"},{"position_m","/position"},
    {"routes","/routes"},{"intervals","/intervals"},{"typeRules","/typeRules"}};
// Authored text opens in spreadsheets; a leading formula character would run as a formula there.
std::string spreadsheetText(std::string text) {
    if(!text.empty() && std::string_view("=+-@\t\r").find(text.front())!=std::string_view::npos)text.insert(0,1,'\'');
    return text;
}
std::span<const Column> columns(ObjectTable table) {
    switch(table) {
    case ObjectTable::links:return links;
    case ObjectTable::connectors:return connectors;
    case ObjectTable::inputs:return inputs;
    case ObjectTable::routes:return routes;
    case ObjectTable::decisions:return decisions;
    }
    throw std::invalid_argument("Unknown object table");
}
Json rows(const ProjectDocument& d,ObjectTable table) {
    // Reuse persisted field serialization without copying images or resolving external catalogs.
    if(table==ObjectTable::links || table==ObjectTable::connectors) {
        ProjectDocument view;
        if(table==ObjectTable::links)view.network.links=d.network.links;
        else view.network.connectors=d.network.connectors;
        auto result=documentJson(view).at("network").at(objectTableName(table));
        for(std::size_t i=0;i<result.size();++i) {
            result[i]["referenceLength"]=polylineLength(table==ObjectTable::links?d.network.links[i].geometry:d.network.connectors[i].geometry);
            if(table==ObjectTable::links)result[i]["laneCount"]=d.network.links[i].lanes.size();
        }
        return result;
    }
    if(!d.definition)return Json::array();
    AuthoringDefinition view;
    if(table==ObjectTable::inputs)view.inputs=d.definition->inputs;
    else if(table==ObjectTable::routes)view.routes=d.definition->routes;
    else view.routingDecisions=d.definition->routingDecisions;
    const auto j=definitionJson(view);
    const auto key=table==ObjectTable::decisions?"routingDecisions":objectTableName(table);
    auto result=j.contains(key)?j.at(key):Json::array();
    if(table==ObjectTable::routes)for(auto& row:result)
        if(const auto it=d.definition->routeDestinations.find(row.at("id").get<std::string>());it!=d.definition->routeDestinations.end())
            row["destination"]={{"objectId",it->second.objectId},{"station",it->second.station}};
    return result;
}
}
const char* objectTableName(ObjectTable table) {
    switch(table) {
    case ObjectTable::links:return "links";
    case ObjectTable::connectors:return "connectors";
    case ObjectTable::inputs:return "inputs";
    case ObjectTable::routes:return "routes";
    case ObjectTable::decisions:return "decisions";
    }
    throw std::invalid_argument("Unknown object table");
}
std::string objectTableCsv(const ProjectDocument& d,ObjectTable table) {
    const auto fields=columns(table);std::string out="\xEF\xBB\xBF";
    const auto endCell=[&](std::size_t i){out+=i+1==fields.size()?"\r\n":",";};
    for(std::size_t i=0;i<fields.size();++i){out+=fields[i].heading;endCell(i);}
    for(const auto& row:rows(d,table))for(std::size_t i=0;i<fields.size();++i) {
        const Json::json_pointer path(fields[i].path);
        if(row.contains(path) && !row.at(path).is_null()) {
            const auto& value=row.at(path);
            out+=value.is_string()?csvQuoted(spreadsheetText(value.get<std::string>())):
                value.is_structured()?csvQuoted(value.dump()):value.dump();
        }
        endCell(i);
    }
    return out;
}
}
