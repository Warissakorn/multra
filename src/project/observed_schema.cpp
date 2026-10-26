#include "observed_schema.hpp"
#include <set>

namespace multra {
namespace {
using Json=nlohmann::json;
Json observe(const Json& value,int depth,int& budget,bool& truncated) {
    if(depth>32 || budget==0) {truncated=true;return Json::object();}
    --budget;
    if(value.is_null())return Json::object();
    if(depth==32 && !value.empty() && (value.is_array() || value.is_object())) {
        truncated=true;
        if(value.is_array())return {{"type","array"},{"items",Json::object()}};
        return {{"type","object"},{"properties",Json::object()},{"additionalProperties",true}};
    }
    if(value.is_object()) {
        Json properties=Json::object();
        for(const auto& [key,child]:value.items()) {
            if(budget==0) {truncated=true;break;}
            properties[key]=observe(child,depth+1,budget,truncated);
        }
        return {{"type","object"},{"properties",properties},{"additionalProperties",true}};
    }
    if(value.is_array()) {
        Json variants=Json::array();std::set<std::string> seen;
        for(const auto& item:value) {
            // Even a partly explored object can consume the remaining budget. Its shape
            // cannot describe the unseen siblings (which may be scalars or other arrays).
            if(budget==0) {
                truncated=true;
                return {{"type","array"},{"items",Json::object()}};
            }
            auto shape=observe(item,depth+1,budget,truncated);
            if(seen.insert(shape.dump()).second)variants.push_back(std::move(shape));
        }
        Json items=variants.empty()?Json::object():variants.size()==1?variants.front():Json{{"anyOf",variants}};
        return {{"type","array"},{"items",items}};
    }
    return {{"type",value.is_boolean()?"boolean":value.is_number()?"number":"string"}};
}
}
ObservedJsonSchema observeJsonSchema(const nlohmann::json& value) {
    int budget=50000;ObservedJsonSchema result;
    result.schema=observe(value,0,budget,result.truncated);
    result.schema["$schema"]="https://json-schema.org/draft/2020-12/schema";
    return result;
}
}
