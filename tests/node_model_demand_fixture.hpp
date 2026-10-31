#pragma once
#include "junction_signal_fixture.hpp"
#include "../src/commands/node_analysis_commands.hpp"
namespace nodeModelTest {
using namespace multra;
inline ProjectDocument fixture(DrivingSide side) {
    History h;h.reset(junctionSignalTest::signalCross(side,"P3"));junctionTest::apply(h);
    auto d=h.document();const auto m=nodeMembership(d.network,d.network.nodes[0]);
    CHECK(m.issues.empty()&&m.turns.size()==12);
    auto& def=*d.definition;
    std::vector<std::string> objects;for(const auto& span:m.turns[0].paths[0])objects.push_back(span.objectId);
    def.routes.push_back({"counted-turn",objects});
    VehicleInput car{"cars","counted-turn","car",1200,0,120};
    car.intervals={{0,60,600},{60,120,1800}};def.inputs.push_back(car);
    def.inputs.push_back({"trucks","counted-turn","rigid-truck",600,0,120});
    NodeAnalysis a;a.cbd=true;a.baseSaturationFlow=SaturationFlowSource{1825,"site measurement"};
    const auto& last=m.turns.back();a.turns.push_back({m.inLegs[last.in].linkId,m.outLegs[last.out].linkId,99,7});
    setNodeAnalysis(d,d.network.nodes[0].id,a);return d;
}
}
