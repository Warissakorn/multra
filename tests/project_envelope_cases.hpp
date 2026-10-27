#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>
#include <limits>

namespace envelopeCases {
struct Case {std::string name;multra::Json document;bool codecAccepts,schemaAccepts;};
inline std::vector<Case> cases() {
    using namespace multra;
    const auto base=documentJson(ProjectDocument{});
    std::vector<Case> result{{"empty draft",base,true,true}};
    result.push_back({"root array",Json::array(),false,false});
    auto legacy=base;legacy.erase("schemaVersion");
    result.push_back({"legacy scenario path",std::move(legacy),true,false});
    const auto change=[&](const char* path,Json value,bool codec,bool schema) {
        auto j=base;j[Json::json_pointer(path)]=std::move(value);
        result.push_back({path,std::move(j),codec,schema});
    };
    for(const auto* key:{"format","nextId","revision","network","background"}) {
        auto j=base;
        j.erase(key);result.push_back({std::string("missing ")+key,std::move(j),false,false});
    }
    for(const auto* path:{"/schemaVersion","/format","/nextId","/revision","/network","/background"})
        change(path,nullptr,false,false);
    change("/format","MULTRA",false,false);
    change("/schemaVersion",0,false,false);
    change("/schemaVersion",kLatestSchema+1,false,false);
    change("/schemaVersion",17.0,false,true); // JSON Schema uses mathematical integers.
    change("/nextId",std::uint64_t{0},false,false);
    change("/nextId",std::numeric_limits<std::uint64_t>::max(),false,false);
    change("/revision",std::numeric_limits<std::uint64_t>::max(),false,false);
    change("/nextId",std::numeric_limits<std::uint64_t>::max()-1,true,true);
    change("/revision",std::numeric_limits<std::uint64_t>::max()-1,true,true);
    change("/nextId",1.0,false,true);
    change("/revision",0.0,false,true);
    change("/nextId",true,false,false);
    change("/nextId",-1,false,false);
    change("/network",Json::array(),false,false);
    change("/network",Json::object(),false,true); // Nested fields remain codec-owned.
    change("/definition",nullptr,true,true);
    change("/definition",7,false,false);
    for(const auto* key:{"pngBase64","x","y","metresPerPixel","rotation","opacity"}) {
        auto j=base;j["background"].erase(key);
        result.push_back({std::string("missing background.")+key,std::move(j),false,false});
        change((std::string("/background/")+key).c_str(),nullptr,false,false);
    }
    change("/background/pngBase64","ไทย",true,true); // No PNG decode in this codec.
    change("/background/x",true,false,false);
    change("/background/metresPerPixel",0,false,false);
    change("/background/metresPerPixel",0.00001,true,true);
    change("/background/opacity",0,true,true);
    change("/background/opacity",1,true,true);
    change("/background/opacity",-0.00001,false,false);
    change("/background/opacity",1.00001,false,false);
    change("/background/extra",Json::array({1,2,3}),true,true);
    change("/unknownExtension",Json{{"anything",true}},true,true);
    change("/map",nullptr,false,true); // Optional feature shapes are explicitly out of scope.
    return result;
}
}
