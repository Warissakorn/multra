#include "node_analysis_source_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
namespace multra {
namespace {
[[noreturn]] void invalid(const std::string& path) {throw ValidationError({{"EDIT_NODE_INVALID",path}});}
void keys(const Json& j,const std::string& path,std::initializer_list<const char*> required,std::initializer_list<const char*> optional={}) {
    if(!j.is_object())invalid(path);
    for(const auto* k:required)if(!j.contains(k))invalid(path+"."+k);
    for(const auto& [k,v]:j.items()) {
        (void)v;
        if(std::none_of(required.begin(),required.end(),[&](const char* x){return k==x;})&&std::none_of(optional.begin(),optional.end(),[&](const char* x){return k==x;}))invalid(path+"."+k);
    }
}
std::string hash(const Json& j,const std::string& path) {
    if(!j.is_string())invalid(path);
    const auto s=j.get<std::string>();
    if(s.size()!=64||!std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))invalid(path);
    return s;
}
std::vector<std::string> strings(const Json& j,const std::string& path) {
    if(!j.is_array())invalid(path);
    std::vector<std::string> out;
    for(std::size_t i=0;i<j.size();++i) {
        const auto at=path+"["+std::to_string(i)+"]";
        if(!j[i].is_string()||j[i].get<std::string>().empty())invalid(at);
        const auto value=j[i].get<std::string>();if(std::find(out.begin(),out.end(),value)!=out.end())invalid(at);out.push_back(value);
    }
    return out;
}
}
Json nodeAnalysisSourceJson(const NodeAnalysisModelSource& s) {
    Json files=Json::array();for(const auto& f:s.files)files.push_back({{"logicalPath",f.logicalPath},{"sha256",f.sha256}});
    Json j{{"kind","micro-demand"},{"revision",s.revision},{"projectSha256",s.projectSha256},{"start",s.start},{"end",s.end},
           {"heavyVehicleTypes",s.heavyVehicleTypes},{"files",files}};
    if(!s.fallbacks.empty())j["fallbacks"]=s.fallbacks;
    return j;
}
NodeAnalysisModelSource parseNodeAnalysisSource(const Json& j,const std::string& path,int schema) {
    if(schema<64)throw ValidationError({{"EDIT_UNSUPPORTED_FIELD",path}});
    keys(j,path,{"kind","revision","projectSha256","start","end","heavyVehicleTypes","files"},{"fallbacks"});
    if(j.at("kind")!="micro-demand")invalid(path+".kind");
    const auto& rev=j.at("revision");if(!rev.is_number_unsigned()&&!(rev.is_number_integer()&&rev.get<std::int64_t>()>=0))invalid(path+".revision");
    NodeAnalysisModelSource s;s.revision=rev.get<std::uint64_t>();s.projectSha256=hash(j.at("projectSha256"),path+".projectSha256");
    const auto number=[&](const char* name){const auto& v=j.at(name);if(!v.is_number()||!std::isfinite(v.get<double>()))invalid(path+"."+name);return v.get<double>();};
    s.start=number("start");s.end=number("end");if(s.start<0)invalid(path+".start");if(s.end<=s.start)invalid(path+".end");
    s.heavyVehicleTypes=strings(j.at("heavyVehicleTypes"),path+".heavyVehicleTypes");
    const auto& files=j.at("files");if(!files.is_array())invalid(path+".files");
    for(std::size_t i=0;i<files.size();++i) {
        const auto at=path+".files["+std::to_string(i)+"]";const auto& f=files[i];keys(f,at,{"logicalPath","sha256"});
        if(!f.at("logicalPath").is_string()||f.at("logicalPath").get<std::string>().empty())invalid(at+".logicalPath");
        const auto name=f.at("logicalPath").get<std::string>();
        if(std::any_of(s.files.begin(),s.files.end(),[&](const auto& x){return x.logicalPath==name;}))invalid(at+".logicalPath");
        s.files.push_back({name,hash(f.at("sha256"),at+".sha256")});
    }
    if(j.contains("fallbacks")){s.fallbacks=strings(j.at("fallbacks"),path+".fallbacks");if(s.fallbacks.empty())invalid(path+".fallbacks");}
    return s;
}
bool usesNodeAnalysisSource(const ProjectDocument& d) {
    return std::any_of(d.network.nodes.begin(),d.network.nodes.end(),[](const auto& n){return n.analysis&&n.analysis->modelSource;});
}
void validateNodeAnalysisSources(const ProjectDocument& d) {
    for(std::size_t i=0;i<d.network.nodes.size();++i)if(const auto& a=d.network.nodes[i].analysis;a&&a->modelSource)
        (void)parseNodeAnalysisSource(nodeAnalysisSourceJson(*a->modelSource),"network.nodes["+std::to_string(i)+"].analysis.modelSource",64);
}
}
