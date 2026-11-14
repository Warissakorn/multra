#include "network_fields.hpp"
#include "../core/validate.hpp"
#include "../core/w74.hpp"
#include "../core/perception.hpp"
#include "../core/lateral.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace multra::networkCodec {
namespace {
constexpr Field networkFields[]={
    {"id",Kind::text,true},{"drivingSide",Kind::side,true},{"links",Kind::links,true},
    {"connectors",Kind::connectors,true},{"signalHeads",Kind::signalHeads,true},
    {"rightOfWay",Kind::rightOfWay,false,14},{"queueCounters",Kind::queueCounters,false,16,0,true},
    {"travelTimeSections",Kind::travelTimeSections,false,23,0,true},{"nodes",Kind::unchecked,false,48},
    {"roads",Kind::unchecked,false,streetSegmentVersion,streetSegmentVersion,false,streetSegmentKeyVersion},
    {"streetSegments",Kind::unchecked,false,streetSegmentKeyVersion,streetSegmentKeyVersion},
    {"overpasses",Kind::unchecked,false,streetSegmentVersion,streetSegmentVersion}};
constexpr Field linkFields[]={
    {"id",Kind::text,true},{"geometry",Kind::geometry,true},{"lanes",Kind::lanes,true},
    {"level",Kind::level},{"displayType",Kind::text},{"laneOffset",Kind::number},
    {"name",Kind::text,false,0,0,true},{"boundaryMarkings",Kind::markings},
    {"behaviourType",Kind::text,false,behaviourVersion},
    // Known to parseNetwork from 21, but document prechecks reject it before 50.
    {"linkType",Kind::text,false,behaviourVersion,linkTypeVersion},
    {"roadId",Kind::text,false,streetSegmentVersion,streetSegmentVersion,false,streetSegmentKeyVersion},
    {"streetSegmentId",Kind::text,false,streetSegmentKeyVersion,streetSegmentKeyVersion},
    {"speedLimit",Kind::number,false,linkSpeedVersion,linkSpeedVersion},
    {"generated",Kind::generated,false,62,62}};
constexpr Field laneFields[]={{"id",Kind::text,true},{"width",Kind::number,true}};
constexpr Field pointFields[]={{"x",Kind::number,true},{"y",Kind::number,true}};
constexpr Field connectorFields[]={
    {"id",Kind::text,true},{"from",Kind::reference,true},{"to",Kind::reference,true},
    {"geometry",Kind::geometry,true},{"fromLaneCount",Kind::level},{"toLaneCount",Kind::level},
    {"level",Kind::level},{"displayType",Kind::text},{"laneBlend",Kind::numbers},
    {"name",Kind::text,false,0,0,true},{"laneWidths",Kind::numbers},{"laneMarkings",Kind::markings},
    {"laneChangeSide",Kind::side,false,laneSideVersion,0,true},
    {"behaviourType",Kind::text,false,behaviourVersion},{"generated",Kind::generated,false,generatedVersion}};
constexpr Field referenceFields[]={
    {"linkId",Kind::text,true},{"laneId",Kind::text,true},
    {"station",Kind::number,false,stationVersion,stationVersion},
    {"fraction",Kind::number,false,0,0,false,stationVersion}};
constexpr Field signalHeadFields[]={
    {"id",Kind::text,true},{"lane",Kind::reference,true},{"position",Kind::number,true},
    {"programId",Kind::text,true},{"connectorId",Kind::text,false,0,0,true},
    {"name",Kind::text,false,0,0,true},{"controllerId",Kind::text,false,0,0,true},
    {"groupNumber",Kind::group,false,0,0,false,0,"controllerId"},
    {"generated",Kind::generated,false,generatedVersion}};
constexpr Field definitionFields[]={
    {"signalPrograms",Kind::programs,true},{"signalControllers",Kind::controllers},
    {"duration",Kind::number,true},{"timeStep",Kind::number,true},
    {"routes",Kind::routes,true},{"inputs",Kind::inputs,true},{"routingDecisions",Kind::routingDecisions},
    {"compositions",Kind::compositions},{"modes",Kind::modes,false,0,modeVersion},
    {"vehicleTypes",Kind::vehicleTypes},{"behaviours",Kind::behaviours}};
constexpr Field controllerFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},
    {"cycle",Kind::number,true},{"offset",Kind::number,true},{"groups",Kind::groups,true},
    {"generated",Kind::generated,false,0,generatedVersion}};
