#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/run.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include <algorithm>
#include <limits>
using namespace multra;
namespace {
Json ownedFile() {
    ProjectDocument d;const auto link=addLink(d,{{0,0},{400,0}},2,3.5);
    const auto route=putRoute(d,{"route",{link}});changeRunSettings(d,120,.1);
    const auto catalog=resolveCatalogs(AuthoringDefinition{},test::root()/"data");
    d.definition->vehicleTypes=catalog.vehicleTypes;d.definition->behaviours=catalog.behaviours;
    for(auto& b:d.definition->behaviours)b.amberDeceleration.reset(); // M4.2 (D147): an owned amberDeceleration is schema 28 (amber tests); this fixture exercises the schemas before it.
    d.definition->externalVehicleTypes=false;d.definition->externalBehaviours=false;
    VehicleInput input{"in",route,"",600,0,120};input.compositionId="mix";putInput(d,input);
    auto j=documentJson(d);
    j["definition"]["compositions"]=Json::array({{{"id","mix"},{"name","Project mix"},
        {"types",Json::array({{{"vehicleTypeId","car"},{"share",3}},{{"vehicleTypeId","heavy-vehicle"},{"share",1}}})}}});
    j["definition"]["vehicleTypes"][0]["name"]="Project vehicle";
    j["schemaVersion"]=18;
    return j;
}
}
TEST(demand, owned_compositions_and_names_round_trip_without_external_catalogs) {
    const auto file=ownedFile();const auto d=parseDocument(file);const auto saved=documentJson(d);
    CHECK(saved["definition"].contains("compositions"));
    CHECK(saved["definition"]["compositions"]==file["definition"]["compositions"]);
    CHECK(saved["definition"]["vehicleTypes"][0]["name"]=="Project vehicle");
    const auto s=compileDocument(d,test::root()/"no-such-demand-directory").scenario;
    double volume=0;for(const auto& i:s.inputs)volume+=i.vehiclesPerHour;
    test::near(volume,600);CHECK(s.inputs.size()==4);
}
TEST(demand, owned_catalogs_reject_duplicate_ids_and_missing_composition_references) {
    auto file=ownedFile();auto& c=file["definition"]["compositions"];c.push_back(c[0]);
    CHECK(c.size()==2 && c[0]["id"]==c[1]["id"]);
    test::throws([&]{parseDocument(file);},"DUPLICATE_ID");
    file=ownedFile();file["definition"]["compositions"]=Json::array();
    CHECK(file["definition"]["inputs"][0]["compositionId"]=="mix");
    test::throws([&]{parseDocument(file);},"UNKNOWN_COMPOSITION");
}
TEST(demand, every_owned_composition_is_validated_even_when_unused) {
    auto file=ownedFile();file["definition"]["inputs"]=Json::array();
    auto& entry=file["definition"]["compositions"][0]["types"][0];entry["share"]=-1;
    CHECK(entry["share"]==-1);test::throws([&]{parseDocument(file);},"INVALID_SHARE");
    file=ownedFile();file["definition"]["compositions"][0]["types"][1]["vehicleTypeId"]="missing";
    CHECK(file["definition"]["compositions"][0]["types"][1]["vehicleTypeId"]=="missing");
    test::throws([&]{parseDocument(file);},"UNKNOWN_VEHICLE_TYPE");
}

