#pragma once
#include "json.hpp"
#include <span>
#include <limits>
#include <utility>
#include <string_view>

namespace multra::networkCodec {
enum class Object { network,link,lane,point,connector,reference,signalHead,signalController,signalGroup,signalProgram,signalPhase,definition,
    rightOfWay,waitingLine,controlPoint,controlPath,conflictArea,conflictSide,priorityRule,stopControl,queueCounter,measurementLine,travelTimeSection,sectionLine,route,vehicleInput,volumeInterval,
    routingDecision,routingRoute,routingInterval,routingTypeRule,composition,compositionShare,compositionInterval,mode,
    vehicleType,speedRange,speedPoint,axles,pcu,behaviour,w74Behaviour };
enum class Kind { text,number,level,side,array,links,lanes,geometry,markings,unchecked,connectors,reference,numbers,generated,signalHeads,group,groups,phases,color,controllers,programs,
    rightOfWay,waitingLines,controlPoint,controlPath,conflictAreas,conflictSide,priorityRules,stopControls,
    conflictKind,conflictPriority,stopMode,texts,queueCounters,measurementLines,travelTimeSections,sectionLine,sectionControl,boolean,routes,inputs,volumeIntervals,
    routingDecisions,routingRoutes,routingIntervals,routingTypeRules,numberMatrix,
    compositions,compositionShares,compositionIntervals,modes,vehicleTypes,speedRange,speedCurve,axles,kerbLanes,pcu,
    behaviours,behaviourModel,w74Model,integerToken };
struct Field {
    const char* name;Kind kind;bool required{};int knownFrom{};
    int rejectBefore{};bool nullable{};int rejectFrom{};
    const char* whenNonNull{};
};
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