constexpr Field groupFields[]={
    {"number",Kind::group,true},{"name",Kind::text,false,0,0,true},
    {"greenStart",Kind::number,true},{"greenEnd",Kind::number,true},{"amber",Kind::number,true}};
constexpr Field programFields[]={
    {"id",Kind::text,true},{"offset",Kind::number,true},{"phases",Kind::phases,true}};
constexpr Field phaseFields[]={{"duration",Kind::number,true},{"color",Kind::color,true}};
constexpr Field rightOfWayFields[]={
    {"waitingLines",Kind::waitingLines,false,0,0,true},{"conflictAreas",Kind::conflictAreas,false,0,0,true},
    {"priorityRules",Kind::priorityRules,false,0,0,true},{"stopControls",Kind::stopControls,false,15,0,true}};
constexpr Field waitingLineFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"point",Kind::controlPoint,true},
    {"generated",Kind::generated,false,generatedVersion}};
constexpr Field controlPointFields[]={{"path",Kind::controlPath,true},{"station",Kind::number,true}};
constexpr Field controlPathFields[]={
    {"linkId",Kind::text,false,0,0,true},{"laneId",Kind::text,false,0,0,true},
    {"connectorId",Kind::text,false,0,0,true},{"fromLaneId",Kind::text,false,0,0,true},
    {"toLaneId",Kind::text,false,0,0,true}};
constexpr Field conflictAreaFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"kind",Kind::conflictKind,true},
    {"first",Kind::conflictSide,true},{"second",Kind::conflictSide,true},{"priority",Kind::conflictPriority,true},
    {"generated",Kind::generated,false,generatedVersion}};
constexpr Field conflictSideFields[]={
    {"path",Kind::controlPath,true},{"entryStation",Kind::number,true},{"exitStation",Kind::number,true},
    {"waitingLineId",Kind::text,true}};
constexpr Field priorityRuleFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"conflictAreaId",Kind::text,true},
    {"gapTime",Kind::number,true},{"headway",Kind::number,true},{"generated",Kind::generated,false,generatedVersion}};
constexpr Field stopControlFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"waitingLineId",Kind::text,true},
    {"mode",Kind::stopMode,true},{"conflictAreaIds",Kind::texts,true},{"generated",Kind::generated,false,generatedVersion}};
constexpr Field queueCounterFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"lines",Kind::measurementLines,true}};
constexpr Field measurementLineFields[]={
    {"referenceId",Kind::text,false,0,0,true},{"point",Kind::controlPoint,false,0,0,true}};
constexpr Field travelTimeSectionFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"start",Kind::sectionLine,true},
    {"end",Kind::sectionLine,true},{"controlType",Kind::sectionControl,false,24,0,true}};
constexpr Field sectionLineFields[]={{"linkId",Kind::text,true},{"station",Kind::number,true}};
constexpr Field routeFields[]={{"id",Kind::text,true},{"segmentIds",Kind::texts,true}};
constexpr Field vehicleInputFields[]={
    {"id",Kind::text,true},{"routeId",Kind::text,true},{"vehicleTypeId",Kind::text,true},
    {"vehiclesPerHour",Kind::number,true},{"startTime",Kind::number,true},{"endTime",Kind::number,true},
    {"laneShares",Kind::numbers},{"compositionId",Kind::text},{"routingDecisionId",Kind::text},
    {"linkId",Kind::text},{"volumeFromCounts",Kind::boolean,false,0,countedVolumeVersion},
    {"intervals",Kind::volumeIntervals}};
constexpr Field volumeIntervalFields[]={
    {"startTime",Kind::number,true},{"endTime",Kind::number,true},{"vehiclesPerHour",Kind::number,true}};
constexpr Field routingDecisionFields[]={
    {"id",Kind::text,true},{"name",Kind::text,false,0,0,true},{"linkId",Kind::text,false,0,0,true},
    {"routes",Kind::routingRoutes,true},{"intervals",Kind::routingIntervals,false,0,0,true},
    {"typeRules",Kind::routingTypeRules},{"position",Kind::number,false,0,routingPositionVersion,true}};
