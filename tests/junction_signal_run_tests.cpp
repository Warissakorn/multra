#include "junction_signal_fixture.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
#include "../src/project/node_analysis_run.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
using namespace multra;
using namespace junctionSignalTest;
TEST(junctionsignals,jt9_hcm_reads_the_entered_greens_and_cycle_for_every_preset) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* preset:{"P1","P2","P3","P4"}) {
        History h;h.reset(signalCross(side,preset));apply(h);auto d=h.document();
        const auto& c=d.definition->signalControllers[0];
        const auto compiled=compileNodeAnalysis(d.network,0,d.definition->signalControllers,{});
        CHECK(compiled.membership.turns.size()==12&&!compiled.groups.empty()&&compiled.advisories.empty());
        std::set<std::size_t> covered;
        for(const auto& g:compiled.groups) {
            const auto p=std::find_if(c.groups.begin(),c.groups.end(),[&](const auto& x){return signalGroupProgramId(c.id,x.number)==g.program;});
            CHECK(p!=c.groups.end());
            const auto entered=std::find_if(node(d).junction->signal->phases.begin(),node(d).junction->signal->phases.end(),[&](const auto& x){return x.number==p->number;});
            CHECK(entered!=node(d).junction->signal->phases.end());
            test::near(g.cycle,*node(d).junction->signal->cycle);test::near(g.displayedGreen,*entered->green);
            test::near(g.effectiveGreen/g.cycle,*entered->green/ *node(d).junction->signal->cycle);
            covered.insert(g.turns.begin(),g.turns.end());
        }
        CHECK(covered.size()==12);
        if(std::string(preset)=="P3") {
            node(d).analysis=NodeAnalysis{};
            for(const auto& t:compiled.membership.turns)node(d).analysis->turns.push_back({compiled.membership.inLegs[t.in].linkId,compiled.membership.outLegs[t.out].linkId,100});
            const auto run=runNodeAnalysis(d,test::root()/"data");CHECK(run.nodes.size()==1&&!run.nodes[0].groups.empty());
            for(const auto& g:run.nodes[0].groups){CHECK(g.result);test::near(g.result->greenRatio,g.group.displayedGreen/c.cycle);}
        }
    }
}
TEST(junctionsignals,jt9_generated_controller_runs_micro_seed42) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=signalCross(side,"P3");std::vector<std::string> entries;
        for(const auto& l:nodeLinkLegs(d.network,node(d)))if(l.in)entries.push_back(l.linkId);
        CHECK(entries.size()==4);History h;h.reset(d);apply(h);d=h.document();
        CHECK(!d.network.signalHeads.empty()&&d.network.rightOfWay.stopControls.empty());
        for(const auto& id:entries){VehicleInput input{"","","car",180,0,120};input.linkId=id;putInput(d,input);}
        const auto data=test::root()/"data";const auto snapshot=compileDocument(d,data);
        const auto report=runSeed(snapshot.scenario,evaluationSpec(d,snapshot,data),42).report;
        CHECK(report.nodes.size()==1&&report.nodes[0].turns.size()==12&&report.nodes[0].vehicles>0&&report.nodes[0].issues.empty());
    }
}
