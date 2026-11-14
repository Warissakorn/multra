#pragma once
#include "json.hpp"
#include <span>
#include <limits>
#include <utility>
#include <string_view>
#include <optional>

namespace multra::networkCodec {
enum class Object { network,link,lane,point,connector,reference,signalHead,signalController,signalGroup,signalProgram,signalPhase,definition,
    rightOfWay,waitingLine,controlPoint,controlPath,conflictArea,conflictSide,priorityRule,stopControl,queueCounter,measurementLine,travelTimeSection,sectionLine,route,vehicleInput,volumeInterval,
    routingDecision,routingRoute,routingInterval,routingTypeRule,composition,compositionShare,compositionInterval,mode,
    vehicleType,speedRange,speedPoint,axles,pcu,behaviour,w74Behaviour,vehicleClass,linkBehaviourType,behaviourOverride,
    rootFeatures,zone,zoneLinkConnector,zoneStreetConnector,odMatrix,parkingLot,studyPeriod,studyArea,gateFlow,map,mapReference,macroRun,macroSettings };
enum class Kind { text,number,level,side,array,links,lanes,geometry,markings,unchecked,connectors,reference,numbers,generated,signalHeads,group,groups,phases,color,controllers,programs,
    rightOfWay,waitingLines,controlPoint,controlPath,conflictAreas,conflictSide,priorityRules,stopControls,
    conflictKind,conflictPriority,stopMode,texts,queueCounters,measurementLines,travelTimeSections,sectionLine,sectionControl,boolean,routes,inputs,volumeIntervals,
    routingDecisions,routingRoutes,routingIntervals,routingTypeRules,numberMatrix,
    compositions,compositionShares,compositionIntervals,modes,vehicleTypes,speedRange,speedCurve,axles,kerbLanes,pcu,
    behaviours,behaviourModel,w74Model,integerToken,vehicleClasses,linkBehaviourTypes,behaviourOverrides,
    zones,zoneConnectors,odMatrices,parkingLots,pairs,numberMap,zoneDirection,travelDirection,segmentId,unsignedCount,generatedBy,
    studyPeriods,studyArea,nodeIds,flowSource,gateFlows,
    map,mapReference,providerId,opacity,utmCrs,longitude,latitude,worldAnchor,rotation,
    macroRuns,macroSettings,targetGap,iterationCap };
struct Field {
    const char* name;Kind kind;bool required{};int knownFrom{};
    int rejectBefore{};bool nullable{};int rejectFrom{};
    const char* whenNonNull{};
    // D566: required unless this key is present, and refused when it is.
    const char* unless{};
};
inline bool activeField(const Field& f,int version) {
    return version>=f.knownFrom && version>=f.rejectBefore && !(f.rejectFrom && version>=f.rejectFrom);
}
inline constexpr int strictVersion=7,behaviourVersion=21,linkTypeVersion=50,streetSegmentVersion=53,streetSegmentKeyVersion=59,linkSpeedVersion=60;
// D389: schema 59 renamed the Road keys; a file below it keeps its own names, which reads and paths use.
inline const char* streetSegmentsKey(int version) {return version<streetSegmentKeyVersion?"roads":"streetSegments";}
inline const char* streetSegmentIdKey(int version) {return version<streetSegmentKeyVersion?"roadId":"streetSegmentId";}
inline const char* streetSegmentDirectionKey(int version) {return version<streetSegmentKeyVersion?"roadDirection":"streetSegmentDirection";}
inline constexpr int levelLimit=1000,countedVolumeVersion=26,routingPositionVersion=20;
// S1.2 (D269, D559): owned definition.modes is refused below it, even null (rejectModesBefore49).
inline constexpr int modeVersion=49;
// D560: a vehicle type's desiredSpeed.curve (B-6) and kerbLanes (B-7) are refused below these; pcu uses modeVersion.
inline constexpr int speedCurveVersion=40,kerbLanesVersion=42;
// D563: an owned W74 behaviour is refused below it (rejectW74Before25).
inline constexpr int w74ModelVersion=25;
// D564 (B2): a behaviour key is refused below its floor, even null (the reject* guards in behaviour_library.cpp).
inline constexpr int amberVersion=28,perceptionVersion=32,lateralVersion=33,curveLateralVersion=34,kerbBiasVersion=35,
    commitmentVersion=36,mandatoryVersion=37,stopDwellVersion=41,driverFactorSpreadVersion=43,redRunningVersion=44;
// The optional numbers both behaviour models read, in read order. `parameter` keys report
// INVALID_BEHAVIOUR_PARAMETER (readOptionals); the others the field sentences.
struct BehaviourNumber {const char* name;std::optional<double> DriverBehaviour::* member;int floor;bool parameter;};
std::span<const BehaviourNumber> behaviourOptionalNumbers();
// D566: the root zone layer is refused below 31, even null; street-segment connectors from 57; generatedBy from 70.
inline constexpr int zoneLayerVersion=31,streetZoneConnectorVersion=57,generatedMatrixVersion=70;
inline constexpr const char* kGeneratedByModeChoice="mode-choice";
// D567: root studyPeriods and definition.clockStart use modeVersion (49, S1.1); the root studyArea is refused below 55.
inline constexpr int studyAreaVersion=55;
// D568: the root map (M1.21) is refused below 29.
inline constexpr int mapVersion=29;
// D569: definition.macroSettings (S2.3) from 51 and the root macroRuns from 56; neither row has versioned keys.
inline constexpr int macroSettingsVersion=51,macroRunVersion=56;
inline constexpr int groupMinimum=std::numeric_limits<int>::min(),groupMaximum=std::numeric_limits<int>::max();
inline constexpr int stationVersion=5,laneSideVersion=17,generatedVersion=54;
inline const char* positionKey(int version) {return version<stationVersion?"fraction":"station";}
std::span<const Field> fields(Object);
// D563: every key either behaviour model may carry (the from-21 closure in parseBehaviourLibrary).
bool behaviourKey(std::string_view key);
bool allowsAdditionalFields(Object,int version);
inline constexpr std::pair<const char*,SignalColor> signalColors[]={
    {"red",SignalColor::red},{"amber",SignalColor::amber},{"green",SignalColor::green}};
std::vector<SignalProgram> readSignalPrograms(const Json& definition);
// Same legacy/additive known-key policy and diagnostics as parseNetwork.
void knownFields(const Json&,Object,const std::string& path,int version);
// D566: a closed row holds exactly its active keys: required ones present, `unless`-excluded ones absent.
bool exactFields(const Json&,Object,int version);
// D569: whether a key is one of the row's active fields at this version.
bool knownKey(std::string_view key,Object,int version);
const Json& value(const Json&,Object,const char* name);
bool readCondition(const Json&,Object,const char* name);
// Link-only decoder: leaves linkType/streetSegmentId to their existing document read stages.
std::vector<Link> readLinks(const Json& network,int version);
std::vector<Point> readGeometry(const Json&,Object,int version);
LaneReference readReference(const Json&,int version);
std::vector<Connector> readConnectors(const Json& network,int version);
std::vector<NetworkSignalHead> readSignalHeads(const Json& network,int version);
RightOfWay readRightOfWay(const Json&,int version);
// The existing optional-text decoder also serves queue-counter points.
ControlPathRef readControlPath(const Json&,const std::string& path,int version);
std::vector<AuthoredQueueCounter> readQueueCounters(const Json& network,int version);
std::vector<TravelTimeSection> readTravelTimeSections(const Json& network,int version);
// Bare scenarios and authoring projects share these reads; file guards stay in parseDocument.
std::vector<Route> readRoutes(const Json& definition);
std::vector<VehicleInput> readVehicleInputs(const Json& definition);
}
