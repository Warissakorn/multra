#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D569: definition.macroSettings (S2.3, from 51) and root macroRuns (from 56), each refused below
// its floor even null. A run is a closed {id, name, period, settings}; settings a closed
// {targetGap, maxIterations} with the codec's ranges (0 < gap < 1, 1 <= iterations <= 100000).
namespace macroRunCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int settingsFrom=51,runsFrom=56;
inline multra::Json settings() {return {{"targetGap",0.001},{"maxIterations",200}};}
inline multra::Json run() {return {{"id","am-run"},{"name","AM run"},{"period","am"},{"settings",settings()}};}
inline multra::Json minimal(int version=runsFrom) {
    auto j=demandCases::minimal(version);
    if(version>=settingsFrom)j["definition"]["macroSettings"]=settings();
    if(version>=runsFrom)j["macroRuns"]=multra::Json::array({run()});
    return multra::Json::parse(j.dump()); // unsigned tokens, as a saved file holds them
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
        add("macroSettings null",[](Json& j){j["definition"]["macroSettings"]=nullptr;},false);
        add("macroRuns null",[](Json& j){j["macroRuns"]=nullptr;},false);
        if(v<settingsFrom)add("macroSettings below 51",[](Json& j){j["definition"]["macroSettings"]=settings();},false);
        if(v<runsFrom)add("macroRuns below 56",[](Json& j){j["macroRuns"]=Json::array({run()});},false);
        if(v<settingsFrom)continue;
        // The settings row, owned by definition.macroSettings and by each run.
        std::vector<std::pair<std::string,std::function<Json&(Json&)>>> rows{{"definition",[](Json& j)->Json&{return j["definition"]["macroSettings"];}}};
        if(v>=runsFrom)rows.push_back({"run",[](Json& j)->Json&{return j["macroRuns"][0]["settings"];}});
        for(const auto& [label,at]:rows) {
            const auto s=[&,at=at,label=label](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
                add(label+" settings "+name,[&](Json& j){edit(at(j));},valid);
            };
            s("7",[](Json& x){x=7;},false);s("extra",[](Json& x){x["extra"]=1;},false);
            for(const auto* key:{"targetGap","maxIterations"}) {
                s(std::string("missing ")+key,[key](Json& x){x.erase(key);},false);s(std::string(key)+" null",[key](Json& x){x[key]=nullptr;},false);
                s(std::string(key)+" text",[key](Json& x){x[key]="bad";},false);
            }
            s("gap 0",[](Json& x){x["targetGap"]=0;},false);s("gap 1",[](Json& x){x["targetGap"]=1;},false);
            s("gap 0.999",[](Json& x){x["targetGap"]=0.999;},true);s("gap -1",[](Json& x){x["targetGap"]=-1;},false);
            s("iterations 0",[](Json& x){x["maxIterations"]=0;},false);s("iterations 1",[](Json& x){x["maxIterations"]=1;},true);
            s("iterations 100000",[](Json& x){x["maxIterations"]=100000;},true);s("iterations 100001",[](Json& x){x["maxIterations"]=100001;},false);
            s("iterations 2.5",[](Json& x){x["maxIterations"]=2.5;},false);s("iterations -4294967295",[](Json& x){x["maxIterations"]=-4294967295LL;},false);
        }
        if(v<runsFrom)continue;
        add("macroRuns []",[](Json& j){j["macroRuns"]=Json::array();},false);add("macroRuns 7",[](Json& j){j["macroRuns"]=7;},false);
        add("run 7",[](Json& j){j["macroRuns"][0]=7;},false);add("run extra",[](Json& j){j["macroRuns"][0]["extra"]=1;},false);
        for(const auto* key:{"id","name","period","settings"}) {
            add(std::string("run missing ")+key,[key](Json& j){j["macroRuns"][0].erase(key);},false);
            add(std::string("run ")+key+" null",[key](Json& j){j["macroRuns"][0][key]=nullptr;},false);
            add(std::string("run ")+key+" 7",[key](Json& j){j["macroRuns"][0][key]=7;},false);
        }
        add("run name empty",[](Json& j){j["macroRuns"][0]["name"]="";},true);
        add("run period blank",[](Json& j){j["macroRuns"][0]["period"]=" ";},true); // the model's INVALID_MACRO_RUN
    }
    return result;
}
}
