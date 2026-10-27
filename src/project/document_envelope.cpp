#include "document_envelope.hpp"
#include "document.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>

namespace multra {
namespace {
constexpr auto counterMaximum=std::numeric_limits<std::uint64_t>::max()-1;
constexpr std::size_t imageByteLimit=32*1024*1024;
constexpr const char* formatMarker="TrafficSim";
enum class Kind { integer,string,unsignedInteger,object };
struct RootField {const char* name;Kind kind;const char* error;};
constexpr RootField rootFields[]={
    {"schemaVersion",Kind::integer,"EDIT_VERSION"},{"format",Kind::string,"EDIT_VERSION"},
    {"nextId",Kind::unsignedInteger,"EDIT_ID_LIMIT"},{"revision",Kind::unsignedInteger,"EDIT_ID_LIMIT"},
    {"background",Kind::object,"EDIT_BACKGROUND_INVALID"}};
struct Counter {const char* name;std::uint64_t ProjectDocument::*member;std::uint64_t minimum;};
constexpr Counter counters[]={{"nextId",&ProjectDocument::nextId,1},{"revision",&ProjectDocument::revision,0}};
struct BackgroundNumber {
    const char* name;double BackgroundImage::*member;
    std::optional<double> minimum{},maximum{};bool exclusiveMinimum{};
};
constexpr BackgroundNumber backgroundNumbers[]={
    {"x",&BackgroundImage::x},{"y",&BackgroundImage::y},
    {"metresPerPixel",&BackgroundImage::metresPerPixel,0,{},true},
    {"rotation",&BackgroundImage::rotation},{"opacity",&BackgroundImage::opacity,0,1}};
bool shape(const Json& value,Kind kind) {
    switch(kind) {
    case Kind::integer:return value.is_number_integer();
    case Kind::unsignedInteger:return value.is_number_unsigned();
    case Kind::string:return value.is_string();
    case Kind::object:return value.is_object();
    }
    return false;
}
const char* type(Kind kind) {
    switch(kind) {
    case Kind::integer:case Kind::unsignedInteger:return "integer";
    case Kind::string:return "string";
    case Kind::object:return "object";
    }
    return "object";
}
}
Json projectEnvelopeSchema() {
    Json properties=Json::object(),required=Json::array();
    for(const auto& field:rootFields) {
        properties[field.name]={{"type",type(field.kind)}};
        required.push_back(field.name);
    }
    properties["format"]["const"]=formatMarker;
    properties["schemaVersion"]["minimum"]=1;
    properties["schemaVersion"]["maximum"]=kLatestSchema;
    properties["schemaVersion"]["x-multra-nativeType"]="integer-token";
    for(const auto& field:counters) {
        properties[field.name]["minimum"]=field.minimum;
        properties[field.name]["maximum"]=counterMaximum;
        properties[field.name]["x-multra-nativeType"]="unsigned-integer-token";
    }
    Json background={{"type","object"},{"additionalProperties",true},
        {"properties",{{"pngBase64",{{"type","string"},{"maxLength",imageByteLimit},
            {"x-multra-maxUtf8Bytes",imageByteLimit}}}}},{"required",Json::array({"pngBase64"})}};
    for(const auto& field:backgroundNumbers) {
        Json rule={{"type","number"}};
        if(field.minimum)rule[field.exclusiveMinimum?"exclusiveMinimum":"minimum"]=*field.minimum;
        if(field.maximum)rule["maximum"]=*field.maximum;
        background["properties"][field.name]=std::move(rule);
        background["required"].push_back(field.name);
    }
    properties["background"]=std::move(background);
    properties["network"]={{"type","object"}};required.push_back("network");
    properties["definition"]={{"type",Json::array({"object","null"})}};
    return {{"$schema","https://json-schema.org/draft/2020-12/schema"},
        {"$comment","Partial envelope/background contract. Nested network, demand and optional feature fields require the codec. Native token types and UTF-8 byte limits also require the codec."},
        {"x-multra-coverage","envelope-background"},{"x-multra-complete",false},
        {"x-multra-validationCommand","document.validate"},
        {"type","object"},{"properties",properties},{"required",required},{"additionalProperties",true}};
}
void readProjectEnvelope(const Json& j,ProjectDocument& d) {
    for(const auto& field:rootFields) {
        if(!j.contains(field.name) || !shape(j.at(field.name),field.kind))throw std::invalid_argument(field.error);
        if(field.kind==Kind::integer && (j.at(field.name)<1 || j.at(field.name)>kLatestSchema))throw std::invalid_argument(field.error);
        if(field.kind==Kind::string && j.at(field.name)!=formatMarker)throw std::invalid_argument(field.error);
    }
    for(const auto& field:counters)d.*(field.member)=j.at(field.name).get<std::uint64_t>();
    const auto& b=j.at("background");
    if(!b.contains("pngBase64") || !b.at("pngBase64").is_string())throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
    for(const auto& field:backgroundNumbers) {
        if(!b.contains(field.name) || !b.at(field.name).is_number())throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        d.background.*(field.member)=b.at(field.name).get<double>();
    }
    d.background.pngBase64=std::make_shared<const std::string>(b.at("pngBase64").get<std::string>());
}
void validateProjectEnvelope(const ProjectDocument& d) {
    for(const auto& field:counters) {
        const auto value=d.*(field.member);
        if(value<field.minimum || value>counterMaximum)throw std::invalid_argument("EDIT_ID_LIMIT");
    }
    const auto& b=d.background;
    if(!b.pngBase64 || b.pngBase64->size()>imageByteLimit)throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
    for(const auto& field:backgroundNumbers) {
        const auto value=b.*(field.member);
        if(!std::isfinite(value) || (field.minimum && (field.exclusiveMinimum?value<=*field.minimum:value<*field.minimum)) ||
           (field.maximum && value>*field.maximum))throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
    }
}
}
