#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D571: schema-67 owned matrices. Blank/unique ids and n-by-n agreement stay with the codec;
// in particular an empty row or a rectangular matrix can be structurally accepted here.
namespace costMatrixCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=67;
inline multra::Json matrix() {
    return {{"id","own"},{"name","Own matrix"},{"source",multra::Json{{"citation","c"}}},{"unit","min"},
        {"zoneIds",multra::Json::array({"a","b"})},
        {"rows",multra::Json::array({multra::Json::array({0,nullptr}),multra::Json::array({5.5,0})})}};
}
inline multra::Json minimal(int version=from) {
    auto j=demandCases::minimal(version);
    if(version>=from)j["costMatrices"]=multra::Json::array({matrix()});
    return multra::Json::parse(j.dump());
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);
        const auto add=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            auto j=base;edit(j);result.push_back({name,Json::parse(j.dump()),valid});
        };
        result.push_back({"minimal",base,true});
        add("costMatrices absent",[](Json& j){j.erase("costMatrices");},true);
        add("costMatrices null",[](Json& j){j["costMatrices"]=nullptr;},false);
        if(v<from) {add("costMatrices below 67",[](Json& j){j["costMatrices"]=Json::array({matrix()});},false);continue;}
        const auto at=[](Json& j)->Json& {return j["costMatrices"][0];};
        add("costMatrices []",[](Json& j){j["costMatrices"]=Json::array();},false);
        add("costMatrices 7",[](Json& j){j["costMatrices"]=7;},false);
        add("matrix 7",[&](Json& j){at(j)=7;},false);
        add("matrix extra",[&](Json& j){at(j)["extra"]=1;},false);
        for(const auto* key:{"id","name","unit","zoneIds","rows"}) {
            add(std::string("matrix missing ")+key,[&,key](Json& j){at(j).erase(key);},false);
            add(std::string("matrix ")+key+" null",[&,key](Json& j){at(j)[key]=nullptr;},false);
            add(std::string("matrix ")+key+" 7",[&,key](Json& j){at(j)[key]=7;},false);
        }
        add("matrix id blank",[&](Json& j){at(j)["id"]=" ";},true);
        add("matrix name empty",[&](Json& j){at(j)["name"]="";},true);
        add("unit unknown",[&](Json& j){at(j)["unit"]="s";},false);
        add("two matrices, one id",[&](Json& j){j["costMatrices"].push_back(matrix());},true);
        add("source absent",[&](Json& j){at(j).erase("source");},true);
        add("source {}",[&](Json& j){at(j)["source"]=Json::object();},true);
        for(const auto& value:std::vector<Json>{nullptr,7,"c",Json{{"table","t"}},Json{{"extra",1}},
                Json{{"citation",nullptr}},Json{{"citation",7}},Json{{"citation",""}}})
            add("source "+value.dump(),[&](Json& j){at(j)["source"]=value;},value==Json{{"citation",""}});
        add("zoneIds []",[&](Json& j){at(j)["zoneIds"]=Json::array();},false);
        for(const auto& value:std::vector<Json>{nullptr,7,true," ","a"})
            add("zone id "+value.dump(),[&](Json& j){at(j)["zoneIds"][1]=value;},value.is_string());
        add("rows []",[&](Json& j){at(j)["rows"]=Json::array();},false);
        for(const auto& value:std::vector<Json>{nullptr,7,"row",Json::array(),Json::array({1}),Json::array({1,2,3})})
            add("row "+value.dump(),[&](Json& j){at(j)["rows"][0]=value;},value.is_array());
        add("too few rows",[&](Json& j){at(j)["rows"].erase(1);},true);
        add("too many rows",[&](Json& j){at(j)["rows"].push_back(Json::array({0,0}));},true);
        for(const auto& value:std::vector<Json>{nullptr,"1",true,Json::object(),Json::array(),-0.1,0,0.1})
            add("cell "+value.dump(),[&](Json& j){at(j)["rows"][0][0]=value;},
                value.is_null() || (value.is_number() && value.get<double>()>=0));
    }
    return result;
}
}
