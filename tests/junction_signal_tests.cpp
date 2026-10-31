#include "junction_signal_fixture.hpp"
using namespace multra;
using namespace junctionSignalTest;
TEST(junctionsignals,jt9_all_presets_both_sides_axes_and_pockets_one_undo) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* preset:{"P1","P2","P3","P4"})
    for(const bool ew:{false,true})for(const bool pockets:{false,true}) {
        auto d=signalCross(side,preset,ew,pockets);const auto before=bytes(d);History h;h.reset(d);
        CHECK(node(d).junction->signal->cycle&&node(d).junction->signal->phases[0].yellow&&node(d).junction->signal->phases[0].allRed);
        const auto rows=offers(d);CHECK(rows.size()==1&&rows[0].refusal.empty());apply(h);
        const auto& after=h.document();const auto layout=approachLayout(after.network,node(after));
        CHECK(after.definition->signalControllers.size()==1&&after.network.signalHeads.size()>=12&&layout.issues.empty());
        const auto& c=after.definition->signalControllers[0];CHECK(c.generated&&c.offset==0&&c.cycle==*node(d).junction->signal->cycle);
        CHECK(node(after).junction==node(d).junction); // Clearance stays entered; waiting is only derived.
        CHECK(c.groups.size()==node(d).junction->signal->phases.size()&&nemaCheck(c,{after.network,layout}).empty());
        const auto mapped=nemaApproachPhases(after.network,layout,node(after).junction->major);
        CHECK(mapped.issues.empty()&&mapped.through.size()==4);
        for(const auto& a:layout.approaches) {
            const int expected=ew?(a.direction==ApproachDirection::west?2:a.direction==ApproachDirection::east?6:a.direction==ApproachDirection::south?4:8):
                (a.direction==ApproachDirection::north?2:a.direction==ApproachDirection::south?6:a.direction==ApproachDirection::west?4:8);
            CHECK(mapped.through.at(a.inLink)==expected);
        }
        for(const auto& p:node(d).junction->signal->phases) {
            const auto& g=group(c,p.number);test::near(signalGroupGreen(c,g),*p.green);test::near(g.amber,*p.yellow);
        }
        test::near(group(c,2).greenStart,0);test::near(group(c,6).greenStart,0);
        test::near(group(c,4).greenStart,std::string(preset)=="P3"?41:33);
        test::near(group(c,8).greenStart,std::string(preset)=="P4"?56:std::string(preset)=="P3"?43:33);
        if(std::string(preset)=="P2"||std::string(preset)=="P3") {
            test::near(group(c,1).greenStart,c.cycle-9);test::near(group(c,5).greenStart,c.cycle-13);
        }
        CHECK(after.network.rightOfWay.stopControls.empty());
        for(const auto& head:after.network.signalHeads)CHECK(head.generated&&head.controllerId==c.id&&head.position==0);
        const auto saved=bytes(after);CHECK(bytes(parseDocument(saved))==saved);
        h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==saved);
    }
}
TEST(junctionsignals,jt9_timing_clearance_wait_cycle_missing_and_grid) {
    auto site=timing("P2");CHECK(site.cycle==69&&site.phases[0].allRed==1);
    const auto t=nemaTiming("P2",site,0.1);CHECK(t.issues.empty());
    CHECK(t.phases.size()==6);const auto six=std::find_if(t.phases.begin(),t.phases.end(),[](const auto& p){return p.number==6;});
    CHECK(six!=t.phases.end());test::near(six->clearance,1);test::near(six->wait,6);
    auto missing=site;missing.phases[0].yellow.reset();missing.phases[1].allRed.reset();
    CHECK(!missing.phases[0].yellow&&!missing.phases[1].allRed);
    const auto m=nemaTiming("P2",missing,0.1);CHECK(issue(m.issues,"JUNCTION_TIMING_MISSING","yellow")&&issue(m.issues,"JUNCTION_TIMING_MISSING","allRed"));
    auto cycle=site;cycle.cycle=70;CHECK(cycle.cycle!=site.cycle);CHECK(issue(nemaTiming("P2",cycle,0.1).issues,"NEMA_CYCLE"));
    auto grid=site;grid.phases[0].green=6.03;CHECK(std::fmod(*grid.phases[0].green,0.1)!=0);CHECK(issue(nemaTiming("P2",grid,0.1).issues,"NEMA_GRID"));
    auto zero=timing("P1");for(auto& p:zero.phases){p.yellow=0;p.allRed=0;}zero.cycle=50;
    CHECK(zero.phases[0].yellow==0&&zero.phases[0].allRed==0);
    const auto z=nemaTiming("P1",zero,0.1);CHECK(z.issues.empty());for(const auto& p:z.controller.groups)CHECK(p.amber==0);
}
TEST(junctionsignals,jt9_checker_names_broken_barrier_ring_and_head) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(signalCross(side,"P3"));apply(h);auto d=h.document();
        const auto l=approachLayout(d.network,node(d));const auto c=d.definition->signalControllers[0];
        CHECK(nemaCheck(c,{d.network,l}).empty());
        auto plain=d;plain.network.streetSegments.clear();node(plain).junction.reset();
        for(auto& link:plain.network.links)link.streetSegmentId.reset();
        const auto drawn=approachLayout(plain.network,node(plain));
        CHECK(plain.network.streetSegments.empty()&&drawn.issues.empty()&&drawn.movements.size()==12);
        CHECK(nemaCheck(c,{plain.network,drawn}).empty());
        auto barrier=c;group(barrier,8).greenStart=0;group(barrier,8).greenEnd=18;
        CHECK(group(barrier,8).greenStart==group(barrier,2).greenStart);CHECK(issue(nemaCheck(barrier,{d.network,l}),"NEMA_BARRIER"));
        auto order=c;group(order,1).greenStart=34;group(order,1).greenEnd=40;
        CHECK(group(order,1).greenStart>group(order,2).greenStart);CHECK(issue(nemaCheck(order,{d.network,l}),"NEMA_RING_ORDER"));
        d.network.signalHeads[0].groupNumber=9;CHECK(d.network.signalHeads[0].groupNumber==9);
        CHECK(issue(nemaCheck(c,{d.network,l}),"NEMA_HEAD_GROUP",d.network.signalHeads[0].id));
    }
}
TEST(junctionsignals,jt9_direction_labels_cannot_make_adjacent_major_legs_opposing) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side,false));apply(h);auto d=h.document();
        const auto original=approachLayout(d.network,node(d));CHECK(original.issues.empty());
        std::vector<std::string> major;
        for(const auto& a:original.approaches) {
            if(a.direction==ApproachDirection::north||a.direction==ApproachDirection::east)major.push_back(*link(d,a.inLink).streetSegmentId);
            node(d).approachDirections[a.inLink]=a.direction==ApproachDirection::east?ApproachDirection::south:a.direction==ApproachDirection::south?ApproachDirection::east:a.direction;
        }
        const auto changed=approachLayout(d.network,node(d));CHECK(changed.issues.empty()&&major.size()==2);
        std::set<ApproachDirection> labels;for(const auto& a:changed.approaches)if(std::find(major.begin(),major.end(),*link(d,a.inLink).streetSegmentId)!=major.end())labels.insert(a.direction);
        CHECK(labels==std::set{ApproachDirection::north,ApproachDirection::south});
        const auto map=nemaApproachPhases(d.network,changed,major);CHECK(issue(map.issues,"NEMA_MAJOR_PAIR"));
    }
}
