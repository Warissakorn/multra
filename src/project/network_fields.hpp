#pragma once
#include "json.hpp"
#include <span>
#include <limits>
#include <utility>

namespace multra::networkCodec {
enum class Object { network,link,lane,point,connector,reference,signalHead,signalController,signalGroup,signalProgram,signalPhase,definition };
enum class Kind { text,number,level,side,array,links,lanes,geometry,markings,unchecked,connectors,reference,numbers,generated,signalHeads,group,groups,phases,color,controllers,programs };
struct Field {
    const char* name;Kind kind;bool required{};int knownFrom{};
    int rejectBefore{};bool nullable{};int rejectFrom{};
    const char* whenNonNull{};
};
inline constexpr int strictVersion=7,behaviourVersion=21,linkTypeVersion=50,roadVersion=53;
inline constexpr int levelLimit=1000;
inline constexpr int groupMinimum=std::numeric_limits<int>::min(),groupMaximum=std::numeric_limits<int>::max();
inline constexpr int stationVersion=5,laneSideVersion=17,generatedVersion=54;
inline const char* positionKey(int version) {return version<stationVersion?"fraction":"station";}
std::span<const Field> fields(Object);
bool allowsAdditionalFields(Object,int version);
inline constexpr std::pair<const char*,SignalColor> signalColors[]={
    {"red",SignalColor::red},{"amber",SignalColor::amber},{"green",SignalColor::green}};
std::vector<SignalProgram> readSignalPrograms(const Json& definition);
// Same legacy/additive known-key policy and diagnostics as parseNetwork.
void knownFields(const Json&,Object,const std::string& path,int version);
const Json& value(const Json&,Object,const char* name);
bool readCondition(const Json&,Object,const char* name);
// Link-only decoder: leaves linkType/roadId to their existing document read stages.
std::vector<Link> readLinks(const Json& network,int version);
std::vector<Point> readGeometry(const Json&,Object,int version);
LaneReference readReference(const Json&,int version);
std::vector<Connector> readConnectors(const Json& network,int version);
std::vector<NetworkSignalHead> readSignalHeads(const Json& network,int version);
}
