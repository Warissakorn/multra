#include "node_model_demand_fixture.hpp"
#include "../src/project/node_model_demand.hpp"
#include "../src/project/node_analysis_report.hpp"
#include "../src/commands/node_analysis_commands.hpp"
#include <numeric>
using namespace multra;
namespace {
using nodeModelTest::fixture;

void counts(DrivingSide side) {
    const auto d=fixture(side);const auto m=nodeMembership(d.network,d.network.nodes[0]);
    const auto compiled=compileDocument(d,test::root()/"data");double expected=0;
    for(const auto& i:compiled.scenario.inputs) {
        const double seconds=std::max(0.,std::min(90.,i.endTime)-std::max(30.,i.startTime));
        expected+=i.vehiclesPerHour*seconds/3600;
    }
    test::near(expected,30,1e-12);CHECK(compiled.scenario.inputs.size()>2&&!compiled.scenario.routeDeadEnds.empty()); // force interval/lane/stub expansion first
    const auto bytes=" \r\n"+documentJson(d).dump()+"\r\n";
    const auto p=previewNodeModelDemand(bytes,d.network.nodes[0].id,{30,90,{"rigid-truck"}},test::root()/"data");
    CHECK(p.turns.size()==12);
    test::near(p.turns[0].vehiclesPerHour,1800,1e-10);CHECK(p.turns[0].heavyVehiclePercent);test::near(*p.turns[0].heavyVehiclePercent,100./3,1e-10);
    CHECK(p.turns.back().vehiclesPerHour==0&&p.turns.back().heavyVehiclePercent==7);
    double total=0;for(const auto& t:p.turns)total+=t.vehiclesPerHour;test::near(total,1800,1e-10);
    CHECK(p.revision==d.revision&&p.inputManifest["files"].size()>1);
    bool found=false;for(const auto& f:p.inputManifest["files"])if(f["logicalPath"]=="project"){found=true;CHECK(f["sha256"]==inputSha256(bytes));}
    CHECK(found&&documentJson(d).dump()+"\n"!=bytes);
    const auto empty=previewNodeModelDemand(bytes,d.network.nodes[0].id,{30,90,{}},test::root()/"data");
    CHECK(empty.turns[0].heavyVehiclePercent==0); // explicitly selecting none means 0, not HCM's default
}
}
TEST(nodeanalysis,na24_model_demand_left){counts(DrivingSide::left);}
TEST(nodeanalysis,na24_model_demand_right){counts(DrivingSide::right);}
