#include "network_fields.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace multra::networkCodec {
namespace {
constexpr Field networkFields[]={
    {"id",Kind::text,true},{"drivingSide",Kind::side,true},{"links",Kind::links,true},
    {"connectors",Kind::array,true},{"signalHeads",Kind::array,true},
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
}
std::span<const Field> fields(Object object) {
    switch(object) {
    case Object::network:return networkFields;
    case Object::link:return linkFields;
    case Object::lane:return laneFields;
    case Object::point:return pointFields;
    }
    return {};
}
void knownFields(const Json& j,Object object,const std::string& path,int version) {
    if(version<strictVersion || !j.is_object())return;
    const auto list=fields(object);
    for(const auto& [key,unused]:j.items()) {
        (void)unused;
        if(std::none_of(list.begin(),list.end(),[&](const auto& f){return key==f.name && version>=f.knownFrom;}))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD",path+"."+key}});
    }
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
    case Kind::array:case Kind::links:case Kind::lanes:case Kind::geometry:case Kind::markings:
        if(!v.is_array())expected="Expected array: ";
        break;
    case Kind::unchecked:break;
    }
    if(expected)throw std::invalid_argument(std::string(expected)+name);
    return v;
}
}
