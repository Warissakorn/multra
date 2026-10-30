#include "junction_t_fixture.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
#include "../src/project/node_analysis_run.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
#include "../src/commands/right_of_way_commands.hpp"
using namespace multra;
using namespace junctionTTest;
TEST(junctionsignals,jt18_catalog_has_one_protected_t_without_site_defaults) {
    const auto entries=resolveJunctionTemplates(AuthoringDefinition{},test::root()/"data");CHECK(entries.size()>=14);
    std::vector<JunctionTemplate> ts;for(const auto& e:entries)if(e.type.legs==3&&e.type.control=="signalised")ts.push_back(e.type);
    CHECK(ts.size()==1&&junctionTemplateJson(ts.front())==junctionTemplateJson(type()));
}
namespace {
void fillT(DrivingSide side) {
    for(int stem=0;stem<4;++stem) {
        History h;h.reset(junction(side,stem));const auto before=bytes(h.document());
        const auto status=junctionFillStatus(h.document(),node(h.document()),catalogs(h.document()));
        const auto entered=site(side,stem);std::vector<int> expected;for(const auto& p:entered.phases)expected.push_back(p.number);
        CHECK(node(h.document()).junction->signal==entered&&expected.size()==4);
        CHECK(status.issues.empty()&&status.signalPhases==expected&&bytes(h.document())==before);apply(h);
        const auto& d=h.document();const auto layout=approachLayout(d.network,node(d));
        CHECK(layout.issues.empty()&&layout.approaches.size()==3&&layout.movements.size()==6&&seams(d).empty());
        const auto& c=d.definition->signalControllers[0];CHECK(c.groups.size()==4&&nemaCheck(c,{d.network,layout}).empty());
        // U0 compass numbers, written independently of nemaThroughPhase/nemaFarPhase.
        const std::array<const char*,4> leftFar{{"WBR","NBR","EBR","SBR"}},rightFar{{"EBL","SBL","WBL","NBL"}};
        const auto far=(side==DrivingSide::left?leftFar:rightFar).at(static_cast<std::size_t>(stem));
        for(const auto& m:layout.movements) {
            const auto programs=turnSignalPrograms(d.network,layout.membership,layout.membership.turns[m.turn]);CHECK(programs.size()==1);
            const int number=m.name==far?leading(side,stem):m.name.starts_with("NB")? (stem%2?2:8):
                m.name.starts_with("SB")?(stem%2?6:4):m.name.starts_with("WB")?(stem%2?4:2):(stem%2?8:6);
            CHECK(programs.contains(signalGroupProgramId(c.id,number)));
        }
        test::near(group(c,2).greenStart,0);test::near(group(c,6).greenStart,0);
        test::near(group(c,minor(stem)).greenStart,44);test::near(group(c,leading(side,stem)).greenStart,86);
        for(const auto& p:entered.phases){test::near(signalGroupGreen(c,group(c,p.number)),*p.green);test::near(group(c,p.number).amber,*p.yellow);}
        CHECK(node(d).junction->signal==entered&&d.network.rightOfWay.stopControls.empty());validateDocument(d);
        const auto after=bytes(d);CHECK(bytes(parseDocument(after))==after);h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
}
TEST(junctionsignals,jt18_three_stage_t_left_u0_compass_all_stems) {fillT(DrivingSide::left);}
TEST(junctionsignals,jt18_three_stage_t_right_u0_compass_all_stems) {fillT(DrivingSide::right);}
namespace {
void stopT(DrivingSide side) {
    auto d=junction(side);auto t=type();t.control="twoWayStop";t.phasing.clear();putJunctionTemplateCatalog(d,{t});
    node(d).junction->signal.reset();CHECK(nodeStreetSegmentLegs(d.network,node(d)).size()==3&&d.definition->crossSections[0].median==8);
    const auto before=bytes(d);History h;h.reset(d);CHECK(offers(d)[0].refusal.empty());apply(h);
    CHECK(h.document().network.streetSegments==d.network.streetSegments&&seams(h.document()).empty());validateDocument(h.document());
    for(const auto& l:h.document().network.links) {
        const auto& old=link(d,l.id);for(const auto p:{l.geometry.front(),l.geometry.back()}) {
            const auto on=pointAlong(old.geometry,stationOfClosestPoint(old.geometry,p));test::near(on.x,p.x);test::near(on.y,p.y);
        }
    }
    const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
}
}
TEST(junctionsignals,jt19_two_way_stop_t_measures_full_major_surface_left) {stopT(DrivingSide::left);}
TEST(junctionsignals,jt19_two_way_stop_t_measures_full_major_surface_right) {stopT(DrivingSide::right);}
TEST(junctionsignals,jt18_timing_missing_unused_wait_and_lane_guards) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=junction(side);const auto numbers=nemaSitePhases("P2",d.network,node(d),node(d).junction->major);
        CHECK(nodeLinkLegs(d.network,node(d)).size()==6&&node(d).junction->signal->phases.size()==4);
        CHECK(numbers.issues.empty()&&numbers.numbers.size()==4);
        auto s=site(side,0);const auto timing=nemaTiming("P2",s,0.1,"junction.signal",numbers.numbers);CHECK(timing.issues.empty());
        if(side==DrivingSide::right) {
            const auto six=std::find_if(timing.phases.begin(),timing.phases.end(),[](const auto& p){return p.number==6;});
            CHECK(six!=timing.phases.end());test::near(six->clearance,1);test::near(six->wait,56); // one phase in ring 2
        }
        auto zero=s;for(auto& p:zero.phases){p.yellow=0;p.allRed=0;}zero.cycle=88;
        CHECK(zero.phases[0].allRed==0&&nemaTiming("P2",zero,0.1,"junction.signal",numbers.numbers).issues.empty());
        s.phases.erase(s.phases.begin());CHECK(s.phases.size()==3);
        CHECK(issue(nemaTiming("P2",s,0.1,"junction.signal",numbers.numbers).issues,"JUNCTION_TIMING_MISSING"));
        node(d).junction->signal.reset();const auto missing=junctionFillStatus(d,node(d),catalogs(d));
        CHECK(missing.signalPhases==numbers.numbers&&std::count_if(missing.issues.begin(),missing.issues.end(),[](const auto& i){return i.code=="JUNCTION_TIMING_MISSING";})==13);
        auto pocket=junction(side,0,false,true);std::string far;
        for(const auto& leg:nodeLinkLegs(pocket.network,node(pocket)))if(leg.in) {
            const auto& l=link(pocket,leg.linkId);const auto u=directionAlong(l.geometry,polylineLength(l.geometry),true);
            if(u.x==(side==DrivingSide::left?-1.:1.))far=*l.streetSegmentId;
        }
        CHECK(!far.empty());auto& leg=node(pocket).junction->legs[far];leg.lanes=std::vector<JunctionLane>{{"F",true},{"T"},{"T"}};
        node(pocket).junction->signal.reset();const auto pending=junctionFillStatus(pocket,node(pocket),catalogs(pocket));
        CHECK(pending.signalPhases==numbers.numbers&&issue(pending.issues,"JUNCTION_STORAGE_MISSING")&&issue(pending.issues,"JUNCTION_TIMING_MISSING"));
        leg.storage={47.5};leg.taper={13.25};node(pocket).junction->signal=site(side,0);History ph;ph.reset(pocket);apply(ph);apply(ph);
        CHECK(seams(ph.document()).empty());const auto pl=approachLayout(ph.document().network,node(ph.document()));
        bool seen=false;for(const auto& a:pl.approaches)for(const auto& lane:a.lanes)if(lane.pocket){test::near(lane.pocket->storage,47.5);test::near(lane.pocket->taper,13.25);seen=true;}CHECK(seen);
        auto unsupported=junction(side);auto wrong=type();wrong.phasing="P1";putJunctionTemplateCatalog(unsupported,{wrong});
        CHECK(unsupported.definition->junctionTemplates[0].phasing=="P1");CHECK(issue(offers(unsupported)[0].issues,"NEMA_PRESET"));
        for(int mode=0;mode<3;++mode) {
            auto bad=junction(side);auto& signal=*node(bad).junction->signal;
            if(mode==0)signal.phases.push_back({leading(side,0)==1?5:1,10,3,1});
            if(mode==1)signal.cycle=101;
            if(mode==2)signal.phases[0].green=40.03;
            CHECK(signal!=site(side,0));const auto before=bytes(bad);History h;h.reset(bad);
            const std::string code=mode==0?"JUNCTION_SITE_COUNT":mode==1?"NEMA_CYCLE":"NEMA_GRID";
            CHECK(issue(offers(bad)[0].issues,code));test::throws([&]{apply(h);},code);CHECK(bytes(h.document())==before&&!h.canUndo());
        }
    }
}
TEST(junctionsignals,jt19_t_cut_station_references_survive_or_refuse_atomically) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=junction(side);std::string incoming,outgoing;
        for(const auto& leg:nodeLinkLegs(d.network,node(d)))if(link(d,leg.linkId).streetSegmentId=="stem")(leg.in?incoming:outgoing)=leg.linkId;
        CHECK(!incoming.empty()&&!outgoing.empty());putTravelTimeSection(d,{"","Keep",{incoming,5},{incoming,15},{}});
        const auto sections=d.network.travelTimeSections;CHECK(sections.size()==1);History h;h.reset(d);const auto before=bytes(d);apply(h);
        CHECK(h.document().network.travelTimeSections==sections);h.undo();CHECK(bytes(h.document())==before);
        const auto program=putProgram(d,{"",0,{{30,SignalColor::green},{30,SignalColor::red}}});
        const auto id=putSignalHead(d,{"",{outgoing,link(d,outgoing).lanes[0].id},40,program,{}});
        CHECK(d.network.signalHeads.size()==1&&d.network.signalHeads[0].position==40);h.reset(d);const auto held=bytes(d);
        CHECK(issue(offers(d)[0].issues,"JUNCTION_EDITED",id));test::throws([&]{apply(h);},"JUNCTION_EDITED");CHECK(bytes(h.document())==held&&!h.canUndo());
    }
}
TEST(junctionsignals,jt18_t_checker_corruptions_and_physical_major_pair) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=junction(side);auto invalid=d;const std::vector<std::string> main{node(d).junction->major[0],"stem"};int index=0;
        for(const auto& leg:nodeLinkLegs(d.network,node(d)))if(leg.in) {
            const auto& l=link(d,leg.linkId);const bool m=std::find(main.begin(),main.end(),*l.streetSegmentId)!=main.end();
            node(invalid).approachDirections[l.id]=m?(index++==0?ApproachDirection::north:ApproachDirection::south):ApproachDirection::east;
        }
        CHECK(index==2&&node(invalid).approachDirections.size()==3);
        CHECK(issue(nemaSitePhases("P2",invalid.network,node(invalid),main).issues,"NEMA_MAJOR_PAIR"));
        History h;h.reset(d);apply(h);d=h.document();const auto layout=approachLayout(d.network,node(d));const auto c=d.definition->signalControllers[0];
        CHECK(layout.movements.size()==6&&nemaCheck(c,{d.network,layout}).empty());
        auto plain=d;plain.network.streetSegments.clear();node(plain).junction.reset();for(auto& l:plain.network.links)l.streetSegmentId.reset();
        const auto drawn=approachLayout(plain.network,node(plain));CHECK(drawn.movements.size()==6&&nemaCheck(c,{plain.network,drawn}).empty());
        auto barrier=c;group(barrier,4).greenStart=0;group(barrier,4).greenEnd=38;
        CHECK(group(barrier,4).greenStart==group(barrier,2).greenStart);CHECK(issue(nemaCheck(barrier,{d.network,layout}),"NEMA_BARRIER"));
        auto absent=c;std::erase_if(absent.groups,[](const auto& g){return g.number==4;});CHECK(absent.groups.size()==3);
        CHECK(issue(nemaCheck(absent,{d.network,layout}),"NEMA_PHASE_NUMBER"));
        d.network.signalHeads[0].groupNumber=9;CHECK(d.network.signalHeads[0].groupNumber==9);
        CHECK(issue(nemaCheck(c,{d.network,layout}),"NEMA_HEAD_GROUP"));
        auto shared=junction(side);for(const auto& leg:nodeLinkLegs(shared.network,node(shared)))if(leg.in) {
            const auto& l=link(shared,leg.linkId);const auto travel=directionAlong(l.geometry,polylineLength(l.geometry),true);
            if(travel.x==(side==DrivingSide::left?-1.:1.))node(shared).junction->legs[*l.streetSegmentId].lanes=std::vector<JunctionLane>{{"FT"},{"T"}};
        }
        CHECK(node(shared).junction->legs.size()==1);CHECK(issue(offers(shared)[0].issues,"NEMA_LANE_GROUP"));
    }
}
TEST(junctionsignals,jt18_installed_t_reapply_refine_and_reference_guards) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(junction(side,0,true));apply(h);const auto before=bytes(h.document());apply(h);
        CHECK(seams(h.document()).empty()&&h.document().network.signalHeads.size()==6);const auto after=bytes(h.document());
        h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
        const auto id=h.document().definition->signalControllers[0].id;
        CHECK(h.execute("Edit controller",[](auto& d){d.definition->signalControllers[0].name="Owner";}));
        CHECK(!h.document().definition->signalControllers[0].generated);const auto edited=bytes(h.document());
        CHECK(issue(offers(h.document())[0].issues,"JUNCTION_EDITED",id)&&issue(seams(h.document()),"SEAM_JUNCTION_DIFFERS"));
        test::throws([&]{apply(h);},"JUNCTION_EDITED");CHECK(bytes(h.document())==edited);
        CHECK(h.execute("Apply again",[&](auto& d){reapplyJunction(d,node(d).id,catalogs(d));},true));CHECK(seams(h.document()).empty());
        h.undo();CHECK(bytes(h.document())==edited);h.redo();
        std::string counter;CHECK(h.execute("Measure head",[&](auto& d){counter=putQueueCounter(d,{"","Keep",{{d.network.signalHeads[0].id,{}}}});}));
        CHECK(!counter.empty()&&h.document().network.queueCounters.size()==1);const auto measured=bytes(h.document());
        CHECK(issue(junctionReapplyPreview(h.document(),node(h.document()).id,catalogs(h.document())).issues,"JUNCTION_EDITED",counter));
        test::throws([&]{apply(h);},"JUNCTION_EDITED");CHECK(bytes(h.document())==measured);
    }
}
TEST(junctionsignals,jt18_t_hcm_and_micro_read_real_heads) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(junction(side));apply(h);auto d=h.document();
        const auto compiled=compileNodeAnalysis(d.network,0,d.definition->signalControllers,{});
        CHECK(compiled.membership.turns.size()==6&&compiled.groups.size()==6&&compiled.advisories.empty());
        node(d).analysis=NodeAnalysis{};
        for(const auto& t:compiled.membership.turns)node(d).analysis->turns.push_back({compiled.membership.inLegs[t.in].linkId,compiled.membership.outLegs[t.out].linkId,100});
        const auto run=runNodeAnalysis(d,test::root()/"data");CHECK(run.nodes.size()==1&&run.nodes[0].groups.size()==6);
        for(const auto& g:run.nodes[0].groups){CHECK(g.result);test::near(g.result->greenRatio,g.group.displayedGreen/100);}
        for(const auto& l:compiled.membership.inLegs){VehicleInput input{"","","car",180,0,120};input.linkId=l.linkId;putInput(d,input);}
        const auto data=test::root()/"data";const auto snapshot=compileDocument(d,data);
        const auto report=runSeed(snapshot.scenario,evaluationSpec(d,snapshot,data),42).report;
        CHECK(report.nodes.size()==1&&report.nodes[0].turns.size()==6&&report.nodes[0].vehicles>0&&report.nodes[0].issues.empty());
    }
}
