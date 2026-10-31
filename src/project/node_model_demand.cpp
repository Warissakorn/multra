#include "node_model_demand.hpp"
#include "od_demand.hpp"
#include "../model/network/node_membership.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
namespace multra {
namespace {
[[noreturn]] void refuse(const char* code,const std::string& path) {throw ValidationError({{code,path}});}
struct Span {
    std::string object;double start{},end{};bool connector{};
    const std::vector<Point> *reference{},*lane{};
};
struct Traversal {std::vector<std::size_t> turns;bool touches{},partial{};};
bool covers(const Span& span,double station) {
    const double at=span.reference?matchedStation(*span.reference,*span.lane,station):station;
    return span.start<=at+1e-6&&span.end>=at-1e-6;
}
Traversal traversal(const Route& route,const std::map<std::string,Span>& spans,const NodeMembership& m) {
    std::vector<Span> chain;
    for(const auto& id:route.segmentIds) {
        const auto& s=spans.at(id);
        if(!chain.empty()&&!s.connector&&chain.back().object==s.object&&chain.back().lane==s.lane&&std::abs(chain.back().end-s.start)<1e-6)chain.back().end=s.end;
        else chain.push_back(s);
    }
    Traversal result;std::set<std::size_t> covered;
    for(std::size_t at=0;at<chain.size();++at) {
        std::set<std::size_t> matches;
        for(std::size_t t=0;t<m.turns.size();++t)for(const auto& path:m.turns[t].paths) {
            if(at+path.size()>chain.size())continue;
            bool match=true;
            for(std::size_t k=0;k<path.size();++k) {
                const auto& s=chain[at+k];const auto& p=path[k];
                if(s.object!=p.objectId||(!s.connector&&(!covers(s,p.from)||!covers(s,p.to)))){match=false;break;}
            }
            if(match){matches.insert(t);for(std::size_t k=0;k<path.size();++k)covered.insert(at+k);}
        }
        if(matches.size()>1)refuse("NODE_MODEL_AMBIGUOUS_ROUTE","routes["+route.id+"]");
        if(!matches.empty())result.turns.push_back(*matches.begin()); // once per traversal, not per lane/path
    }
    for(std::size_t k=0;k<chain.size();++k) {
        const auto& s=chain[k];
        const bool touches=std::find(m.connectors.begin(),m.connectors.end(),s.object)!=m.connectors.end()||
            std::find(m.links.begin(),m.links.end(),s.object)!=m.links.end()||
            std::any_of(m.inLegs.begin(),m.inLegs.end(),[&](const auto& l){return l.linkId==s.object&&covers(s,l.station);})||
            std::any_of(m.outLegs.begin(),m.outLegs.end(),[&](const auto& l){return l.linkId==s.object&&covers(s,l.station);});
        result.touches=result.touches||touches;result.partial=result.partial||(touches&&!covered.contains(k));
    }
    return result;
}
}
NodeModelDemand previewNodeModelDemand(std::string_view bytes,const std::string& nodeId,const NodeModelDemandRequest& request,
                                     const std::filesystem::path& data,const StudyAreaSources* sources) {
    InputManifest manifest;manifest.record("project",bytes);
    const auto d=parseDocument(Json::parse(bytes.begin(),bytes.end()));
    if(!d.definition)refuse("EDIT_NO_DEFINITION","definition");
    if(!std::isfinite(request.start)||!std::isfinite(request.end)||request.start<0||request.end<=request.start||request.end>d.definition->duration)
        refuse("NODE_MODEL_WINDOW","analysis.modelSource");
    const auto original=std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& n){return n.id==nodeId;});
    if(original==d.network.nodes.end())refuse("EDIT_UNKNOWN_OBJECT",nodeId);
    const auto path="network.nodes["+std::to_string(original-d.network.nodes.begin())+"]";
    // An OD contribution omitted by the ordinary run cannot silently become an analysis zero.
    for(const auto& gap:odDemandGaps(d))refuse("NODE_MODEL_INCOMPLETE_DEMAND",gap.path);
    const auto snapshot=compileDocument(d,data,&manifest,sources);
    const auto node=std::find_if(snapshot.network.nodes.begin(),snapshot.network.nodes.end(),[&](const auto& n){return n.id==nodeId;});
    if(node==snapshot.network.nodes.end())refuse("NODE_MODEL_OUTSIDE_AREA",path);
    const auto m=nodeMembership(snapshot.network,*node);
    if(!m.issues.empty())refuse(m.issues[0].c_str(),path);
    if(m.turns.empty())refuse("NODE_ANALYSIS_NEEDS_DETAIL",path);
    std::set<std::string> heavy;
    for(std::size_t i=0;i<request.heavyVehicleTypes.size();++i) {
        const auto& id=request.heavyVehicleTypes[i];
        if(!heavy.insert(id).second||std::none_of(snapshot.scenario.vehicleTypes.begin(),snapshot.scenario.vehicleTypes.end(),[&](const auto& t){return t.id==id;}))
            refuse("NODE_MODEL_HV_TYPE","heavyVehicleTypes["+std::to_string(i)+"]");
    }
    const auto table=runtimeSections(snapshot.network);std::map<std::string,Span> spans;
    std::map<std::string,std::vector<Point>> geometries;std::map<std::string,const std::vector<Point>*> references;
    for(const auto& link:snapshot.network.links)for(const auto& lane:link.lanes) {
        references.emplace(lane.id,&link.geometry);geometries.emplace(lane.id,laneGeometry(link,lane.id,snapshot.network.drivingSide));
    }
    for(const auto& s:table.sections)spans.emplace(s.id,Span{s.linkId,s.start,s.end,false,references.at(s.laneId),&geometries.at(s.laneId)});
    for(std::size_t k=0;k<table.paths.size();++k)spans.emplace(table.paths[k].id,Span{table.pathConnector[k],0,0,true});
    std::map<std::string,Traversal> walks;
    for(const auto& r:snapshot.scenario.routes)walks.emplace(r.id,traversal(r,spans,m));
    std::set<std::string> stubs;for(const auto& dead:snapshot.scenario.routeDeadEnds)stubs.insert(dead.routeId);
    // A compiled entry-lane stub is scheduled demand for its authored route, even if a
    // simulated vehicle later fails its mandatory lane change. Use only an unambiguous complete
    // family from the compiler's exact origin mapping, never an id-prefix guess or lane count.
    for(const auto& dead:snapshot.scenario.routeDeadEnds) {
        const auto origin=snapshot.routeOrigins.find(dead.routeId);if(origin==snapshot.routeOrigins.end())continue;
        std::optional<Traversal> complete;bool ambiguous=false;
        for(const auto& [id,other]:snapshot.routeOrigins)if(other==origin->second&&!stubs.contains(id)&&!walks.at(id).partial) {
            const auto& w=walks.at(id);
            if(!complete)complete=w;else if(complete->turns!=w.turns)ambiguous=true;
        }
        if(complete&&!ambiguous)walks.at(dead.routeId)=*complete;
    }
    const auto reachable=[&](const std::string& root) {
        std::set<std::string> seen;std::vector<std::string> queue{root};
        for(std::size_t k=0;k<queue.size();++k)if(seen.insert(queue[k]).second)
            for(const auto& decision:snapshot.scenario.routeDecisions)if(decision.fromRouteId==queue[k])
                for(const auto& choice:decision.choices)if(choice.weight>0||std::any_of(choice.intervalWeights.begin(),choice.intervalWeights.end(),[](double n){return n>0;}))queue.push_back(choice.routeId);
        return seen;
    };
    // Determine which lots could release vehicles, including lot-to-lot chains. A provably
    // empty lot without gate demand is harmless; an exit's zero-rate queue alone proves nothing.
    const auto enters=[&](const std::string& route,const ParkingLotSpec& lot) {
        for(const auto& id:reachable(route))for(const auto& gate:lot.gateRouteIds) {
            if(id==gate)return true;
            const auto a=snapshot.routeOrigins.find(id),b=snapshot.routeOrigins.find(gate);
            if(a!=snapshot.routeOrigins.end()&&b!=snapshot.routeOrigins.end()&&a->second==b->second)return true;
        }
        return false;
    };
    std::set<std::string> activeLots;
    for(const auto& lot:snapshot.scenario.parkingLots) {
        bool active=lot.initialOccupancy>0;
        for(const auto& input:snapshot.scenario.inputs)if(input.vehiclesPerHour>0&&input.startTime<request.end&&enters(input.routeId,lot))active=true;
        if(active)activeLots.insert(lot.id);
    }
    for(std::size_t pass=0;pass<snapshot.scenario.parkingLots.size();++pass)
        for(const auto& from:snapshot.scenario.parkingLots)if(activeLots.contains(from.id))for(const auto& exit:from.exits)if(exit.weight>0)
            for(const auto& to:snapshot.scenario.parkingLots)if(enters(exit.routeId,to))activeLots.insert(to.id);
    for(const auto& lot:snapshot.scenario.parkingLots)if(activeLots.contains(lot.id))for(const auto& exit:lot.exits)if(exit.weight>0)
        for(const auto& id:reachable(exit.routeId))if(walks.at(id).touches)refuse("NODE_MODEL_RUNTIME_DEMAND","parkingLots["+lot.id+"]");
    NodeModelDemand result;result.revision=d.revision;
    for(const auto& t:m.turns) {
        NodeAnalysisTurn row{m.inLegs[t.in].linkId,m.outLegs[t.out].linkId,0,{}};
        if(original->analysis)for(const auto& old:original->analysis->turns)if(old.in==row.in&&old.out==row.out){row.heavyVehiclePercent=old.heavyVehiclePercent;break;}
        result.turns.push_back(std::move(row));
    }
    std::vector<double> volumes(m.turns.size()),hv(m.turns.size());
    for(const auto& input:snapshot.scenario.inputs) {
        const double seconds=std::max(0.,std::min(input.endTime,request.end)-std::max(input.startTime,request.start));
        const double count=input.vehiclesPerHour*seconds/3600;if(!(count>0))continue;
        const auto possible=reachable(input.routeId);const auto& walk=walks.at(input.routeId);
        const bool affected=std::any_of(possible.begin(),possible.end(),[&](const auto& id){return walks.at(id).touches;});
        for(const auto& decision:snapshot.scenario.routeDecisions)if(affected&&possible.contains(decision.fromRouteId)) {
            // A choice that preserves every Node traversal is harmless (e.g. routing downstream).
            const auto& first=walks.at(decision.fromRouteId);
            for(const auto& c:decision.choices)if(c.weight>0||std::any_of(c.intervalWeights.begin(),c.intervalWeights.end(),[](double n){return n>0;})) {
                const auto& alternative=walks.at(c.routeId);
                if(first.turns!=alternative.turns||first.partial!=alternative.partial)
                    refuse("NODE_MODEL_RUNTIME_DEMAND","routingDecisions["+decision.id+"]");
            }
        }
        if(walk.partial)refuse("NODE_MODEL_PARTIAL_ROUTE","inputs["+input.id+"].routeId");
        const auto r=std::find_if(snapshot.scenario.routes.begin(),snapshot.scenario.routes.end(),[&](const auto& r){return r.id==input.routeId;});
        if(walk.touches&&r->circuit)refuse("NODE_MODEL_RUNTIME_DEMAND","routes["+r->id+"]");
        for(const auto turn:walk.turns){volumes[turn]+=count;if(heavy.contains(input.vehicleTypeId))hv[turn]+=count;}
    }
    for(std::size_t k=0;k<volumes.size();++k)if(volumes[k]>0) {
        result.turns[k].vehiclesPerHour=volumes[k]*3600/(request.end-request.start);
        result.turns[k].heavyVehiclePercent=100*hv[k]/volumes[k];
    }
    manifest.validate();result.inputManifest=manifest.json();return result;
}
}
