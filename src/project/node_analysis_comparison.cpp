#include "node_analysis_comparison.hpp"
#include "csv_format.hpp"
#include "../model/network/node_membership.hpp"
#include "../model/network/polygon.hpp"
#include <algorithm>
#include <cmath>
namespace multra {
NodeAnalysisMicroSource nodeAnalysisMicroSource(const ProjectDocument& d,const Network& network) {
    NodeAnalysisMicroSource out{d,{}};
    for(const auto& n:network.nodes)if(simplePolygon(n.polygon)) {
        NodeAnalysisMicroKey key{n.id,{}};for(const auto& leg:nodeMembership(network,n).inLegs)key.inLinks.push_back(leg.linkId);out.keys.push_back(std::move(key));
    }
    return out;
}
NodeAnalysisComparison compareNodeAnalysis(const NodeAnalysisReport& a,const NodeAnalysisMicroSource& source,const BatchReport& b,const MovementReport& evaluation) {
    NodeAnalysisComparison out;out.revision=a.run.revision;out.start=evaluation.warmup;out.end=evaluation.evaluationEnd;out.seeds=b.seeds;out.marking=a.marking;
    if(a.run.inputManifest.is_object())for(const auto& f:a.run.inputManifest.at("files"))if(f.at("logicalPath")=="project")out.projectSha256=f.at("sha256").get<std::string>();
    const auto issue=[&](const char* code,const std::string& path){out.issues.push_back({code,path});};
    if(source.document!=a.source){issue("NODE_MICRO_SNAPSHOT","project");return out;}
    if(b.seeds.empty()){issue("NODE_MICRO_BATCH_REQUIRED","batch");return out;}
    const double end=source.document.definition?source.document.definition->duration+evaluation.cooldown.value_or(0):evaluation.time;
    if(evaluation.time+1e-8<end){issue("NODE_MICRO_UNFINISHED","batch");return out;}
    if(source.keys.size()!=b.nodes.size()){issue("NODE_MICRO_MAPPING","nodes");return out;}
    for(const auto& n:a.run.nodes) {
        if(!n.modelSource){issue("NODE_MICRO_NO_WINDOW",n.id);continue;}
        const auto& window=*n.modelSource;
        if(std::abs(window.start-out.start)>1e-8||std::abs(window.end-out.end)>1e-8||std::abs(n.analysisPeriod*3600-(out.end-out.start))>1e-8){issue("NODE_MICRO_WINDOW",n.id);continue;}
        const auto key=std::find_if(source.keys.begin(),source.keys.end(),[&](const auto& k){return k.id==n.id;});
        if(key==source.keys.end()){issue("NODE_MICRO_MAPPING",n.id);continue;}
        const auto index=static_cast<std::size_t>(key-source.keys.begin());const auto& micro=b.nodes[index];
        if(key->inLinks.size()!=micro.legs.size()){issue("NODE_MICRO_MAPPING",n.id);continue;}
        out.rows.push_back({n.id,{},n.name,{},n.delay,n.los,micro.delay,micro.vehicles,micro.unfinished});
        for(std::size_t k=0;k<key->inLinks.size();++k) {
            const auto& link=key->inLinks[k];const auto& leg=micro.legs[k];
            const auto approach=std::find_if(n.approaches.begin(),n.approaches.end(),[&](const auto& x){return x.inLink==link;});
            out.rows.push_back({n.id,link,n.name,leg.name,approach==n.approaches.end()?std::nullopt:approach->delay,
                approach==n.approaches.end()?std::nullopt:approach->los,leg.delay,leg.vehicles,leg.unfinished});
        }
        for(const auto& warning:n.advisories)if(warning.code=="NODE_ANALYSIS_PARTIAL")out.issues.push_back(warning);
    }
    return out;
}
std::string nodeAnalysisComparisonCsv(const NodeAnalysisComparison& r,bool stale) {
    std::string out="# MULTRA - not yet validated. Analytical HCM control delay and Micro simulated travel-time/section delay are different quantities.\n";
    out+="# Revision: "+std::to_string(r.revision)+"; evaluation: "+csvNumber(r.start)+" to "+csvNumber(r.end)+" s; seeds:";
    for(const auto seed:r.seeds)out+=" "+std::to_string(seed);
    out+='\n';
    if(!r.projectSha256.empty())out+="# Project SHA-256: "+r.projectSha256+"\n";
    for(const auto& issue:r.issues)out+="# "+issue.code+","+csvQuoted(issue.path)+"\n";
    out+="level,node,nodeId,approach,inLink,analyticalDelay_s,analyticalLos,micro_n,microDelay_s,microCi95_s,microVehicles_mean,microUnfinished_mean\n";
    for(const auto& row:r.rows)out+=(row.inLink.empty()?std::string("node"):std::string("approach"))+","+csvQuoted(row.nodeName)+","+csvQuoted(row.nodeId)+","+csvQuoted(row.approachName)+","+csvQuoted(row.inLink)+","+
        csvNumber(row.analyticalDelay)+","+(row.analyticalLos?std::string(1,*row.analyticalLos):std::string())+","+std::to_string(row.microDelay.n)+","+
        csvNumber(row.microDelay.mean)+","+csvNumber(row.microDelay.halfWidth95)+","+csvNumber(row.vehicles.mean)+","+csvNumber(row.unfinished.mean)+"\n";
    auto marking=r.marking;if(stale&&std::find(marking.stale.begin(),marking.stale.end(),"project")==marking.stale.end())marking.stale.push_back("project");
    return markedCsv(std::move(out),marking);
}
}
