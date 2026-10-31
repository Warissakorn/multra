#include "node_model_demand_fixture.hpp"
#include "../src/project/node_model_demand.hpp"
#include "../src/commands/zone_commands.hpp"
using namespace multra;
namespace {
ProjectDocument od(DrivingSide side) {
    auto d=nodeModelTest::fixture(side);auto& def=*d.definition;def.inputs.clear();def.routes.clear();def.clockStart=25200;
    const auto m=nodeMembership(d.network,d.network.nodes[0]);const auto& t=m.turns[0];
    putZone(d,{"origin","Origin",{{-500,-500},{-490,-500},{-490,-490}}, {}});
    putZone(d,{"destination","Destination",{{500,500},{510,500},{510,510}}, {}});
    putZoneConnector(d,{"origin-link","origin",m.inLegs[t.in].linkId,ZoneDirection::origin,1});
    putZoneConnector(d,{"destination-link","destination",m.outLegs[t.out].linkId,ZoneDirection::destination,1});
    putStudyPeriod(d,{"am","AM",25200,25320});
    OdMatrix matrix{"matrix",0,0,"urban-mixed",{"origin","destination"},{{0,100},{0,0}}};matrix.periodId="am";
    putOdMatrix(d,matrix);return d;
}
void composedOd(DrivingSide side) {
    const auto d=od(side);const auto compiled=compileDocument(d,test::root()/"data");
    CHECK(!compiled.scenario.inputs.empty()&&d.zones.odMatrices[0].periodId=="am"&&d.definition->clockStart==25200);
    double expected=0,heavy=0;
    for(const auto& i:compiled.scenario.inputs){CHECK(i.startTime>=0&&i.endTime<=120);expected+=i.vehiclesPerHour;if(i.vehicleTypeId=="heavy-vehicle")heavy+=i.vehiclesPerHour;}
    test::near(expected,3000,1e-10);test::near(heavy,150,1e-10); // forcing: 100 trips/120 s, urban-mixed = 5 % HV
    const auto p=previewNodeModelDemand(documentJson(d).dump(),d.network.nodes[0].id,{30,90,{"heavy-vehicle"}},test::root()/"data");
    CHECK(p.turns.size()==12&&p.turns[0].heavyVehiclePercent);
    test::near(p.turns[0].vehiclesPerHour,3000,1e-9);test::near(*p.turns[0].heavyVehiclePercent,5,1e-12);
    double total=0;for(const auto& t:p.turns)total+=t.vehiclesPerHour;test::near(total,3000,1e-9);
}
ProjectDocument decision(DrivingSide side,bool positioned) {
    auto d=nodeModelTest::fixture(side);const auto m=nodeMembership(d.network,d.network.nodes[0]);
    CHECK(m.turns[0].in==m.turns[1].in);auto& def=*d.definition;
    std::vector<std::string> second;for(const auto& span:m.turns[1].paths[0])second.push_back(span.objectId);
    def.routes.push_back({"other-turn",second});def.inputs.clear();
    RoutingDecision split;split.id="split";split.routes={{"counted-turn",1,{}, {1,3}},{"other-turn",1,{}, {3,1}}};split.intervals={{0,60},{60,120}};
    if(positioned){split.linkId=m.inLegs[m.turns[0].in].linkId;split.position=10;}
    def.routingDecisions.push_back(split);VehicleInput input{"source",{},"car",1200,0,120};
    if(positioned)input.linkId=split.linkId;else input.routingDecisionId="split";
    def.inputs.push_back(input);return d;
}
void entryShares(DrivingSide side) {
    const auto d=decision(side,false);const auto compiled=compileDocument(d,test::root()/"data");
    CHECK(compiled.scenario.routeDecisions.empty()&&compiled.scenario.inputs.size()>2);
    const auto p=previewNodeModelDemand(documentJson(d).dump(),d.network.nodes[0].id,{0,60,{}},test::root()/"data");
    CHECK(p.turns.size()==12);test::near(p.turns[0].vehiclesPerHour,300,1e-10);test::near(p.turns[1].vehiclesPerHour,900,1e-10);
    const auto later=previewNodeModelDemand(documentJson(d).dump(),d.network.nodes[0].id,{60,120,{}},test::root()/"data");
    CHECK(later.turns.size()==12);test::near(later.turns[0].vehiclesPerHour,900,1e-10);test::near(later.turns[1].vehiclesPerHour,300,1e-10);
}
void refusal(const ProjectDocument& d,const NodeModelDemandRequest& request,const char* code,const std::string& path) {
    const auto bytes=documentJson(d).dump();bool threw=false;
    try{(void)previewNodeModelDemand(bytes,d.network.nodes[0].id,request,test::root()/"data");}
    catch(const ValidationError& e){threw=true;CHECK(junctionTest::issue(e.issues,code,path));}
    CHECK(threw&&documentJson(d).dump()==bytes);
}
void guards(DrivingSide side) {
    const auto d=nodeModelTest::fixture(side);const auto compiled=compileDocument(d,test::root()/"data");CHECK(!compiled.scenario.inputs.empty());
    for(const auto& request:{NodeModelDemandRequest{-1,60,{}},{30,30,{}},{60,30,{}},{0,121,{}}}) {
        CHECK(request.start<0||request.end<=request.start||request.end>120);refusal(d,request,"NODE_MODEL_WINDOW","modelSource");
    }
    CHECK(std::none_of(compiled.scenario.vehicleTypes.begin(),compiled.scenario.vehicleTypes.end(),[](const auto& t){return t.id=="not-a-type";}));
    refusal(d,{0,60,{"not-a-type"}},"NODE_MODEL_HV_TYPE","heavyVehicleTypes");
    refusal(d,{0,60,{"car","car"}},"NODE_MODEL_HV_TYPE","heavyVehicleTypes");
    const auto dynamic=decision(side,true);const auto routed=compileDocument(dynamic,test::root()/"data");
    CHECK(!routed.scenario.routeDecisions.empty()&&dynamic.definition->routingDecisions[0].position==10);
    refusal(dynamic,{0,60,{}},"NODE_MODEL_RUNTIME_DEMAND","split");
    auto partial=d;partial.definition->routes[0].segmentIds.resize(1);
    const auto shortRun=compileDocument(partial,test::root()/"data");CHECK(std::all_of(shortRun.scenario.routes.begin(),shortRun.scenario.routes.end(),[](const auto& r){return r.segmentIds.size()==1;}));
    refusal(partial,{0,60,{}},"NODE_MODEL_PARTIAL_ROUTE","inputs[");
    auto parking=od(side);
    putZoneConnector(parking,{"lot-gate","origin",parking.zones.zoneConnectors[1].linkId,ZoneDirection::destination,1});
    putParkingLot(parking,{"lot","origin",1,1,"car-only",0,{{1,1}}});
    const auto lot=compileDocument(parking,test::root()/"data");CHECK(lot.scenario.parkingLots.size()==1&&lot.scenario.parkingLots[0].initialOccupancy==1&&!lot.scenario.parkingLots[0].exits.empty());
    refusal(parking,{0,60,{}},"NODE_MODEL_RUNTIME_DEMAND","lot");
    // A provably empty lot with no gate demand does not make unrelated, determined demand uncertain.
    parking.zones.parkingLots[0].initialOccupancy=0;parking.zones.parkingLots[0].initialCompositionId.clear();
    auto& ordinary=*parking.definition;ordinary.routes=d.definition->routes;ordinary.inputs=d.definition->inputs;
    parking.zones.zoneConnectors.back().linkId=nodeMembership(parking.network,parking.network.nodes[0]).outLegs.back().linkId;
    CHECK(parking.zones.zoneConnectors.back().linkId!=parking.zones.zoneConnectors[1].linkId);
    const auto emptyLot=compileDocument(parking,test::root()/"data");
    CHECK(emptyLot.scenario.parkingLots[0].initialOccupancy==0);
    CHECK(std::none_of(emptyLot.scenario.inputs.begin(),emptyLot.scenario.inputs.end(),[&](const auto& i){return i.vehiclesPerHour>0&&std::find(emptyLot.scenario.parkingLots[0].gateRouteIds.begin(),emptyLot.scenario.parkingLots[0].gateRouteIds.end(),i.routeId)!=emptyLot.scenario.parkingLots[0].gateRouteIds.end();}));
    const auto known=previewNodeModelDemand(documentJson(parking).dump(),parking.network.nodes[0].id,{30,90,{"rigid-truck"}},test::root()/"data");
    CHECK(known.turns.size()==12);test::near(known.turns[0].vehiclesPerHour,1800,1e-9);
}
}
TEST(nodeanalysis,na24_composed_od_left){composedOd(DrivingSide::left);}
TEST(nodeanalysis,na24_composed_od_right){composedOd(DrivingSide::right);}
TEST(nodeanalysis,na24_entry_shares_left){entryShares(DrivingSide::left);}
TEST(nodeanalysis,na24_entry_shares_right){entryShares(DrivingSide::right);}
TEST(nodeanalysis,na25_model_guards_left){guards(DrivingSide::left);}
TEST(nodeanalysis,na25_model_guards_right){guards(DrivingSide::right);}
