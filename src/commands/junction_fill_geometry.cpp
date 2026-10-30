#include "junction_fill_detail.hpp"
#include "connector_commands.hpp"
#include "network_commands.hpp"
#include "pocket_commands.hpp"
#include "street_segment_commands.hpp"
#include "../model/demand/coarse_compile.hpp"
#include "../model/demand/nema.hpp"
#include "../model/network/approach_layout.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/node_detail.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace multra::junctionDetail {
namespace {
struct Arm { std::string id,in,out,role; std::vector<JunctionLane> rows; };
std::string root(const ProjectDocument& d,const Node& node) {
    const auto i=std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& x){return x.id==node.id;});
    return "network.nodes["+std::to_string(i-d.network.nodes.begin())+"].junction";
}
double sweep(const Link& in,const Link& out,DrivingSide side) {
    const auto a=directionAlong(in.geometry,polylineLength(in.geometry),true), b=directionAlong(out.geometry,0,false);
    return (side==DrivingSide::left?1.:-1.)*std::atan2(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y);
}
char movement(double angle) {
    if(std::abs(angle)<std::numbers::pi/4)return 'T';
    if(std::abs(angle)>=3*std::numbers::pi/4)return 'U';
    return angle>0?'K':'F';
}
PocketSide sideOf(const JunctionLane& row) {return row.side.value_or(row.movements=="F"?PocketSide::far:PocketSide::kerb);}
}
Plan planNormalized(const ProjectDocument& original,const Node& selected,const NodeFillCatalogs& catalogs,bool explicitPolygonFit) {
    Plan result; result.document=original;
    auto& d=result.document; auto& n=d.network;
    const auto at=root(original,selected);
    const auto bad=[&](const std::string& code,const std::string& path){result.issues.push_back({code,path});};
    if(!selected.junction){bad("JUNCTION_TEMPLATE_UNKNOWN",at);return result;}
    const auto recipe=*selected.junction;
    const auto& pool=d.definition&&!d.definition->externalJunctionTemplates?d.definition->junctionTemplates:catalogs.junctionTemplates;
    const auto found=std::find_if(pool.begin(),pool.end(),[&](const auto& t){return t.id==recipe.templateId;});
    if(found==pool.end()){bad("JUNCTION_TEMPLATE_UNKNOWN",at+".templateId");return result;}
    const auto type=*found;
    if(type.kind=="transition")return transitionPlan(original,selected,catalogs,type);
    if(type.control=="allWayStop")bad("FILL_NODE_ALL_WAY_STOP",at+".templateId"); // B31: deferred until A5 or U1 needs it
    if(const auto invalid=junctionTemplateIssues(type,at+".templateId");!invalid.empty()){result.issues=invalid;return result;}
    if(type.control=="signalised"&&type.legs!=3) {
        result.signalPhases=nemaPhases(type.phasing);
        auto timing=nemaTiming(type.phasing,recipe.signal.value_or(JunctionSignal{}),d.definition?d.definition->timeStep:0,at+".signal");
        result.issues.insert(result.issues.end(),timing.issues.begin(),timing.issues.end());
        if(timing.issues.empty())result.signal=std::move(timing.controller);
    } else if(type.control!="signalised"&&recipe.signal)bad("JUNCTION_SITE_COUNT",at+".signal");
    const auto restored=restore(d,selected,catalogs);
    result.issues.insert(result.issues.end(),restored.begin(),restored.end());
    if(!restored.empty())return result;
    std::map<std::string,Arm> arms;
    for(const auto& leg:nodeStreetSegmentLegs(n,selected))arms.emplace(leg.streetSegmentId,Arm{leg.streetSegmentId});
    if(arms.size()!=static_cast<std::size_t>(type.legs))bad("JUNCTION_LEG_COUNT",at+".templateId");
    for(const auto& leg:nodeLinkLegs(n,selected)) {
        const auto& l=link(d,leg.linkId);
        if(!l.streetSegmentId||!arms.contains(*l.streetSegmentId)){bad("JUNCTION_LEG_UNKNOWN",l.id);continue;}
        auto& arm=arms.at(*l.streetSegmentId); auto& slot=leg.in?arm.in:arm.out;
        if(!slot.empty())bad("JUNCTION_LEG_COUNT",arm.id); else slot=l.id;
    }
    for(const auto& [id,arm]:arms) {
        const auto r=std::find_if(n.streetSegments.begin(),n.streetSegments.end(),[&](const auto& x){return x.id==id;});
        if(r==n.streetSegments.end()||r->links.empty())bad("FILL_NODE_STREET_SEGMENT_NOT_GENERATED",id);
        else if(arm.in.empty()||arm.out.empty())bad("FILL_NODE_ARM_OUTSIDE",id);
    }
    for(const auto& [id,leg]:recipe.legs) { (void)leg; if(!arms.contains(id))bad("JUNCTION_LEG_UNKNOWN",at+".legs."+id); }
    std::set<std::string> major(recipe.major.begin(),recipe.major.end());
    if(major.empty()) {
        std::vector<std::pair<std::string,std::optional<int>>> ranks;
        for(const auto& [id,arm]:arms) {
            std::optional<int> rank;
            if(!arm.in.empty())if(const auto typeId=link(d,arm.in).linkTypeId)
                for(const auto& t:catalogs.types)if(t.id==*typeId)rank=t.rank;
            ranks.emplace_back(id,rank);
        }
        if(const auto ranked=majorByRank(ranks))major.insert(ranked->begin(),ranked->end());
        else bad("NODE_PRIORITY_UNDETERMINED",at+".major");
    }
    if(!major.empty() && (major.size()!=2||std::any_of(major.begin(),major.end(),[&](const auto& id){return !arms.contains(id);})))bad("JUNCTION_MAJOR_ARMS",at+".major");
    if(arms.size()!=static_cast<std::size_t>(type.legs)||major.size()!=2||std::any_of(arms.begin(),arms.end(),[](const auto& x){return x.second.in.empty()||x.second.out.empty();}) ||
       std::any_of(major.begin(),major.end(),[&](const auto& id){return !arms.contains(id);}))return result;
    for(auto& [id,arm]:arms) {
        arm.role=major.contains(id)?"major":"minor";
        if(type.legs==3&&major.contains(id)) {
            const auto stem=std::find_if(arms.begin(),arms.end(),[&](const auto& x){return !major.contains(x.first);});
            arm.role=movement(sweep(link(d,arm.in),link(d,stem->second.out),n.drivingSide))=='F'?"major-far":"major-kerb";
        }
        const auto leg=recipe.legs.find(id);
        arm.rows=leg!=recipe.legs.end()&&leg->second.lanes?*leg->second.lanes:type.roles.at(arm.role);
        const auto path=at+".legs."+id;
        const auto actual=link(d,arm.in).lanes.size();
        const auto required=static_cast<std::size_t>(std::count_if(arm.rows.begin(),arm.rows.end(),[](const auto& r){return !r.pocket;}));
        if(actual!=required){bad("JUNCTION_LANE_COUNT",path+".lanes");result.laneCounts.push_back({id,actual,required});}
        if(arm.rows.size()>12)bad("JUNCTION_LANE_COUNT",path+".lanes");
        const auto pockets=arm.rows.size()-required;
        for(const auto* key:{"storage","taper"}) {
            const std::vector<std::optional<double>> empty;
            const auto& values=leg==recipe.legs.end()?empty:std::string(key)=="storage"?leg->second.storage:leg->second.taper;
            if(values.size()>pockets)bad("JUNCTION_SITE_COUNT",path+"."+key);
            for(std::size_t p=0;p<pockets;++p) {
                const auto valuePath=path+"."+key+"["+std::to_string(p)+"]";
                if(p>=values.size()||!values[p])bad(std::string(key)=="storage"?"JUNCTION_STORAGE_MISSING":"JUNCTION_TAPER_MISSING",valuePath);
                else if(*values[p]<kMinSectionLength)bad("JUNCTION_POCKET_RANGE",valuePath);
            }
        }
        for(std::size_t i=0;i<arm.rows.size();++i)if(arm.rows[i].pocket) {
            const auto side=sideOf(arm.rows[i]);
            for(std::size_t j=0;j<arm.rows.size();++j)
                if((side==PocketSide::far?j<i:j>i)&&!arm.rows[j].pocket)bad("JUNCTION_POCKET_LAYOUT",path+".lanes["+std::to_string(i)+"]");
        }
    }
    if(type.control=="signalised"&&type.legs==3) {
        const auto current=std::find_if(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return x.id==selected.id;});
        auto phases=nemaSitePhases(type.phasing,n,*current,{major.begin(),major.end()});
        result.signalPhases=std::move(phases.numbers);result.issues.insert(result.issues.end(),phases.issues.begin(),phases.issues.end());
        if(phases.issues.empty()) {
            auto timing=nemaTiming(type.phasing,recipe.signal.value_or(JunctionSignal{}),d.definition?d.definition->timeStep:0,at+".signal",result.signalPhases);
            result.issues.insert(result.issues.end(),timing.issues.begin(),timing.issues.end());if(timing.issues.empty())result.signal=std::move(timing.controller);
        }
    }
    if(!result.issues.empty())return result;
    // Measure the stop lines on the final, full-width lane surfaces, preserving the base
    // lanes' footprint. Only the copy is widened here; actual pockets retain a taper upstream.
    auto measured=n;
    for(const auto& [id,arm]:arms) {
        auto& l=*std::find_if(measured.links.begin(),measured.links.end(),[&](const auto& x){return x.id==arm.in;});
        for(std::size_t i=0;i<arm.rows.size();++i)if(arm.rows[i].pocket) {
            const bool kerb=sideOf(arm.rows[i])==PocketSide::kerb;
            auto lanes=l.lanes; const auto width=(kerb?lanes.front():lanes.back()).width;
            lanes.insert(kerb?lanes.begin():lanes.end(),{"junction-measure-"+std::to_string(i),width});
            replaceLaneBundle(l,std::move(lanes),kerb);
        }
    }
    const auto cuts=type.legs==3?tLegCuts(std::move(measured),n,selected,major):nodeLegCuts(measured,selected,{});
    if(!cuts.reason.empty()){bad(cuts.reason,at);return result;}
    result.issues=cutReferenceIssues(d,cuts.cuts);
    if(!result.issues.empty())return result;
    try {
        for(const auto& cut:cuts.cuts) {
            auto& l=editableLink(d,cut.linkId); l.geometry=cut.geometry; l.generated=true;
        }
        reanchorConnectors(d);
        const auto before=n.links.size();
        std::map<std::string,std::vector<std::string>> roadLinks;
        for(const auto& r:n.streetSegments)roadLinks[r.id]=r.links;
        for(auto& [id,arm]:arms) {
            struct Pocket {double storage,taper;PocketSide side;}; std::vector<Pocket> pockets;std::size_t p=0;
            for(const auto& row:arm.rows)if(row.pocket) {
                const auto& leg=recipe.legs.at(id);
                pockets.push_back({*leg.storage[p],*leg.taper[p],sideOf(row)});++p;
            }
            std::stable_sort(pockets.begin(),pockets.end(),[](const auto& a,const auto& b){return a.storage>b.storage;});
            const auto originalIn=arm.in;
            for(const auto& pocket:pockets)arm.in=addTurnPocket(d,arm.in,pocket.storage,pocket.taper,pocket.side);
            if(arm.in!=originalIn) {
                auto& l=editableLink(d,arm.in);l.streetSegmentId=id;
                for(auto& old:roadLinks[id])if(old==originalIn)old=arm.in;
            }
        }
        for(std::size_t i=before;i<n.links.size();++i)n.links[i].generated=true;
        for(auto& r:n.streetSegments)r.links=roadLinks.at(r.id);
        // One contiguous range per movement, merging adjacent rows that share it.
        for(const auto& [id,arm]:arms) {
            std::map<char,std::pair<std::string,double>> targets;
            for(const auto& [other,out]:arms)if(other!=id) {
                const auto angle=sweep(link(d,arm.in),link(d,out.out),n.drivingSide); const auto m=movement(angle);
                if(m=='U'){bad("JUNCTION_UTURN",at+".legs."+id);continue;}
                if(targets.contains(m)){bad("JUNCTION_MOVEMENT_AMBIGUOUS",at+".legs."+id);continue;}
                targets[m]={out.out,angle};
            }
            for(const char m:std::string("FTK"))for(std::size_t i=0;i<arm.rows.size();) {
                if(arm.rows[i].movements.find(m)==std::string::npos){++i;continue;}
                const auto first=i;while(i<arm.rows.size()&&arm.rows[i].movements.find(m)!=std::string::npos)++i;
                if(!targets.contains(m)){bad("JUNCTION_MOVEMENT_MISSING",at+".legs."+id+".lanes["+std::to_string(first)+"].movements");continue;}
                const auto& from=link(d,arm.in);const auto& [out,angle]=targets.at(m);const auto& to=link(d,out);
                const auto count=static_cast<int>(i-first), target=std::min(count,static_cast<int>(to.lanes.size()));
                const auto fromIndex=arm.rows.size()-i, toIndex=m=='F'?to.lanes.size()-static_cast<std::size_t>(target):0;
                const auto connector=addConnectorRange(d,{from.id,from.lanes[fromIndex].id},{to.id,to.lanes[toIndex].id},count,target);
                editableConnector(d,connector).generated=true;
                result.turns[connector]={id,movementRank(major.contains(id),major.contains(*to.streetSegmentId),angle,arms.size()),!major.contains(id),{},false,result.turns.size()};
            }
        }
        if(!result.issues.empty())return result;
        fitJunctionPolygon(result,selected,cuts.cuts,explicitPolygonFit);
        if(!result.issues.empty())return result;
        const auto current=std::find_if(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return x.id==selected.id;});
        const auto layout=approachLayout(n,*current);
        const auto issues=fillLayoutIssues(n,*current,layout.issues);
        result.issues.insert(result.issues.end(),issues.begin(),issues.end());
        // Storage must remain outside the Node, so a widening taper cannot become a turn or
        // disappear from the derived pocket walk. Check the actual row order after both sides.
        for(const auto& [id,arm]:arms) {
            const auto a=std::find_if(layout.approaches.begin(),layout.approaches.end(),[&](const auto& x){return x.inLink==arm.in;});
            if(a==layout.approaches.end())continue;
            std::size_t p=0;
            for(std::size_t i=0;i<arm.rows.size();++i)if(arm.rows[i].pocket) {
                const auto& pocket=a->lanes.at(arm.rows.size()-1-i).pocket;
                const auto& leg=recipe.legs.at(id);
                if(!pocket||std::abs(pocket->storage-*leg.storage[p])>1e-6||std::abs(pocket->taper-*leg.taper[p])>1e-6)
                    bad("JUNCTION_POCKET_LAYOUT",at+".legs."+id+".storage["+std::to_string(p)+"]");
                ++p;
            }
        }
        if(result.issues.empty()&&result.signal)authorSignals(result,selected.id,{major.begin(),major.end()});
    } catch(const ValidationError& e) {result.issues.insert(result.issues.end(),e.issues.begin(),e.issues.end());}
      catch(const std::invalid_argument& e) {bad(std::string(e.what())=="EDIT_SPLIT_RANGE"?"JUNCTION_POCKET_RANGE":e.what(),at);}
    return result;
}
}