TEST(demand, catalog_capture_undo_redo_and_failed_reference_edit_are_atomic) {
    auto file=ownedFile();auto d=parseDocument(file);History history;history.reset(d);
    const auto before=documentJson(history.document());
    auto catalog=resolveDemandCatalog(d.definition.value(),test::root()/"missing-data");
    CHECK(catalog.compositions.size()==1);
    catalog.compositions.clear();CHECK(catalog.compositions.empty());
    test::throws([&]{history.execute("catalog",[&](auto& target){putDemandCatalog(target,catalog);});},"UNKNOWN_COMPOSITION");
    CHECK(documentJson(history.document())==before);
    catalog=resolveDemandCatalog(d.definition.value(),test::root()/"missing-data");
    catalog.compositions.front().types[0].share=1;
    CHECK(history.execute("catalog",[&](auto& target){putDemandCatalog(target,catalog);}));
    const auto after=documentJson(history.document());CHECK(after!=before);
    history.undo();CHECK(documentJson(history.document())==before);
    history.redo();CHECK(documentJson(history.document())==after);
}
TEST(demand, catalog_rejects_duplicate_members_nonfinite_totals_and_unknown_behaviour) {
    auto file=ownedFile();auto& members=file["definition"]["compositions"][0]["types"];
    members[1]["vehicleTypeId"]=members[0]["vehicleTypeId"];
    CHECK(members[0]["vehicleTypeId"]==members[1]["vehicleTypeId"]);
    test::throws([&]{parseDocument(file);},"DUPLICATE_ID");
    auto d=parseDocument(ownedFile());
    d.definition->compositions.front().types[0].share=std::numeric_limits<double>::infinity();
    CHECK(!std::isfinite(d.definition->compositions.front().types[0].share));
    test::throws([&]{validateDocument(d);},"INVALID_SHARE");
    file=ownedFile();file["definition"]["vehicleTypes"][0]["behaviourId"]="missing";
    CHECK(file["definition"]["vehicleTypes"][0]["behaviourId"]=="missing");
    test::throws([&]{parseDocument(file);},"UNKNOWN_BEHAVIOUR");
}
TEST(demand, catalog_names_are_metadata_and_old_external_ownership_stays_absent) {
    auto d=parseDocument(ownedFile());const auto baseline=compileDocument(d,test::root()/"missing-data").scenario;
    d.definition->compositions.front().name="Renamed composition";d.definition->vehicleTypeNames["car"]="Renamed car";
    CHECK(static_cast<const ScenarioDefinition&>(compileDocument(d,test::root()/"missing-data").scenario)==
        static_cast<const ScenarioDefinition&>(baseline));
    auto old=ownedFile();old["schemaVersion"]=17;old["definition"].erase("compositions");
    old["definition"]["inputs"][0]["compositionId"]="urban-mixed";
    const auto legacy=parseDocument(old);CHECK(legacy.definition->externalCompositions);
    CHECK(!documentJson(legacy)["definition"].contains("compositions"));
    const auto resolved=compileDocument(legacy,test::root()/"data").scenario;
    double volume=0;for(const auto& input:resolved.inputs)volume+=input.vehiclesPerHour;test::near(volume,600);
}
TEST(demand, diverse_catalog_compiles_all_types_and_owned_copy_is_portable) {
    const auto data=test::root()/"data";
    const auto catalog=resolveDemandCatalog(AuthoringDefinition{},data);
    for(const auto* id:{"motorcycle","pickup","van","bus","rigid-truck"}) {
        const auto type=std::find_if(catalog.vehicleTypes.begin(),catalog.vehicleTypes.end(),[&](const auto& t){return t.id==id;});
        CHECK(type!=catalog.vehicleTypes.end());CHECK(type->axles.has_value());
        test::near(type->axles->wheelbase+type->axles->frontOverhang+type->axles->rearOverhang,type->length);
        CHECK(!catalog.vehicleTypeNames.at(id).empty());
    }
    ProjectDocument d;const auto link=addLink(d,{{0,0},{1000,0}},2,3.5);
    const auto route=putRoute(d,{"route",{link}});changeRunSettings(d,120,.1);
    VehicleInput input{"in",route,"",3600,0,120};input.compositionId="urban-diverse";putInput(d,input);
    const auto external=compileDocument(d,data).scenario;
    CHECK(validateScenario(external).empty());
    const std::map<std::string,double> expected{{"car",.45},{"motorcycle",.20},{"pickup",.15},{"van",.08},
        {"bus",.04},{"rigid-truck",.05},{"heavy-vehicle",.03}};
    std::map<std::string,double> volumes;
    for(const auto& compiled:external.inputs)volumes[compiled.vehicleTypeId]+=compiled.vehiclesPerHour;
    CHECK(volumes.size()==expected.size());
    for(const auto& [id,share]:expected)test::near(volumes.at(id),3600*share);
    putDemandCatalog(d,catalog);
    const auto reopened=parseDocument(documentJson(d));
    const auto portable=compileDocument(reopened,test::root()/"missing-catalogs").scenario;
    CHECK(portable.vehicleTypes==external.vehicleTypes);CHECK(portable.behaviours==external.behaviours);
    CHECK(portable.inputs==external.inputs);CHECK(portable.routes==external.routes);
    CHECK(portable.segments.size()==external.segments.size());
    for(std::size_t i=0;i<external.segments.size();++i) {
        const auto& saved=portable.segments[i];const auto& original=external.segments[i];
        CHECK(saved.id==original.id);test::near(saved.length,original.length);
        CHECK(saved.next==original.next);test::near(saved.curvature,original.curvature);
    }
    CHECK(validateScenario(portable).empty());
    CHECK(reopened.definition->vehicleTypeNames==catalog.vehicleTypeNames);
}
TEST(demand, expanded_catalog_keeps_legacy_mix_and_vehicle_parameters) {
    const auto data=test::root()/"data";
    const auto catalog=resolveDemandCatalog(AuthoringDefinition{},data);
    const auto mix=std::find_if(catalog.compositions.begin(),catalog.compositions.end(),[](const auto& c){return c.id=="urban-mixed";});
    CHECK(mix!=catalog.compositions.end());
    CHECK(mix->types==std::vector<CompositionShare>({{"car",.95},{"heavy-vehicle",.05}}));
    const auto type=[&](const char* id)->const VehicleType& {
        const auto found=std::find_if(catalog.vehicleTypes.begin(),catalog.vehicleTypes.end(),[&](const auto& t){return t.id==id;});
        CHECK(found!=catalog.vehicleTypes.end());return *found;
    };
    test::near(type("car").length,4.5);test::near(type("car").width,1.8);
    test::near(type("heavy-vehicle").length,12);test::near(type("heavy-vehicle").width,2.5);
    CHECK(type("motorcycle").width<type("car").width);
    CHECK(type("bus").length>type("van").length);
}
TEST(demand, preferred_type_is_car_before_the_first_by_id) {
    const auto catalog=resolveDemandCatalog(AuthoringDefinition{},test::root()/"data");
    CHECK(!catalog.vehicleTypes.empty() && catalog.vehicleTypes.front().id=="bus"); // forcing: bus sorts before car
    const auto* preferred=preferredVehicleType(catalog.vehicleTypes);
    CHECK(preferred && preferred->id=="car");
    auto noCar=catalog.vehicleTypes;
    std::erase_if(noCar,[](const auto& t){return t.id=="car";});
    CHECK(preferredVehicleType(noCar)==&noCar.front());
    CHECK(preferredVehicleType({})==nullptr);
}
