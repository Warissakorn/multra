#include "network_fields.hpp"
#include "../core/validate.hpp"
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
    {"rightOfWay",Kind::unchecked,false,14},{"queueCounters",Kind::unchecked,false,16},
    {"travelTimeSections",Kind::unchecked,false,23},{"nodes",Kind::unchecked,false,48},
    {"roads",Kind::unchecked,false,roadVersion,roadVersion},
    {"overpasses",Kind::unchecked,false,roadVersion,roadVersion}};
constexpr Field linkFields[]={
    {"id",Kind::text,true},{"geometry",Kind::geometry,true},{"lanes",Kind::lanes,true},
    {"level",Kind::level},{"displayType",Kind::text},{"laneOffset",Kind::number},
    {"name",Kind::text,false,0,0,true},{"boundaryMarkings",Kind::markings},
    {"behaviourType",Kind::text,false,behaviourVersion},
    // Known to parseNetwork from 21, but document prechecks reject it before 50.
    {"linkType",Kind::text,false,behaviourVersion,linkTypeVersion},
    {"roadId",Kind::text,false,roadVersion,roadVersion}};
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
    }
    return {};
}
void knownFields(const Json& j,Object object,const std::string& path,int version) {
    if(version<strictVersion || !j.is_object())return;
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
    case Kind::text:case Kind::side:if(!v.is_string())expected="Expected text: ";break;
    case Kind::number:if(!v.is_number())expected="Expected number: ";break;
    case Kind::level:
        if(!v.is_number_integer() || v < -levelLimit || v > levelLimit)throw std::invalid_argument("EDIT_DISPLAY_VALUE");
        break;
    case Kind::group:
        if(!v.is_number()) {expected="Expected number: ";break;}
        if(const auto n=v.get<double>();!std::isfinite(n) || n<groupMinimum || n>groupMaximum || n!=std::trunc(n))
            throw std::invalid_argument("DUPLICATE_SIGNAL_GROUP");
        break;
    case Kind::array:case Kind::links:case Kind::lanes:case Kind::geometry:case Kind::markings:case Kind::connectors:case Kind::numbers:case Kind::signalHeads:
        if(!v.is_array())expected="Expected array: ";
        break;
    case Kind::unchecked:case Kind::reference:case Kind::generated:break;
    }
    if(expected)throw std::invalid_argument(std::string(expected)+name);
    return v;
}
}
