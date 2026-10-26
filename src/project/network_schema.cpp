#include "network_schema.hpp"
#include "network_fields.hpp"
#include "document.hpp"
#include "document_envelope.hpp"
#include <nlohmann/json.hpp>
#include <map>

namespace multra {
namespace {
using namespace networkCodec;
class NetworkSchema {
    Json definitions_=Json::object();
    std::map<std::string,std::string> names_;
    Json fieldSchema(const Field& field,int version) {
        Json rule=Json::object();
        switch(field.kind) {
        case Kind::text:rule={{"type",field.nullable?Json::array({"string","null"}):Json("string")}};break;
        case Kind::side:
            rule={{"type",field.nullable?Json::array({"string","null"}):Json("string")},
                {"enum",field.nullable?Json::array({"left","right",nullptr}):Json::array({"left","right"})}};
            break;
        case Kind::number:rule={{"type","number"}};break;
        case Kind::level:rule={{"type","integer"},{"minimum",-levelLimit},{"maximum",levelLimit},{"x-multra-nativeType","integer-token"}};break;
        case Kind::group:rule={{"type","integer"},{"minimum",groupMinimum},{"maximum",groupMaximum}};break;
        case Kind::array:rule={{"type","array"},{"items",Json::object()}};break;
        case Kind::links:rule={{"type","array"},{"items",reference(Object::link,version)}};break;
        case Kind::connectors:rule={{"type","array"},{"items",reference(Object::connector,version)}};break;
        case Kind::signalHeads:rule={{"type","array"},{"items",reference(Object::signalHead,version)}};break;
        case Kind::controllers:rule={{"type","array"},{"items",reference(Object::signalController,version)}};break;
        case Kind::groups:rule={{"type","array"},{"items",reference(Object::signalGroup,version)}};break;
        case Kind::programs:rule={{"type","array"},{"items",reference(Object::signalProgram,version)}};break;
        case Kind::phases:rule={{"type","array"},{"items",reference(Object::signalPhase,version)}};break;
        case Kind::color: {
            Json names=Json::array();for(const auto& entry:signalColors)names.push_back(entry.first);
            rule={{"type","string"},{"enum",names}};break;
        }
        case Kind::rightOfWay:rule=reference(Object::rightOfWay,version);break;
        case Kind::waitingLines:rule={{"type","array"},{"items",reference(Object::waitingLine,version)}};break;
        case Kind::controlPoint:
            rule=reference(Object::controlPoint,version);
            if(field.nullable)rule={{"anyOf",Json::array({rule,Json{{"type","null"}}})}};
            break;
        case Kind::controlPath:rule={{"allOf",Json::array({reference(Object::controlPath,version)})},{"not",{{"type","null"}}}};break;
        case Kind::conflictAreas:rule={{"type","array"},{"items",reference(Object::conflictArea,version)}};break;
        case Kind::conflictSide:rule=reference(Object::conflictSide,version);break;
        case Kind::priorityRules:rule={{"type","array"},{"items",reference(Object::priorityRule,version)}};break;
        case Kind::stopControls:rule={{"type","array"},{"items",reference(Object::stopControl,version)}};break;
        case Kind::queueCounters:rule={{"type","array"},{"items",reference(Object::queueCounter,version)}};break;
        case Kind::measurementLines:rule={{"type","array"},{"items",reference(Object::measurementLine,version)}};break;
        case Kind::travelTimeSections:rule={{"type","array"},{"items",reference(Object::travelTimeSection,version)}};break;
        case Kind::sectionLine:rule=reference(Object::sectionLine,version);break;
        case Kind::sectionControl:
            rule={{"type",Json::array({"string","null"})},
                {"enum",Json::array({sectionControlName(SectionControl::signalised),sectionControlName(SectionControl::unsignalised),nullptr})}};break;
        case Kind::texts:rule={{"type","array"},{"items",{{"type","string"}}}};break;
        case Kind::conflictKind:rule={{"type","string"},{"enum",Json::array({conflictKindName(ConflictKind::crossing),conflictKindName(ConflictKind::merge)})}};break;
        case Kind::conflictPriority:rule={{"type","string"},{"enum",Json::array({conflictPriorityName(ConflictPriority::firstYields),conflictPriorityName(ConflictPriority::secondYields),conflictPriorityName(ConflictPriority::undetermined)})}};break;
        case Kind::stopMode:rule={{"type","string"},{"enum",Json::array({stopModeName(StopMode::stop),stopModeName(StopMode::yield)})}};break;
        case Kind::reference:rule=reference(Object::reference,version);break;
        case Kind::numbers:rule={{"type","array"},{"items",{{"type","number"}}}};break;
        case Kind::generated:rule={{"type","boolean"},{"const",true}};break;
        case Kind::lanes:rule={{"type","array"},{"items",reference(Object::lane,version)}};break;
        case Kind::geometry:rule={{"type","array"},{"items",reference(Object::point,version)}};break;
        case Kind::markings: {
            Json names=Json::array();
            for(const auto m:{MarkingType::solid,MarkingType::dashed,MarkingType::none,MarkingType::doubleLine})names.push_back(markingName(m));
            rule={{"type","array"},{"items",{{"type","string"},{"enum",names}}}};break;
        }
        case Kind::unchecked:break;
        }
        if(field.nullable && rule.contains("type") && rule.at("type")=="array")
            rule["type"]=Json::array({"array","null"});
        return rule;
    }
    Json objectSchema(Object object,int version) {
        Json properties=Json::object(),required=Json::array(),conditions=Json::array();
        for(const auto& field:fields(object)) {
            if(version<field.rejectBefore || (field.rejectFrom && version>=field.rejectFrom)) {properties[field.name]=false;continue;}
            if(version<field.knownFrom)continue;
            properties[field.name]=fieldSchema(field,version);
            if(field.whenNonNull) {
                conditions.push_back({{"if",{{"required",Json::array({field.whenNonNull})},
                    {"properties",{{field.whenNonNull,{{"not",{{"type","null"}}}}}}}}},
                    {"then",{{"required",Json::array({field.name})},{"properties",{{field.name,properties.at(field.name)}}}}}});
                properties[field.name]=Json::object(); // An inactive field is ignored by the codec.
            }
            if(field.required)required.push_back(field.name);
        }
        if(object==Object::link) {
            const Link defaults;
            properties["level"]["default"]=defaults.level;
            properties["laneOffset"]["default"]=defaults.laneOffset;
            properties["displayType"]["default"]=defaults.displayType;
            properties["name"]["default"]=defaults.name;
            properties["boundaryMarkings"]["default"]=Json::array();
        }
        if(object==Object::connector) {
            const Connector defaults;
            properties["fromLaneCount"]["default"]=defaults.fromLaneCount;
            properties["toLaneCount"]["default"]=defaults.toLaneCount;
            properties["level"]["default"]=defaults.level;
            properties["displayType"]["default"]=defaults.displayType;
            properties["name"]["default"]=defaults.name;
            for(const auto* key:{"laneBlend","laneWidths","laneMarkings"})properties[key]["default"]=Json::array();
        }
        if(object==Object::signalHead) {
            const NetworkSignalHead defaults;
            properties["name"]["default"]=defaults.name;
            properties["connectorId"]["default"]=defaults.connectorId;
        }
        Json result={{"type","object"},{"properties",properties},{"required",required},{"additionalProperties",allowsAdditionalFields(object,version)}};
        // These legacy readers ignore non-object containers/references; semantic
        // path validity stays with the model. Required path fields still reject null.
        if(object==Object::rightOfWay || object==Object::controlPath || object==Object::measurementLine)result.erase("type");
        if(object==Object::definition)result["type"]=Json::array({"object","null"});
        if(!conditions.empty())result["allOf"]=std::move(conditions);
        return result;
    }
public:
    Json reference(Object object,int version) {
        const auto schema=objectSchema(object,version);
        const auto signature=schema.dump();
        auto it=names_.find(signature);
        if(it==names_.end()) {
            const auto prefix=[&]() {
                switch(object) {
                case Object::network:return "network";
                case Object::link:return "link";
                case Object::lane:return "lane";
                case Object::point:return "point";
                case Object::connector:return "connector";
                case Object::reference:return "reference";
                case Object::signalHead:return "signalHead";
                case Object::definition:return "definition";
                case Object::signalController:return "signalController";
                case Object::signalGroup:return "signalGroup";
                case Object::signalProgram:return "signalProgram";
                case Object::signalPhase:return "signalPhase";
                case Object::rightOfWay:return "rightOfWay";
                case Object::waitingLine:return "waitingLine";
                case Object::controlPoint:return "controlPoint";
                case Object::controlPath:return "controlPath";
                case Object::conflictArea:return "conflictArea";
                case Object::conflictSide:return "conflictSide";
                case Object::priorityRule:return "priorityRule";
                case Object::stopControl:return "stopControl";
                case Object::queueCounter:return "queueCounter";
                case Object::measurementLine:return "measurementLine";
                case Object::travelTimeSection:return "travelTimeSection";
                case Object::sectionLine:return "sectionLine";
                }
                throw std::logic_error("Unknown schema object");
            }();
            const auto name=std::string(prefix)+std::to_string(version);
            definitions_[name]=schema;
            it=names_.emplace(signature,name).first;
        }
        return {{"$ref","#/$defs/"+it->second}};
    }
    Json compose() {
        auto result=projectEnvelopeSchema();
        result["x-multra-coverage"]="envelope-background-network-link-lane-connector-reference-signal-head-signal-control-right-of-way-measurements";
        result["$comment"]="Partial envelope/background and versioned Network/Link/Lane/Connector/SignalHead/geometry/attachment-reference and signal controller/group/program/phase and right-of-way/queue-counter/travel-time-section structure. Model invariants and other demand/optional features require the codec. Native integer tokens and UTF-8 byte limits also require the codec.";
        Json groups=Json::array();std::map<std::string,std::size_t> indices;
        for(int version=1;version<=kLatestSchema;++version) {
            const auto ref=reference(Object::network,version);
            const auto definition=reference(Object::definition,version);
            const auto key=ref.dump()+definition.dump();
            auto [it,inserted]=indices.emplace(key,groups.size());
            if(inserted)groups.push_back({{"if",{{"properties",{{"schemaVersion",{{"enum",Json::array()}}}}}}},
                {"then",{{"properties",{{"network",ref},{"definition",definition}}}}}});
            groups[it->second]["if"]["properties"]["schemaVersion"]["enum"].push_back(version);
        }
        result["$defs"]=std::move(definitions_);result["allOf"]=std::move(groups);
        return result;
    }
};
}
Json projectDocumentSchema() {return NetworkSchema{}.compose();}
}
