#include "node_model_demand_fixture.hpp"
#include "../src/project/node_model_demand.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <fstream>
using namespace multra;
namespace {
ProjectDocument shipped(DrivingSide side) {
    std::ifstream f(test::root()/"data/projects/m2.6-study-template.traffic.json");Json j;f>>j;
    auto d=parseDocument(j);
    if(side==DrivingSide::right) {
        for(auto& l:d.network.links)for(auto& p:l.geometry)p.y=-p.y;
        for(auto& c:d.network.connectors)for(auto& p:c.geometry)p.y=-p.y;
        d.network.drivingSide=side;
    }
    const auto nodes=suggestNodes(d);CHECK(nodes.size()==1);
    CHECK(nodeMembership(d.network,d.network.nodes[0]).issues.empty());return d;
}
void study(DrivingSide side) {
    const auto d=shipped(side);const auto s=compileDocument(d,test::root()/"data");
    const auto stub=std::find_if(s.scenario.routeDeadEnds.begin(),s.scenario.routeDeadEnds.end(),[](const auto& x){return x.routeId=="link:link-1/path-4";});
    CHECK(stub!=s.scenario.routeDeadEnds.end()&&s.routeOrigins.at(stub->routeId)==stub->routeId);
    std::set<std::string> targets;for(const auto& span:s.scenario.laneChanges)if(span.fromRouteId==stub->routeId)targets.insert(span.toRouteId);
    CHECK(targets==std::set<std::string>{"link:link-1/path-3"}); // forcing: singleton origin cannot resolve this stub
    for(const auto& window:{std::pair{0.,900.},std::pair{900.,1800.},std::pair{0.,3600.}}) {
        const auto p=previewNodeModelDemand(documentJson(d).dump(),d.network.nodes[0].id,{window.first,window.second,{"heavy-vehicle"}},test::root()/"data");
        double actual=0,expected=0;for(const auto& t:p.turns)actual+=t.vehiclesPerHour;
        for(const auto& i:s.scenario.inputs)expected+=i.vehiclesPerHour*std::max(0.,std::min(window.second,i.endTime)-std::max(window.first,i.startTime))/(window.second-window.first);
        test::near(actual,expected,1e-8);test::near(actual,2040,1e-8);
    }
}
ProjectDocument pocket(DrivingSide side,bool ambiguous) {
    auto d=nodeModelTest::fixture(side); // keep only the run/catalog settings
    d.network=Network{"pocket",side,{},{},{},{}};d.definition->inputs.clear();d.definition->routes.clear();d.definition->routingDecisions.clear();d.definition->signalControllers.clear();d.definition->signalPrograms.clear();
    const double y=side==DrivingSide::left?1.:-1.;
    const auto line=[&](double a,double b,double c,double e,int n){return addLink(d,{{a,b*y},{c,e*y}},n,3.5);};
    const auto in=line(-300,7.25,-80,7.25,2),body=line(-60,5.5,-16,5.5,3);
    const auto through=line(16,3.75,300,3.75,2),kerb=line(-3.75,16,-3.75,300,2),far=line(3.75,-16,3.75,-300,2);
    const auto lane=[&](const std::string& id,int k)->std::string{return junctionTest::link(d,id).lanes.at(k).id;};
    addConnectorRange(d,{in,lane(in,0)},{body,lane(body,0)},2,2);
    addConnector(d,{in,lane(in,1)},{body,lane(body,2)});
    addConnectorRange(d,{body,lane(body,0)},{through,lane(through,0)},2,2);
    addConnector(d,{body,lane(body,0),37},{kerb,lane(kerb,0)});
    addConnector(d,{body,lane(body,2)},{far,lane(far,1)});
    if(!ambiguous) {
        RoutingDecision decision;decision.id="turns";decision.linkId=in;
        decision.routes={{"",6,through,{}},{"",2,kerb,{}},{"",2,far,{}}};d.definition->routingDecisions={decision};
    } else {
        const auto sink=line(500,0,600,0,2);
        addConnector(d,{kerb,lane(kerb,0)},{sink,lane(sink,0)});
        addConnector(d,{far,lane(far,1)},{sink,lane(sink,1)});
        RoutingDecision decision;decision.id="same-destination";decision.linkId=body;
        decision.routes={{"",1,sink,{}}};d.definition->routingDecisions={decision};
        deleteObjects(d,{in}); // the three-lane ambiguous control enters on body itself
    }
    VehicleInput input{"source",{},"car",1200,0,120};input.linkId=ambiguous?body:in;d.definition->inputs={input};
    putNode(d,{"","Pocket",{{-30,-30},{30,-30},{30,30},{-30,30}},false});return d;
}
void hand(DrivingSide side) {
    const auto d=pocket(side,false);const auto s=compileDocument(d,test::root()/"data");
    bool forcing=false;
    for(const auto& dead:s.scenario.routeDeadEnds) {
        const auto origin=s.routeOrigins.at(dead.routeId);
        const auto count=std::count_if(s.routeOrigins.begin(),s.routeOrigins.end(),[&](const auto& x){return x.second==origin;});
        if(count==1&&std::any_of(s.scenario.laneChanges.begin(),s.scenario.laneChanges.end(),[&](const auto& x){return x.fromRouteId==dead.routeId;}))forcing=true;
    }
    CHECK(forcing);const auto p=previewNodeModelDemand(documentJson(d).dump(),d.network.nodes[0].id,{0,120,{}},test::root()/"data");
    CHECK(p.turns.size()==3);std::multiset<double> volumes;for(const auto& t:p.turns)volumes.insert(t.vehiclesPerHour);
    CHECK(volumes==std::multiset<double>{240,240,720}); // 1200 * (2,2,6)/10, independent of source lanes/stubs
}
void ambiguous(DrivingSide side) {
    const auto d=pocket(side,true);const auto s=compileDocument(d,test::root()/"data");
    const auto m=nodeMembership(s.network,s.network.nodes[0]);const auto table=runtimeSections(s.network);bool forcing=false;
    for(const auto& dead:s.scenario.routeDeadEnds) {
        std::set<std::string> targets,exits;for(const auto& c:s.scenario.laneChanges)if(c.fromRouteId==dead.routeId)targets.insert(c.toRouteId);
        if(targets.size()<2||std::none_of(s.scenario.inputs.begin(),s.scenario.inputs.end(),[&](const auto& i){return i.routeId==dead.routeId&&i.vehiclesPerHour>0;}))continue;
        bool complete=true;
        for(const auto& id:targets) {
            complete=complete&&std::none_of(s.scenario.routeDeadEnds.begin(),s.scenario.routeDeadEnds.end(),[&](const auto& x){return x.routeId==id;});
            const auto r=std::find_if(s.scenario.routes.begin(),s.scenario.routes.end(),[&](const auto& x){return x.id==id;});CHECK(r!=s.scenario.routes.end());
            for(const auto& section:table.sections)if(std::find(r->segmentIds.begin(),r->segmentIds.end(),section.id)!=r->segmentIds.end())
                for(const auto& out:m.outLegs)if(section.linkId==out.linkId)exits.insert(out.linkId);
        }
        forcing=forcing||(complete&&exits.size()>1);
    }
    CHECK(forcing); // positive stub demand, complete compiled targets, different Node exits
    test::throws([&]{(void)previewNodeModelDemand(documentJson(d).dump(),d.network.nodes[0].id,{0,120,{}},test::root()/"data");},"NODE_MODEL_PARTIAL_ROUTE");
}
}
TEST(nodeanalysis,na25_study_stub_left){study(DrivingSide::left);}
TEST(nodeanalysis,na25_study_stub_right){study(DrivingSide::right);}
TEST(nodeanalysis,na25_pocket_stub_left){hand(DrivingSide::left);}
TEST(nodeanalysis,na25_pocket_stub_right){hand(DrivingSide::right);}
TEST(nodeanalysis,na25_ambiguous_stub_left){ambiguous(DrivingSide::left);}
TEST(nodeanalysis,na25_ambiguous_stub_right){ambiguous(DrivingSide::right);}
