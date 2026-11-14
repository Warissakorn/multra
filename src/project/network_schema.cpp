#include "network_schema.hpp"
#include "latest_schema.hpp"
#include "network_fields.hpp"
#include "document.hpp"
#include "document_envelope.hpp"
#include "map_settings.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
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
        case Kind::boolean:rule={{"type","boolean"}};break;
        case Kind::number:rule={{"type","number"}};break;
        case Kind::level:rule={{"type","integer"},{"minimum",-levelLimit},{"maximum",levelLimit},{"x-multra-nativeType","integer-token"}};break;
        case Kind::group:rule={{"type","integer"},{"minimum",groupMinimum},{"maximum",groupMaximum}};break;
        case Kind::array:rule={{"type","array"},{"items",Json::object()}};break;
        case Kind::links:rule={{"type","array"},{"items",reference(Object::link,version)}};break;
        case Kind::connectors:rule={{"type","array"},{"items",reference(Object::connector,version)}};break;
        case Kind::signalHeads:rule={{"type","array"},{"items",reference(Object::signalHead,version)}};break;
        case Kind::routes:rule={{"type","array"},{"items",reference(Object::route,version)}};break;
        case Kind::inputs:rule={{"type","array"},{"items",reference(Object::vehicleInput,version)}};break;
        case Kind::volumeIntervals:rule={{"type","array"},{"items",reference(Object::volumeInterval,version)}};break;
        case Kind::routingDecisions:rule={{"type","array"},{"items",reference(Object::routingDecision,version)}};break;
        case Kind::routingRoutes:rule={{"type","array"},{"items",reference(Object::routingRoute,version)}};break;
        case Kind::routingIntervals:rule={{"type","array"},{"items",reference(Object::routingInterval,version)}};break;
        case Kind::routingTypeRules:rule={{"type","array"},{"items",reference(Object::routingTypeRule,version)}};break;
        case Kind::compositions:rule={{"type","array"},{"items",reference(Object::composition,version)}};break;
        case Kind::compositionShares:rule={{"type","array"},{"items",reference(Object::compositionShare,version)}};break;
        case Kind::compositionIntervals:rule={{"type","array"},{"items",reference(Object::compositionInterval,version)}};break;
        case Kind::modes:rule={{"type","array"},{"items",reference(Object::mode,version)}};break;
        case Kind::vehicleTypes:rule={{"type","array"},{"items",reference(Object::vehicleType,version)}};break;
        case Kind::speedRange:rule=reference(Object::speedRange,version);break;
        case Kind::speedCurve:rule={{"type","array"},{"items",reference(Object::speedPoint,version)}};break;
        case Kind::axles:rule=reference(Object::axles,version);break;
        case Kind::pcu:rule=reference(Object::pcu,version);break;
        case Kind::kerbLanes:rule={{"type","integer"},{"x-multra-nativeType","integer-token"}};break;
        case Kind::behaviours: { // D563: from 25 the model selects the W74 row
            Json items=reference(Object::behaviour,version);
            if(version>=w74ModelVersion)items={{"if",{{"required",Json::array({"model"})},{"properties",{{"model",{{"const",kW74BehaviourModel}}}}}}},
                {"then",reference(Object::w74Behaviour,version)},{"else",items}};
            rule={{"type","array"},{"items",items}};break;
        }
        case Kind::behaviourModel:
            rule={{"type","string"},{"enum",version<w74ModelVersion?Json::array({kPrototypeBehaviourModel}):Json::array({kPrototypeBehaviourModel,kW74BehaviourModel})}};break;
        case Kind::w74Model:rule={{"type","string"},{"const",kW74BehaviourModel}};break;
        case Kind::integerToken:rule={{"type","integer"},{"x-multra-nativeType","integer-token"}};break;
        case Kind::vehicleClasses:rule={{"type","array"},{"items",reference(Object::vehicleClass,version)}};break;
        case Kind::linkBehaviourTypes:rule={{"type","array"},{"items",reference(Object::linkBehaviourType,version)}};break;
        case Kind::behaviourOverrides:rule={{"type","array"},{"items",reference(Object::behaviourOverride,version)}};break;
        case Kind::zones:rule={{"type","array"},{"minItems",1},{"items",reference(Object::zone,version)}};break;
        case Kind::zoneConnectors: { // D566: a street-segment key selects the street row from 57
            Json items=reference(Object::zoneLinkConnector,version);
            if(version>=streetZoneConnectorVersion)items={{"if",{{"required",Json::array({streetSegmentIdKey(version)})}}},
                {"then",reference(Object::zoneStreetConnector,version)},{"else",items}};
            rule={{"type","array"},{"minItems",1},{"items",items}};break;
        }
        case Kind::odMatrices:rule={{"type","array"},{"minItems",1},{"items",reference(Object::odMatrix,version)}};break;
        case Kind::parkingLots:rule={{"type","array"},{"minItems",1},{"items",reference(Object::parkingLot,version)}};break;
        case Kind::pairs:rule={{"type","array"},{"items",{{"type","array"},{"minItems",2},{"maxItems",2},{"items",{{"type","number"}}}}}};break;
        case Kind::numberMap:rule={{"type","object"},{"additionalProperties",{{"type","number"}}}};break;
        case Kind::zoneDirection:rule={{"type","string"},{"enum",Json::array({"origin","destination"})}};break;
        case Kind::travelDirection:rule={{"type","string"},{"enum",Json::array({"forward","backward"})}};break;
        case Kind::segmentId:rule={{"type","string"},{"minLength",1}};break;
        case Kind::unsignedCount:
            rule={{"type","integer"},{"minimum",0},{"maximum",0xffffffffULL},{"x-multra-nativeType","unsigned-integer-token"}};break;
        case Kind::generatedBy:rule={{"type","string"},{"const",kGeneratedByModeChoice}};break;
        case Kind::studyPeriods:rule={{"type","array"},{"minItems",1},{"items",reference(Object::studyPeriod,version)}};break;
        case Kind::studyArea:rule=reference(Object::studyArea,version);break;
        case Kind::nodeIds:rule={{"type","array"},{"minItems",1},{"items",{{"type","string"}}}};break;
        case Kind::flowSource:rule={{"type","string"},{"enum",Json::array({"typed","macro","meso"})}};break;
        case Kind::gateFlows:rule={{"type","array"},{"minItems",1},{"items",reference(Object::gateFlow,version)}};break;
        case Kind::map:rule=reference(Object::map,version);break;
        case Kind::mapReference:rule=reference(Object::mapReference,version);break;
        case Kind::providerId:rule={{"type","string"},{"pattern","^[a-z0-9-]{1,80}$"}};break;
        case Kind::opacity:rule={{"type","number"},{"minimum",0},{"maximum",1}};break;
        case Kind::utmCrs:rule={{"type","string"},{"pattern","^EPSG:32[67](0[1-9]|[1-5][0-9]|60)$"}};break;
        case Kind::longitude:rule={{"type","number"},{"minimum",-180},{"maximum",180}};break;
        case Kind::latitude:rule={{"type","number"},{"minimum",-80},{"maximum",84}};break;
        case Kind::worldAnchor:rule={{"type","number"},{"minimum",-mapWorldAnchorLimit},{"maximum",mapWorldAnchorLimit}};break;
        case Kind::rotation:rule={{"type","number"},{"minimum",-180},{"maximum",180}};break;
        case Kind::macroRuns:rule={{"type","array"},{"minItems",1},{"items",reference(Object::macroRun,version)}};break;
        case Kind::macroSettings:rule=reference(Object::macroSettings,version);break;
        case Kind::targetGap:rule={{"type","number"},{"exclusiveMinimum",0},{"exclusiveMaximum",1}};break;
        case Kind::iterationCap:rule={{"type","integer"},{"minimum",1},{"maximum",100000},{"x-multra-nativeType","integer-token"}};break;
        case Kind::tripRateSets:rule={{"type","array"},{"minItems",1},{"items",reference(Object::tripRateSet,version)}};break;
        case Kind::tripRateSource:rule=reference(Object::tripRateSource,version);break;
        case Kind::tripRatePurposes:rule={{"type","array"},{"minItems",1},{"items",reference(Object::tripRatePurpose,version)}};break;
        case Kind::tripRateSide: // D570: `method` selects the row; any other method falls to the cross-classification row and its enum
            rule={{"if",{{"required",Json::array({"method"})},{"properties",{{"method",{{"const",kRegression}}}}}}},
                {"then",reference(Object::tripRateRegressionSide,version)},{"else",reference(Object::tripRateCrossSide,version)}};break;
        case Kind::tripRateCategories:rule={{"type","array"},{"minItems",1},{"items",reference(Object::tripRateCategory,version)}};break;
        case Kind::tripRateTerms:rule={{"type","array"},{"items",reference(Object::tripRateTerm,version)}};break;
        case Kind::crossMethod:rule={{"type","string"},{"const",kCrossClassification}};break;
        case Kind::regressionMethod:rule={{"type","string"},{"const",kRegression}};break;
        case Kind::rate:rule={{"type","number"},{"minimum",0}};break;
        case Kind::numberMatrix:rule={{"type","array"},{"items",{{"type","array"},{"items",{{"type","number"}}}}}};break;
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
        if(field.nullable && rule.contains("type") && rule.at("type").is_string())
            rule["type"]=Json::array({rule.at("type"),"null"});
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
            if(field.unless) { // D566: required unless the alternative is present, refused when it is
                const auto list=fields(object);
                const auto alternative=std::find_if(list.begin(),list.end(),[&](const auto& f){return std::string_view(field.unless)==f.name;});
                if(alternative==list.end())throw std::logic_error("Unknown alternative field");
                if(activeField(*alternative,version)) {
                    conditions.push_back({{"if",{{"required",Json::array({field.unless})}}},
                        {"then",{{"properties",{{field.name,false}}}}},{"else",{{"required",Json::array({field.name})}}}});
                    continue;
                }
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
                case Object::route:return "route";
                case Object::vehicleInput:return "vehicleInput";
                case Object::volumeInterval:return "volumeInterval";
                case Object::routingDecision:return "routingDecision";
                case Object::routingRoute:return "routingRoute";
                case Object::routingInterval:return "routingInterval";
                case Object::routingTypeRule:return "routingTypeRule";
                case Object::composition:return "composition";
                case Object::compositionShare:return "compositionShare";
                case Object::compositionInterval:return "compositionInterval";
                case Object::mode:return "mode";
                case Object::vehicleType:return "vehicleType";
                case Object::speedRange:return "speedRange";
                case Object::speedPoint:return "speedPoint";
                case Object::axles:return "axles";
                case Object::pcu:return "pcu";
                case Object::behaviour:return "behaviour";
                case Object::w74Behaviour:return "w74Behaviour";
                case Object::vehicleClass:return "vehicleClass";
                case Object::linkBehaviourType:return "linkBehaviourType";
                case Object::behaviourOverride:return "behaviourOverride";
                case Object::rootFeatures:return "rootFeatures";
                case Object::zone:return "zone";
                case Object::zoneLinkConnector:return "zoneLinkConnector";
                case Object::zoneStreetConnector:return "zoneStreetConnector";
                case Object::odMatrix:return "odMatrix";
                case Object::parkingLot:return "parkingLot";
                case Object::studyPeriod:return "studyPeriod";
                case Object::studyArea:return "studyArea";
                case Object::gateFlow:return "gateFlow";
                case Object::map:return "map";
                case Object::mapReference:return "mapReference";
                case Object::macroRun:return "macroRun";
                case Object::macroSettings:return "macroSettings";
                case Object::tripRateSet:return "tripRateSet";
                case Object::tripRateSource:return "tripRateSource";
                case Object::tripRatePurpose:return "tripRatePurpose";
                case Object::tripRateCrossSide:return "tripRateCrossSide";
                case Object::tripRateRegressionSide:return "tripRateRegressionSide";
                case Object::tripRateCategory:return "tripRateCategory";
                case Object::tripRateTerm:return "tripRateTerm";
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
        result["x-multra-coverage"]="envelope-background-network-link-lane-connector-reference-signal-head-signal-control-right-of-way-measurements-routes-inputs-routing-decisions-compositions-modes-vehicle-types-behaviours-behaviour-classes-zones-studies-map-macro-runs-trip-rates";
        result["$comment"]="Partial envelope/background and versioned Network/Link/Lane/Connector/SignalHead/geometry/attachment-reference and signal controller/group/program/phase and right-of-way/queue-counter/travel-time-section and base definition/Route/VehicleInput/interval/RoutingDecision and owned composition/type-share/interval, mode, vehicle-type per-model behaviour and vehicle-class/link-behaviour-type structure, and the root zone/zone-connector/OD-matrix/parking-lot layer, study periods, clockStart, study area, map settings, Macro settings and runs, and owned trip-rate sets. Model invariants and other demand/optional features require the codec. Native integer tokens and UTF-8 byte limits also require the codec.";
        Json groups=Json::array();std::map<std::string,std::size_t> indices;
        for(int version=1;version<=kLatestSchema;++version) {
            const auto ref=reference(Object::network,version);
            const auto definition=reference(Object::definition,version);
            // D566: the root zone layer sits beside network and definition (false below 31).
            Json properties=objectSchema(Object::rootFeatures,version).at("properties");
            properties["network"]=ref;properties["definition"]=definition;
            const auto key=properties.dump();
            auto [it,inserted]=indices.emplace(key,groups.size());
            if(inserted)groups.push_back({{"if",{{"properties",{{"schemaVersion",{{"enum",Json::array()}}}}}}},
                {"then",{{"properties",std::move(properties)}}}});
            groups[it->second]["if"]["properties"]["schemaVersion"]["enum"].push_back(version);
        }
        result["$defs"]=std::move(definitions_);result["allOf"]=std::move(groups);
        return result;
    }
};
}
Json projectDocumentSchema() {return NetworkSchema{}.compose();}
}
