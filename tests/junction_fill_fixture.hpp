#pragma once
#include "test.hpp"
#include "../src/commands/fill_commands.hpp"
#include "../src/commands/junction_commands.hpp"
#include "../src/commands/junction_fill.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/model/network/approach_layout.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/project/junction_template_codec.hpp"
#include <fstream>
#include <set>
namespace junctionTest {
using namespace multra;
inline const Node& node(const ProjectDocument& d) { return d.network.nodes.front(); }
inline Node& node(ProjectDocument& d) { return d.network.nodes.front(); }
inline const Link& link(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.links.begin(),d.network.links.end(),[&](const auto& l) { return l.id==id; });
}
inline NodeFillCatalogs catalogs(const ProjectDocument& d) {
    NodeFillCatalogs c{d.definition->crossSections,{}, {3.0,7.0}};
    for (const auto& t:resolveJunctionTemplates(*d.definition,test::root()/"data")) c.junctionTemplates.push_back(t.type);
    return c;
}
inline ProjectDocument cross(DrivingSide side=DrivingSide::left, bool pockets=true,bool spacious=true,bool attachRecipe=true) {
    std::ifstream f(test::root()/"data/projects/t-junction-priority.traffic.json"); Json j; f>>j;
    CHECK(f.good() || f.eof());
    ProjectDocument d; d.definition=*parseDocument(j).definition;
    auto& def=*d.definition;
    def.inputs.clear(); def.routes.clear(); def.routingDecisions.clear(); def.crossSections.clear();
    def.signalControllers.clear(); def.signalPrograms.clear(); def.duration=120; def.evaluation.reset();
    d.network.drivingSide=side;
    putCrossSection(d,{"test","Test",2,2,2,3.5,{},8,std::nullopt,"Test values only",50},{});
    putStreetSegment(d,{"ns","NS",{{0,-180},{0,180}},std::string("test"),0,{},{}});
    putStreetSegment(d,{"ew","EW",{{-180,0},{180,0}},std::string("test"),0,{},{}});
    const auto places=findCrossings(d.network,2);
    CHECK(places.size()==1 && places[0].legs==4); // forcing: the ordinary crossing workflow
    const auto ids=applyCrossings(d,places,{CrossingChoice::junction},2,def.crossSections);
    CHECK(ids.size()==1 && d.network.streetSegments.size()==4);
    // A deliberately authored polygon with room for a kerb-side pocket as well as the base arms.
    if(spacious)node(d).polygon={{-30,-30},{30,-30},{30,30},{-30,30}};
    for (const auto& r:d.network.streetSegments) applyTemplate(d,r.id,def.crossSections);
    NodeJunction recipe; recipe.templateId=pockets?"stop-4-major-far-pockets":"stop-4-shared";
    for (const auto& r:d.network.streetSegments) if (r.centreline.front().x==0 && r.centreline.back().x==0) {
        recipe.major.push_back(r.id);
        if (pockets) recipe.legs[r.id]={{47.5},{13.25},std::nullopt};
    }
    if(attachRecipe)setNodeJunction(d,ids[0],recipe);
    CHECK(!nodeHasDetail(d.network,node(d)) && nodeLinkLegs(d.network,node(d)).size()==8);
    return d;
}
inline std::vector<FillOffer> offers(const ProjectDocument& d) {
    auto out=fillOffers(d,catalogs(d));
    std::erase_if(out,[](const auto& x) { return x.kind!=FillKind::junction; }); return out;
}
inline Json bytes(const ProjectDocument& d) { return documentJson(d); }
inline bool issue(const std::vector<ValidationIssue>& issues,const std::string& code,const std::string& path={}) {
    return std::any_of(issues.begin(),issues.end(),[&](const auto& x) { return x.code==code && (path.empty() || x.path.find(path)!=std::string::npos); });
}
inline void apply(History& h) {
    const auto chosen=offers(h.document()); CHECK(chosen.size()==1);
    if(!chosen[0].refusal.empty()) {
        std::string reasons;
        for(const auto& x:chosen[0].issues)reasons+=x.code+" at "+x.path+"; ";
        throw std::runtime_error(reasons);
    }
    CHECK(h.execute("editorFillDefaults",[&](auto& d) { applyFills(d,chosen,catalogs(d)); },true));
}
inline void checkLayout(const ProjectDocument& d) {
    const auto layout=approachLayout(d.network,node(d));
    CHECK(layout.issues.empty() && layout.approaches.size()==4 && layout.movements.size()==12);
    const auto& recipe=*node(d).junction;
    std::size_t pockets=0;
    for (const auto& a:layout.approaches) {
        const auto& arm=*link(d,a.inLink).streetSegmentId;
        const bool major=std::find(recipe.major.begin(),recipe.major.end(),arm)!=recipe.major.end();
        auto c=catalogs(d);
        const auto t=std::find_if(c.junctionTemplates.begin(),c.junctionTemplates.end(),[&](const auto& x){return x.id==recipe.templateId;});
        const auto row=recipe.legs.find(arm);
        const auto& rows=row!=recipe.legs.end() && row->second.lanes ? *row->second.lanes : t->roles.at(major?"major":"minor");
        CHECK(rows.size()==a.lanes.size());
        std::size_t p=0;
        for (std::size_t i=0;i<rows.size();++i) {
            const auto& lane=a.lanes[a.lanes.size()-1-i]; std::string actual;
            for (const char movement:std::string("FTK")) for (auto turn:lane.turns) {
                const auto m=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& x){return x.turn==turn;});
                const auto geometric=movement=='T'?JunctionMovement::through:
                    ((movement=='F')==(d.network.drivingSide==DrivingSide::left)?JunctionMovement::right:JunctionMovement::left);
                if (m->movement==geometric) actual+=movement;
            }
            CHECK(actual==rows[i].movements && lane.pocket.has_value()==rows[i].pocket);
            if (rows[i].pocket) {
                CHECK(row!=recipe.legs.end());
                test::near(lane.pocket->storage,*row->second.storage[p],1e-6);
                test::near(lane.pocket->taper,*row->second.taper[p++],1e-6);
                CHECK(lane.pocket->side==rows[i].side.value_or(rows[i].movements=="F"?PocketSide::far:PocketSide::kerb)); ++pockets;
            }
        }
    }
    CHECK(pockets>=2);
}
}