constexpr Field routingRouteFields[]={
    {"routeId",Kind::text,true},{"relativeFlow",Kind::number,true},
    {"destinationLinkId",Kind::text,false,0,0,true},{"intervalFlows",Kind::numbers,false,0,0,true}};
constexpr Field routingIntervalFields[]={{"startTime",Kind::number,true},{"endTime",Kind::number,true}};
constexpr Field routingTypeRuleFields[]={
    {"vehicleTypeId",Kind::text,true},{"relativeFlows",Kind::numbers,true},{"intervalFlows",Kind::numberMatrix,true}};
// D558: owned compositions; name and intervals are contains-based (explicit null fails).
constexpr Field compositionFields[]={
    {"id",Kind::text,true},{"name",Kind::text},{"types",Kind::compositionShares,true},{"intervals",Kind::compositionIntervals}};
constexpr Field compositionShareFields[]={{"vehicleTypeId",Kind::text,true},{"share",Kind::number,true}};
constexpr Field compositionIntervalFields[]={
    {"startTime",Kind::number,true},{"endTime",Kind::number,true},{"types",Kind::compositionShares,true}};
// D559: exactly these keys, each required text; parseMode counts and checks them.
constexpr Field modeFields[]={{"id",Kind::text,true},{"name",Kind::text,true},{"compositionId",Kind::text,true}};
// D560: open below behaviourVersion, closed from it (parseBehaviourLibrary). name is read by
// parseAuthoringDefinition; kerbLanes, pcu and the curve keep their own readers and codes.
constexpr Field vehicleTypeFields[]={
    {"id",Kind::text,true},{"name",Kind::text},{"length",Kind::number,true},{"width",Kind::number,true},
    {"desiredSpeed",Kind::speedRange,true},{"maxAcceleration",Kind::number,true},
    {"comfortableDeceleration",Kind::number,true},{"maxDeceleration",Kind::number,true},
    {"behaviourId",Kind::text,true},{"axles",Kind::axles},{"kerbLanes",Kind::kerbLanes,false,0,kerbLanesVersion},
    {"pcu",Kind::pcu,false,0,modeVersion}};
constexpr Field speedRangeFields[]={{"min",Kind::number,true},{"max",Kind::number,true},{"curve",Kind::speedCurve,false,0,speedCurveVersion}};
constexpr Field speedPointFields[]={{"speed",Kind::number,true},{"share",Kind::number,true}};
constexpr Field axleFields[]={{"wheelbase",Kind::number,true},{"frontOverhang",Kind::number,true},{"rearOverhang",Kind::number,true}};
constexpr Field pcuFields[]={{"value",Kind::number,true},{"source",Kind::text,true}};
// D563 (PROJECT_SCHEMA_DEMAND, behaviour inventory): the two model rows. The parameter keys come
// from core's lists; a key of the other model is `false` at every version (rejectFrom 1).
constexpr const char* prototypeRequired[]={"standstillDistance","additiveSafetyDistance",
    "multiplicativeSafetyDistance","followingTime","speedThreshold"};
constexpr const char* prototypeOptional[]={"discretionaryLaneChangeThreshold","acceptedDecelerationTrailingVehicle",
    "discretionaryLaneChangeHoldTime"};
using B=DriverBehaviour;
const BehaviourNumber sharedOptional[]={{"maxDecelerationCooperativeBraking",&B::maxDecelerationCooperativeBraking,0,false},
    {"amberDeceleration",&B::amberDeceleration,amberVersion,false},
    {"curveLateralAcceleration",&B::curveLateralAcceleration,curveLateralVersion,true},
    {"kerbLaneBias",&B::kerbLaneBias,kerbBiasVersion,true},{"commitmentDeceleration",&B::commitmentDeceleration,commitmentVersion,true},
    {"mandatoryDecelerationOwn",&B::mandatoryDecelerationOwn,mandatoryVersion,true},
    {"mandatoryDecelerationTrailingVehicle",&B::mandatoryDecelerationTrailingVehicle,mandatoryVersion,true},
    {"stopDwellTime",&B::stopDwellTime,stopDwellVersion,true},{"driverFactorSpread",&B::driverFactorSpread,driverFactorSpreadVersion,true},
    {"redRunningShare",&B::redRunningShare,redRunningVersion,true},{"redRunningTime",&B::redRunningTime,redRunningVersion,true}};
