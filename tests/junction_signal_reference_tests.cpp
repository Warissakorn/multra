#include "junction_signal_fixture.hpp"
#include "../src/commands/right_of_way_commands.hpp"
using namespace multra;
using namespace junctionSignalTest;
TEST(junctionsignals,jt9_missing_timing_and_protected_shared_lane_refuse_atomically) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool missing:{false,true}) {
        auto d=signalCross(side,"P2",false,false);
        if(missing)node(d).junction->signal->phases[0].yellow.reset();
        else node(d).junction->legs[node(d).junction->major[0]].lanes=std::vector<JunctionLane>{{"FT"},{"TK"}};
        CHECK(missing?!node(d).junction->signal->phases[0].yellow:node(d).junction->legs.begin()->second.lanes.has_value());
        if(!missing) {
            const auto& road=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==node(d).junction->major[0];});
            CHECK(link(d,road.links[0]).lanes.size()==node(d).junction->legs.at(road.id).lanes->size());
        }
        const auto before=bytes(d);History h;h.reset(d);const auto rows=offers(d);CHECK(rows.size()==1&&!rows[0].refusal.empty());
        const std::string code=missing?"JUNCTION_TIMING_MISSING":"NEMA_LANE_GROUP";
        if(!issue(rows[0].issues,code)){std::string why;for(const auto& i:rows[0].issues)why+=i.code+" at "+i.path+"; ";throw std::runtime_error(why);}
        CHECK(issue(rows[0].issues,code));
        test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},code);
        CHECK(!h.canUndo()&&bytes(h.document())==before);
    }
}
TEST(junctionsignals,jt9_reapply_generated_signals_and_exact_undo) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* preset:{"P1","P2","P3","P4"}) {
        History h;h.reset(signalCross(side,preset));apply(h);
        const auto before=bytes(h.document());const auto id=h.document().definition->signalControllers[0].id;
        const auto heads=h.document().network.signalHeads.size();
        CHECK(h.document().definition->signalControllers[0].generated&&heads>=12);
        apply(h);const auto& after=h.document();
        CHECK(after.definition->signalControllers.size()==1&&after.definition->signalControllers[0].generated&&after.definition->signalControllers[0].id!=id);
        CHECK(after.network.signalHeads.size()==heads&&std::all_of(after.network.signalHeads.begin(),after.network.signalHeads.end(),[](const auto& x){return x.generated;}));
        const auto layout=approachLayout(after.network,node(after));CHECK(nemaCheck(after.definition->signalControllers[0],{after.network,layout}).empty());
        const auto saved=bytes(after);h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==saved);
    }
}
TEST(junctionsignals,jt9_edited_controller_head_and_new_references_refuse_replacement) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const int edit:{0,1,2,3}) {
        History h;h.reset(signalCross(side,"P3"));apply(h);std::string edited;
        CHECK(h.execute("manual",[&](auto& d) {
            if(edit==0){auto& head=d.network.signalHeads[0];edited=head.id;head.name="Manual head";}
            if(edit==1){auto& c=d.definition->signalControllers[0];edited=c.id;c.name="Manual controller";}
            if(edit==2){const auto& l=d.network.links[0];NetworkSignalHead head;head.lane={l.id,l.lanes[0].id};head.position=5;head.controllerId=d.definition->signalControllers[0].id;head.groupNumber=2;edited=putSignalHead(d,head);}
            if(edit==3)edited=putQueueCounter(d,{"","Manual measurement",{{d.network.signalHeads[0].id,{}}}});
        }));
        if(edit==0)CHECK(!h.document().network.signalHeads[0].generated);
        if(edit==1)CHECK(!h.document().definition->signalControllers[0].generated);
        if(edit==2)CHECK(h.document().network.signalHeads.back().id==edited&&!h.document().network.signalHeads.back().generated);
        if(edit==3)CHECK(h.document().network.queueCounters.back().id==edited);
        const auto before=bytes(h.document());const auto rows=offers(h.document());CHECK(rows.size()==1&&issue(rows[0].issues,"JUNCTION_EDITED",edited));
        test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},"JUNCTION_EDITED");
        CHECK(bytes(h.document())==before);
    }
}
