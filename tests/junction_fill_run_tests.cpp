#include "junction_fill_fixture.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
using namespace multra;
using namespace junctionTest;
TEST(junctionfill,jt8_micro_seed42_and_hcm_lane_groups) {
    for (const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side); std::vector<std::string> entries;
        for (const auto& leg:nodeLinkLegs(d.network,node(d))) if (leg.in) entries.push_back(leg.linkId);
        CHECK(entries.size()==4 && d.network.connectors.empty());
        History h; h.reset(d); apply(h); d=h.document(); checkLayout(d);
        CHECK(d.definition->signalControllers.empty() && d.network.signalHeads.empty());
        for (const auto& id:entries) { VehicleInput input{"","","car",180,0,120}; input.linkId=id; putInput(d,input); }
        const auto data=test::root()/"data";
        const auto snapshot=compileDocument(d,data);
        const auto report=runSeed(snapshot.scenario,evaluationSpec(d,snapshot,data),42).report;
        CHECK(report.nodes.size()==1 && report.nodes[0].turns.size()==12 && report.nodes[0].vehicles>0 && report.nodes[0].issues.empty());
        // Owner-approved JT8 setup: signals are authored with the OLD commands only in this test.
        // This is not a template preset/NEMA implementation (J4), nor a TWSC HCM method (A4).
        CHECK(h.execute("editorSignalController",[&](auto& x) {
            const auto layout=approachLayout(x.network,node(x));
            SignalController c{"","JT8 test-only split phasing",100,0,{}};
            for (std::size_t i=0;i<layout.approaches.size();++i) {
                SignalGroup g; g.number=static_cast<int>(i)+1; g.greenStart=25*static_cast<double>(i); g.greenEnd=g.greenStart+20; c.groups.push_back(g);
            }
            const auto controller=putSignalController(x,c);
            for (const auto& connector:x.network.connectors) {
                const auto a=std::find_if(layout.approaches.begin(),layout.approaches.end(),[&](const auto& v){return v.inLink==connector.from.linkId;});
                if (a==layout.approaches.end()) continue; // taper, outside the Node
                for (const auto& path:connectorPaths(x.network,connector)) {
                    NetworkSignalHead head; head.connectorId=path.id; head.controllerId=controller;
                    head.groupNumber=static_cast<int>(a-layout.approaches.begin())+1; putSignalHead(x,head);
                }
            }
        }));
        CHECK(!h.document().network.signalHeads.empty());
        const auto compiled=compileNodeAnalysis(h.document().network,0,h.document().definition->signalControllers,{});
        CHECK(compiled.membership.turns.size()==12 && compiled.groups.size()==10 && compiled.advisories.empty());
        std::set<std::size_t> covered;
        for (const auto& g:compiled.groups) { CHECK(g.lanes>0); covered.insert(g.turns.begin(),g.turns.end()); }
        CHECK(covered.size()==12);
    }
}