std::vector<Field> behaviourRow(bool w74) {
    const int notThis=w74?1:0,notOther=w74?0:1;
    std::vector<Field> row{{"id",Kind::text,true},{"name",Kind::text,false,0,behaviourVersion},
        {"model",w74?Kind::w74Model:Kind::behaviourModel,true,0,behaviourVersion}};
    for(const auto* key:prototypeRequired)row.push_back({key,Kind::number,!w74,0,0,false,notThis});
    for(const auto* key:prototypeOptional)row.push_back({key,Kind::number,false,0,0,false,notThis});
    for(const auto& key:w74ParameterKeys())row.push_back({key.name,Kind::number,w74,0,0,false,notOther});
    for(const auto& key:sharedOptional)row.push_back({key.name,Kind::number,false,0,key.floor});
    for(const auto& key:perceptionParameterKeys())row.push_back({key.name,key.integer?Kind::integerToken:Kind::number,false,0,perceptionVersion});
    for(const auto& key:lateralParameterKeys())row.push_back({key.name,Kind::number,false,0,lateralVersion});
    return row;
}
}
std::span<const Field> fields(Object object) {
    switch(object) {
    case Object::network:return networkFields;
    case Object::link:return linkFields;
    case Object::lane:return laneFields;
    case Object::point:return pointFields;
    case Object::connector:return connectorFields;
    case Object::reference:return referenceFields;
    case Object::signalHead:return signalHeadFields;
    case Object::definition:return definitionFields;
    case Object::signalController:return controllerFields;
    case Object::signalGroup:return groupFields;
    case Object::signalProgram:return programFields;
    case Object::signalPhase:return phaseFields;
    case Object::rightOfWay:return rightOfWayFields;
    case Object::waitingLine:return waitingLineFields;
    case Object::controlPoint:return controlPointFields;
    case Object::controlPath:return controlPathFields;
    case Object::conflictArea:return conflictAreaFields;
    case Object::conflictSide:return conflictSideFields;
    case Object::priorityRule:return priorityRuleFields;
    case Object::stopControl:return stopControlFields;
    case Object::queueCounter:return queueCounterFields;
    case Object::measurementLine:return measurementLineFields;
    case Object::travelTimeSection:return travelTimeSectionFields;
    case Object::sectionLine:return sectionLineFields;
    case Object::route:return routeFields;
    case Object::vehicleInput:return vehicleInputFields;
    case Object::volumeInterval:return volumeIntervalFields;
    case Object::routingDecision:return routingDecisionFields;
    case Object::routingRoute:return routingRouteFields;
    case Object::routingInterval:return routingIntervalFields;
    case Object::routingTypeRule:return routingTypeRuleFields;
    case Object::composition:return compositionFields;
    case Object::compositionShare:return compositionShareFields;
    case Object::compositionInterval:return compositionIntervalFields;
    case Object::mode:return modeFields;
    case Object::vehicleType:return vehicleTypeFields;
    case Object::speedRange:return speedRangeFields;
    case Object::speedPoint:return speedPointFields;
    case Object::axles:return axleFields;
    case Object::pcu:return pcuFields;
    case Object::behaviour:{static const auto row=behaviourRow(false);return row;}
    case Object::w74Behaviour:{static const auto row=behaviourRow(true);return row;}
    }
    return {};
}
std::span<const BehaviourNumber> behaviourOptionalNumbers() {return sharedOptional;}
bool behaviourKey(std::string_view key) {
    const auto row=fields(Object::behaviour); // lists both models' keys, the other's as rejected
    return std::any_of(row.begin(),row.end(),[&](const auto& f){return key==f.name;});
}
bool allowsAdditionalFields(Object object,int version) {
    if(object==Object::signalController || object==Object::signalGroup || object==Object::routingDecision ||
       object==Object::routingRoute || object==Object::routingTypeRule || object==Object::composition ||
       object==Object::compositionShare || object==Object::compositionInterval || object==Object::mode ||
       object==Object::axles || object==Object::pcu)return false;
    // D562 (owner, OWNER_SITTING 49b): desiredSpeed and its curve points close with the type row.
    if(object==Object::vehicleType || object==Object::behaviour || object==Object::w74Behaviour || object==Object::speedRange || object==Object::speedPoint)return version<behaviourVersion;
    if(object==Object::route || object==Object::vehicleInput || object==Object::volumeInterval || object==Object::routingInterval)return true;
    if(object==Object::signalProgram || object==Object::signalPhase || object==Object::definition)return true;
    return version<strictVersion;
}
void knownFields(const Json& j,Object object,const std::string& path,int version) {
    if(allowsAdditionalFields(object,version) || !j.is_object())return;
    const auto list=fields(object);
    for(const auto& [key,unused]:j.items()) {
        (void)unused;
        if(std::none_of(list.begin(),list.end(),[&](const auto& f){
            return key==f.name && version>=f.knownFrom && (!f.rejectFrom || version<f.rejectFrom);
        }))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD",path+"."+key}});
    }
}
bool readCondition(const Json& j,Object object,const char* name) {
    for(const auto& field:fields(object))if(std::string_view(name)==field.name)
        return !field.whenNonNull || present(j,field.whenNonNull);
    throw std::logic_error("Unknown network codec field");
}
const Json& value(const Json& j,Object object,const char* name) {
    if(!j.is_object())throw std::invalid_argument(std::string("Expected an object containing: ")+name);
    if(!j.contains(name))throw std::invalid_argument(std::string("Missing field: ")+name);
    const auto& v=j.at(name);
    if(v.is_null())throw std::invalid_argument(std::string("Field is null: ")+name);
    const auto list=fields(object);
    const auto found=std::find_if(list.begin(),list.end(),[&](const auto& f){return std::string_view(name)==f.name;});
    if(found==list.end())throw std::logic_error("Unknown network codec field");
    const char* expected=nullptr;
    switch(found->kind) {
    case Kind::text:case Kind::behaviourModel:case Kind::w74Model:case Kind::side:case Kind::color:case Kind::conflictKind:case Kind::conflictPriority:case Kind::stopMode:case Kind::sectionControl:if(!v.is_string())expected="Expected text: ";break;
    case Kind::boolean:if(!v.is_boolean())expected="Expected true or false: ";break;
    case Kind::number:if(!v.is_number())expected="Expected number: ";break;
    case Kind::level:
        if(!v.is_number_integer() || v < -levelLimit || v > levelLimit)throw std::invalid_argument("EDIT_DISPLAY_VALUE");
        break;
    case Kind::group:
        if(!v.is_number()) {expected="Expected number: ";break;}
        if(const auto n=v.get<double>();!std::isfinite(n) || n<groupMinimum || n>groupMaximum || n!=std::trunc(n))
            throw std::invalid_argument("DUPLICATE_SIGNAL_GROUP");
        break;
    case Kind::array:case Kind::links:case Kind::lanes:case Kind::geometry:case Kind::markings:case Kind::connectors:case Kind::numbers:case Kind::signalHeads:case Kind::groups:case Kind::phases:case Kind::controllers:case Kind::programs:
    case Kind::waitingLines:case Kind::conflictAreas:case Kind::priorityRules:case Kind::stopControls:case Kind::texts:
    case Kind::queueCounters:case Kind::measurementLines:case Kind::travelTimeSections:
    case Kind::routes:case Kind::inputs:case Kind::volumeIntervals:
    case Kind::routingDecisions:case Kind::routingRoutes:case Kind::routingIntervals:case Kind::routingTypeRules:case Kind::numberMatrix:
    case Kind::compositions:case Kind::compositionShares:case Kind::compositionIntervals:case Kind::modes:
    case Kind::vehicleTypes:case Kind::speedCurve:case Kind::behaviours:
        if(!v.is_array())expected="Expected array: ";
        break;
    case Kind::unchecked:case Kind::reference:case Kind::generated:
    case Kind::speedRange:case Kind::axles:case Kind::kerbLanes:case Kind::pcu:case Kind::integerToken:
    case Kind::rightOfWay:case Kind::controlPoint:case Kind::controlPath:case Kind::conflictSide:case Kind::sectionLine:break;
    }
    if(expected)throw std::invalid_argument(std::string(expected)+name);
    return v;
}
}
