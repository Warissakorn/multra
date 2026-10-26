#include "json.hpp"
#include "network_fields.hpp"
#include "node_codec.hpp"
#include "../core/validate.hpp"
#include "../core/w74.hpp"
#include "../core/perception.hpp"
#include "../core/lateral.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace multra {
bool present(const Json& value, const char* name) {
    return value.is_object() && value.contains(name) && !value.at(name).is_null();
}
const Json& section(const Json& value, const char* name) {
    if (!present(value, name)) throw std::invalid_argument(std::string("Missing section: ") + name);
    return value.at(name);
}
namespace {
// Schema 7 is implemented incrementally. Never accept a target-spec field and then silently
// discard it on save (or imply its simulation behavior exists). Legacy readers stay additive.
void knownFields(const Json& value,std::initializer_list<const char*> keys,
                 const std::string& path,int version) {
    if(version<7 || !value.is_object())return;
    for(const auto& [key,unused]:value.items()) {
        (void)unused;
        if(std::none_of(keys.begin(),keys.end(),[&](const char* k){return key==k;}))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD",path+"."+key}});
    }
}
// Every accessor goes through this: reaching .at() on a null or a non-object is how an
// nlohmann type_error escapes to the user instead of a sentence naming the missing field.
const Json& member(const Json& value, const char* name) {
    if (!value.is_object()) throw std::invalid_argument(std::string("Expected an object containing: ") + name);
    if (!value.contains(name)) throw std::invalid_argument(std::string("Missing field: ") + name);
    if (value.at(name).is_null()) throw std::invalid_argument(std::string("Field is null: ") + name);
    return value.at(name);
}
template<class T> T field(const Json& value, const char* name) {
    const auto& item = member(value, name);
    if constexpr (std::is_same_v<T, double>) {
        if (!item.is_number()) throw std::invalid_argument(std::string("Expected number: ") + name);
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!item.is_string()) throw std::invalid_argument(std::string("Expected text: ") + name);
    } else if constexpr (std::is_same_v<T, bool>) {
        if (!item.is_boolean()) throw std::invalid_argument(std::string("Expected true or false: ") + name);
    }
    return item.get<T>();
}
const Json& array(const Json& value, const char* name) {
    const auto& items = member(value, name);
    if (!items.is_array()) throw std::invalid_argument(std::string("Expected array: ") + name);
    return items;
}
std::vector<std::string> strings(const Json& value, const char* name) {
    std::vector<std::string> result;
    for (const auto& item : array(value, name)) {
        if (!item.is_string()) throw std::invalid_argument(std::string("Expected strings: ") + name);
        result.push_back(item.get<std::string>());
    }
    return result;
}
std::vector<double> doubles(const Json& value, const char* name) {
    std::vector<double> result;
    for (const auto& item : array(value, name)) {
        if (!item.is_number()) throw std::invalid_argument(std::string("Expected numbers: ") + name);
        result.push_back(item.get<double>());
    }
    return result;
}
// Below schema 5 the stored number is a fraction of the lane's own arclength. Convert it to a
// station on the link once every link is parsed, so the attachment keeps the world position it
// was drawn at. Signal heads share LaneReference, so they are walked too.
void migrateAttachments(Network& network) {
    const auto convert=[&](LaneReference& ref,bool outgoing) {
        if(!ref.station)return;
        for(const auto& link:network.links)if(link.id==ref.linkId) {
            const auto lane=laneGeometry(link,ref.laneId,network.drivingSide);
            const double station=matchedStation(lane,link.geometry,*ref.station*polylineLength(lane));
            const double reference=polylineLength(link.geometry);
            // An attachment that was exactly at the end keeps meaning "the end" as the link changes.
            if(station<=0 && !outgoing)ref.station.reset();
            else if(station>=reference && outgoing)ref.station.reset();
            else ref.station=std::clamp(station,0.,reference);
            return;
        }
    };
    for(auto& c:network.connectors){convert(c.from,true);convert(c.to,false);}
    for(auto& h:network.signalHeads)convert(h.lane,false);
}

}
Network parseNetwork(const Json& value, int schemaVersion) {
    networkCodec::knownFields(value,networkCodec::Object::network,"network",schemaVersion);
    Network network;
    network.id = networkCodec::value(value,networkCodec::Object::network,"id").get<std::string>();
    const auto side = networkCodec::value(value,networkCodec::Object::network,"drivingSide").get<std::string>();
    if (side != "left" && side != "right") throw std::invalid_argument("INVALID_DRIVING_SIDE");
    network.drivingSide = side == "left" ? DrivingSide::left : DrivingSide::right;
    network.links=networkCodec::readLinks(value,schemaVersion);
    network.connectors=networkCodec::readConnectors(value,schemaVersion);
    network.signalHeads=networkCodec::readSignalHeads(value,schemaVersion);
    if(schemaVersion<5)migrateAttachments(network);
    if(schemaVersion>=14 && present(value,"rightOfWay"))network.rightOfWay=networkCodec::readRightOfWay(value.at("rightOfWay"),schemaVersion);
    if(schemaVersion>=16 && present(value,"queueCounters"))for(const auto& c:array(value,"queueCounters")) {
        const auto path="queueCounters["+std::to_string(network.queueCounters.size())+"]";
        knownFields(c,{"id","name","lines"},path,schemaVersion);
        AuthoredQueueCounter counter{field<std::string>(c,"id"),present(c,"name")?field<std::string>(c,"name"):"",{}};
        for(const auto& l:array(c,"lines")) {
            const auto at=path+".lines["+std::to_string(counter.lines.size())+"]";
            knownFields(l,{"referenceId","point"},at,schemaVersion);
            MeasurementLine line;
            if(present(l,"referenceId"))line.referenceId=field<std::string>(l,"referenceId");
            if(present(l,"point")) {
                const auto& point=member(l,"point");
                knownFields(point,{"path","station"},at+".point",schemaVersion);
                line.point=ControlPoint{networkCodec::readControlPath(member(point,"path"),at+".point.path",schemaVersion),field<double>(point,"station")};
            }
            counter.lines.push_back(std::move(line));
        }
        network.queueCounters.push_back(std::move(counter));
    }
    if(schemaVersion>=23 && present(value,"travelTimeSections"))for(const auto& s:array(value,"travelTimeSections")) {
        const auto path="travelTimeSections["+std::to_string(network.travelTimeSections.size())+"]";
        // `controlType` from schema 24 (M5.5).
        if(schemaVersion>=24)knownFields(s,{"id","name","start","end","controlType"},path,schemaVersion);
        else knownFields(s,{"id","name","start","end"},path,schemaVersion);
        const auto line=[&](const char* name) {
            const auto& l=member(s,name);
            knownFields(l,{"linkId","station"},path+"."+name,schemaVersion);
            return SectionLine{field<std::string>(l,"linkId"),field<double>(l,"station")};
        };
        network.travelTimeSections.push_back({field<std::string>(s,"id"),present(s,"name")?field<std::string>(s,"name"):"",line("start"),line("end"),{}});
        if(schemaVersion>=24 && present(s,"controlType"))network.travelTimeSections.back().controlType=sectionControlFromName(field<std::string>(s,"controlType"));
    }
    return network;
}
void requireKnownFields(const Json& value,std::initializer_list<const char*> keys,const std::string& path) {
    knownFields(value,keys,path,21);
}
PriorityDefaults parsePriorityDefaults(const Json& value) {
    return {field<double>(value, "gapTime"), field<double>(value, "headway")};
}
// M3.5 (PERCEPTION.md §6, D181): the optional perception keys, read the same way on both models.
// A non-number, or a non-integer where the key is an integer, is refused here; ranges are the
// validator's (perceptionParameterIssues), so an unused behaviour is checked as well.
static void readPerception(const Json& b, const std::string& path, PerceptionParameters& p) {
    for (const auto& key : perceptionParameterKeys()) {
        if (!b.contains(key.name)) continue;
        const auto& value = b.at(key.name);
        if (!value.is_number() || (key.integer && !value.is_number_integer()))
            throw ValidationError({{"INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name}});
        p.*key.member = value.get<double>();
    }
}
// One optional number each (curve D212, kerb D215, commitment D219, mandatory D221, Stop dwell D231, spread D236, red running D238); ranges are the validator's.
static void readOptionals(const Json& b, const std::string& path, DriverBehaviour& d) {
    for (const auto& [key, value] : {std::pair{"curveLateralAcceleration", &d.curveLateralAcceleration}, std::pair{"kerbLaneBias", &d.kerbLaneBias}, std::pair{"commitmentDeceleration", &d.commitmentDeceleration}, std::pair{"mandatoryDecelerationOwn", &d.mandatoryDecelerationOwn}, std::pair{"mandatoryDecelerationTrailingVehicle", &d.mandatoryDecelerationTrailingVehicle}, std::pair{"stopDwellTime", &d.stopDwellTime}, std::pair{"driverFactorSpread", &d.driverFactorSpread}, std::pair{"redRunningShare", &d.redRunningShare}, std::pair{"redRunningTime", &d.redRunningTime}})
        if (b.contains(key) && !b.at(key).is_number()) throw ValidationError({{"INVALID_BEHAVIOUR_PARAMETER", path + "." + key}});
        else if (b.contains(key)) *value = b.at(key).get<double>();
}
// M3.7 (LATERAL.md §9, D187): the same for the lateral keys; completeness and ranges are the validator's.
static void readLateral(const Json& b, const std::string& path, LateralParameters& p) {
    for (const auto& key : lateralParameterKeys()) {
        if (!b.contains(key.name)) continue;
        if (!b.at(key.name).is_number()) throw ValidationError({{"INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name}});
        p.*key.member = b.at(key.name).get<double>();
    }
}
DriverBehaviour parseBehaviour(const Json& b, const std::string& path) {
    static constexpr const char* prototypeKeys[] = {"standstillDistance", "additiveSafetyDistance",
        "multiplicativeSafetyDistance", "followingTime", "speedThreshold", "discretionaryLaneChangeThreshold",
        "acceptedDecelerationTrailingVehicle", "discretionaryLaneChangeHoldTime"};
    std::string model = kPrototypeBehaviourModel;
    if (b.is_object() && b.contains("model")) {
        if (!b.at("model").is_string()) throw ValidationError({{"UNSUPPORTED_BEHAVIOUR_MODEL", path + ".model"}});
        model = b.at("model").get<std::string>();
    }
    if (model == kW74BehaviourModel) {
        for (const char* key : prototypeKeys)
            if (b.contains(key)) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path + "." + key}});
        DriverBehaviour behaviour{field<std::string>(b, "id")};
        W74Parameters parameters{};
        for (const auto& key : w74ParameterKeys()) {
            if (!b.contains(key.name) || !b.at(key.name).is_number())
                throw ValidationError({{"INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name}});
            parameters.*key.member = b.at(key.name).get<double>();
        }
        behaviour.w74 = parameters;
        if (b.contains("maxDecelerationCooperativeBraking"))
            behaviour.maxDecelerationCooperativeBraking = field<double>(b, "maxDecelerationCooperativeBraking");
        if (b.contains("amberDeceleration")) behaviour.amberDeceleration = field<double>(b, "amberDeceleration"); // M4.2
        readPerception(b, path, behaviour.perception); readLateral(b, path, behaviour.lateral);
        readOptionals(b, path, behaviour);
        return behaviour;
    }
    if (model != kPrototypeBehaviourModel) throw ValidationError({{"UNSUPPORTED_BEHAVIOUR_MODEL", path + ".model"}});
    for (const auto& key : w74ParameterKeys())
        if (b.contains(key.name)) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path + "." + key.name}});
    DriverBehaviour behaviour{field<std::string>(b, "id"), field<double>(b, "standstillDistance"),
        field<double>(b, "additiveSafetyDistance"), field<double>(b, "multiplicativeSafetyDistance"),
        field<double>(b, "followingTime"), field<double>(b, "speedThreshold"), std::nullopt};
    // Optional (M3.2.8c): absent, the behaviour does not brake cooperatively.
    if (b.contains("maxDecelerationCooperativeBraking"))
        behaviour.maxDecelerationCooperativeBraking = field<double>(b, "maxDecelerationCooperativeBraking");
    // Optional (D95): absent, the behaviour makes no discretionary lane change.
    if (b.contains("discretionaryLaneChangeThreshold"))
        behaviour.discretionaryLaneChangeThreshold = field<double>(b, "discretionaryLaneChangeThreshold");
    if (b.contains("acceptedDecelerationTrailingVehicle"))
        behaviour.acceptedDecelerationTrailingVehicle = field<double>(b, "acceptedDecelerationTrailingVehicle");
    // Optional (D101): absent, a discretionary change is never held.
    if (b.contains("discretionaryLaneChangeHoldTime"))
        behaviour.discretionaryLaneChangeHoldTime = field<double>(b, "discretionaryLaneChangeHoldTime");
    // Optional (M4.2, D147): absent, amber holds like red.
    if (b.contains("amberDeceleration")) behaviour.amberDeceleration = field<double>(b, "amberDeceleration");
    readPerception(b, path, behaviour.perception); readLateral(b, path, behaviour.lateral);
    readOptionals(b, path, behaviour);
    return behaviour;
}
VehicleType parseVehicleType(const Json& t) {
    const auto& range = member(t, "desiredSpeed");
    VehicleType type{field<std::string>(t, "id"), field<double>(t, "length"), field<double>(t, "width"),
        parseSpeedRange(range), field<double>(t, "maxAcceleration"),
        field<double>(t, "comfortableDeceleration"), field<double>(t, "maxDeceleration"), field<std::string>(t, "behaviourId")};
    if(t.contains("axles")) {
        const auto& a=member(t,"axles");
        knownFields(a,{"wheelbase","frontOverhang","rearOverhang"},"vehicleType.axles",17);
        type.axles=VehicleAxles{field<double>(a,"wheelbase"),field<double>(a,"frontOverhang"),field<double>(a,"rearOverhang")};
    }
    type.kerbLanes=parseKerbLanes(t); type.pcu=parsePcu(t); return type; // B-7 (D234); S1.2 (D270)
}
Composition parseComposition(const Json& c) {
    knownFields(c,{"id","name","types","intervals"},"composition",19);
    Composition composition{field<std::string>(c, "id"), {}};
    if(c.contains("name"))composition.name=field<std::string>(c,"name");
    for (const auto& t : array(c, "types")) {
        knownFields(t,{"vehicleTypeId","share"},"composition.types",18);
        composition.types.push_back({field<std::string>(t, "vehicleTypeId"), field<double>(t, "share")});
    }
    if(c.contains("intervals"))for(const auto& p:array(c,"intervals")) {
        knownFields(p,{"startTime","endTime","types"},"composition.intervals",19);
        CompositionInterval period{field<double>(p,"startTime"),field<double>(p,"endTime"),{}};
        for(const auto& t:array(p,"types")) {
            knownFields(t,{"vehicleTypeId","share"},"composition.intervals.types",19);
            period.types.push_back({field<std::string>(t,"vehicleTypeId"),field<double>(t,"share")});
        }
        composition.intervals.push_back(std::move(period));
    }
    return composition;
}
std::vector<RoutingDecision> parseRoutingDecisions(const Json& definition) {
    std::vector<RoutingDecision> result;
    for (const auto& x : array(definition, "routingDecisions")) {
        knownFields(x,{"id","name","linkId","routes","intervals","typeRules","position"},"routingDecision",20);
        RoutingDecision decision{field<std::string>(x, "id"),
                                 present(x, "name") ? field<std::string>(x, "name") : std::string{}, {}};
        if (present(x, "linkId")) decision.linkId = field<std::string>(x, "linkId"); // M2.1.1
        if(present(x,"position"))decision.position=field<double>(x,"position");
        for (const auto& r : array(x, "routes")) {
            knownFields(r,{"routeId","relativeFlow","destinationLinkId","intervalFlows"},"routingDecision.routes",19);
            decision.routes.push_back({field<std::string>(r, "routeId"), field<double>(r, "relativeFlow")});
            if (present(r, "destinationLinkId")) decision.routes.back().destinationLinkId = field<std::string>(r, "destinationLinkId");
            if (present(r, "intervalFlows")) for (const auto& f : array(r, "intervalFlows")) { // M2.1.2
                if (!f.is_number()) throw std::invalid_argument("INVALID_SHARE");
                decision.routes.back().intervalFlows.push_back(f.get<double>());
            }
        }
        if (present(x, "intervals")) for (const auto& i : array(x, "intervals")) // M2.1.2
            decision.intervals.push_back({field<double>(i, "startTime"), field<double>(i, "endTime")});
        if(x.contains("typeRules"))for(const auto& rule:array(x,"typeRules")) {
            knownFields(rule,{"vehicleTypeId","relativeFlows","intervalFlows"},"routingDecision.typeRules",19);
            RoutingTypeRule parsed{field<std::string>(rule,"vehicleTypeId"),doubles(rule,"relativeFlows"),{}};
            for(const auto& row:array(rule,"intervalFlows")) {
                if(!row.is_array())throw std::invalid_argument("ROUTING_DECISION_INTERVALS");
                std::vector<double> flows;for(const auto& f:row){if(!f.is_number())throw std::invalid_argument("INVALID_SHARE");flows.push_back(f.get<double>());}
                parsed.intervalFlows.push_back(std::move(flows));
            }
            decision.typeRules.push_back(std::move(parsed));
        }
        result.push_back(std::move(decision));
    }
    return result;
}
ScenarioDefinition parseDefinition(const Json& value) {
    ScenarioDefinition definition;
    definition.duration = field<double>(value, "duration");
    definition.timeStep = field<double>(value, "timeStep");
    for (const auto& r : array(value, "routes"))
        definition.routes.push_back({field<std::string>(r, "id"), strings(r, "segmentIds")});
    for (const auto& i : array(value, "inputs")) {
        VehicleInput input{field<std::string>(i, "id"), field<std::string>(i, "routeId"),
            field<std::string>(i, "vehicleTypeId"), field<double>(i, "vehiclesPerHour"),
            field<double>(i, "startTime"), field<double>(i, "endTime")};
        if (i.contains("laneShares")) input.laneShares = doubles(i, "laneShares");
        if (i.contains("compositionId")) input.compositionId = field<std::string>(i, "compositionId");
        if (i.contains("routingDecisionId")) input.routingDecisionId = field<std::string>(i, "routingDecisionId");
        if (i.contains("linkId")) input.linkId = field<std::string>(i, "linkId"); // M2.1.1
        if (i.contains("volumeFromCounts")) input.volumeFromCounts = field<bool>(i, "volumeFromCounts"); // D142
        if (i.contains("intervals")) {
            for (const auto& p : array(i, "intervals"))
                input.intervals.push_back({field<double>(p, "startTime"), field<double>(p, "endTime"),
                                           field<double>(p, "vehiclesPerHour")});
            // The file's scalars are a convenience copy; the intervals are the source, so a
            // hand edit that makes them disagree cannot create a second truth (hard rule 3).
            deriveInputTotals(input);
        }
        definition.inputs.push_back(std::move(input));
    }
    definition.signalPrograms=networkCodec::readSignalPrograms(value);
    if (value.contains("vehicleTypes"))
        for (const auto& t : array(value, "vehicleTypes")) definition.vehicleTypes.push_back(parseVehicleType(t));
    if (value.contains("behaviours"))
        for (const auto& b : array(value, "behaviours"))
            definition.behaviours.push_back(parseBehaviour(b, "behaviours[" + std::to_string(definition.behaviours.size()) + "]"));
    return definition;
}
}
