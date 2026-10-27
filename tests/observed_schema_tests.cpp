#include "test.hpp"
#include "../src/project/observed_schema.hpp"

using namespace multra;
namespace {
// Evaluate precisely the structural keywords this discovery emits. This checks that
// discovery never contradicts its own sample, including heterogeneous truncated arrays.
bool accepts(const Json& schema,const Json& value) {
    if(schema.contains("anyOf")) {
        for(const auto& branch:schema.at("anyOf"))if(accepts(branch,value))return true;
        return false;
    }
    if(!schema.contains("type"))return true;
    const auto type=schema.at("type").get<std::string>();
    if(type=="object") {
        if(!value.is_object())return false;
        for(const auto& [key,child]:schema.at("properties").items())
            if(value.contains(key) && !accepts(child,value.at(key)))return false;
        return true;
    }
    if(type=="array") {
        if(!value.is_array())return false;
        for(const auto& item:value)if(!accepts(schema.at("items"),item))return false;
        return true;
    }
    if(type=="number")return value.is_number();
    if(type=="boolean")return value.is_boolean();
    return value.is_string();
}
}
TEST(project, observed_schema_is_structural_and_accepts_mixed_samples) {
    const Json value={{"mixed",Json::array({1,2.5,"ไทย",true,nullptr,Json{{"a/b~c",3}},Json::array()})},
                      {"empty",Json::object()}};
    const auto result=observeJsonSchema(value);
    CHECK(!result.truncated);CHECK(accepts(result.schema,value));
    CHECK(result.schema.at("$schema")=="https://json-schema.org/draft/2020-12/schema");
    CHECK(!result.schema.contains("required"));CHECK(result.schema.at("additionalProperties")==true);
    const auto variants=result.schema.at("properties").at("mixed").at("items").at("anyOf");
    CHECK(variants.size()==6); // integer and floating-point observations share number.
    CHECK(variants.front()==Json({{"type","number"}}));
    CHECK(observeJsonSchema(Json::array()).schema.at("items")==Json::object());
    CHECK(!observeJsonSchema(nullptr).schema.contains("type"));
}
TEST(project, observed_schema_budget_does_not_exclude_unvisited_array_siblings) {
    Json payload=Json::array();
    for(int i=0;i<50000;++i)payload.push_back(i);
    const Json sample=Json::array({Json{{"payload",std::move(payload)}},"unvisited",true});
    CHECK(sample.at(0).at("payload").size()==50000);CHECK(sample.at(1).is_string());
    const auto result=observeJsonSchema(sample);
    CHECK(result.truncated);
    CHECK(result.schema.at("items")==Json::object());
    CHECK(accepts(result.schema,sample));
    // The same exhausted budget must also leave enclosing arrays open.
    const Json nested=Json::array({sample,17});
    const auto outer=observeJsonSchema(nested);
    CHECK(outer.truncated);CHECK(accepts(outer.schema,nested));
}
TEST(project, observed_schema_budget_boundary_reports_only_unvisited_values) {
    Json sample=Json::array();
    for(int i=0;i<49999;++i)sample.push_back(i);
    auto result=observeJsonSchema(sample);
    CHECK(!result.truncated);CHECK(result.schema.at("items")==Json({{"type","number"}}));
    sample.push_back("beyond budget");
    result=observeJsonSchema(sample);
    CHECK(result.truncated);CHECK(result.schema.at("items")==Json::object());
    CHECK(accepts(result.schema,sample));
}
TEST(project, observed_schema_depth_boundary_preserves_the_sample) {
    Json sample="leaf";
    for(int i=0;i<33;++i)sample=Json::array({sample});
    const auto result=observeJsonSchema(sample);
    CHECK(result.truncated);CHECK(accepts(result.schema,sample));
    sample="leaf";
    for(int i=0;i<32;++i)sample=Json{{"child",std::move(sample)}};
    CHECK(!observeJsonSchema(sample).truncated);
    sample=Json{{"child",std::move(sample)}};
    const auto object=observeJsonSchema(sample);
    CHECK(object.truncated);CHECK(accepts(object.schema,sample));
}
