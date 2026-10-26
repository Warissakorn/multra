// Meso's single-type inputs survive the study-area cut independently of composition classes.
#include "study_area_fixture.hpp"
#include "../src/project/od_demand.hpp"

namespace {
Corridor typedCorridor(bool externalCompositions = false) {
    auto c = mesoCorridor(50);
    auto& d = c.d;
    auto& def = *d.definition;
    // Add a routed input naming one type on the composition demand's chain. A non-unit PCU makes
    // a missing PCU-to-vehicles conversion visible.
    const auto expanded = withOdDemand(d);
    auto route = expanded.routes.back(); route.id = "typed-route";
    def.routes.push_back(route);
    VehicleInput input{"typed-heavy", route.id, "heavy-vehicle", 180, 0, 600};
    def.inputs.push_back(input);
    for (auto& type : def.vehicleTypes) if (type.id == "heavy-vehicle") type.pcu = Pcu{2.5, "Test values"};
    if (externalCompositions) {
        def.externalCompositions = true;
        def.compositions.clear();
        // Unused external compositions refer to other types. The cut must not turn them into an
        // owned catalog and reject this otherwise legal narrow library.
        std::erase_if(def.vehicleTypes, [](const auto& type) { return type.id != "car" && type.id != "heavy-vehicle"; });
        std::erase_if(def.vehicleTypeNames, [](const auto& name) { return name.first != "car" && name.first != "heavy-vehicle"; });
    } else {
        // Both classes have the same suffix, gates and slice windows: only the kind of class can
        // keep these cars separate from the lone heavy type.
        def.compositions.push_back({"heavy-vehicle", {{"car", 1}}, "User composition"});
        VehicleInput cars{"typed-cars", route.id, "car", 300, 0, 600};
        cars.compositionId = "heavy-vehicle";
        def.inputs.push_back(cars);
        d.zones.odMatrices.clear();
    }
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "meso";
    return c;
}
double flowVehicles(const GateFlow& flow) { return flow.vehiclesPerHour * (flow.end - flow.start) / 3600; }
std::map<std::string, double> enteredVehicles(const MesoRun& run) {
    std::map<std::string, double> result;
    for (const auto& watched : run.loading.watched) {
        if (watched.position != 0) continue;
        const auto& cls = run.classes[watched.cls];
        result[cls.id] += watched.entrance.at.back() / cls.pcu;
    }
    return result;
}
}

TEST(studyarea, meso_vehicle_type_cut_preserves_classes) {
    auto c = typedCorridor();
    const auto& d = c.d;
    const auto data = test::root() / "data";
    const auto bytes = documentBytes(d);
    const auto kept = studyAreaCut(d.network, {c.a}).links;
    const auto run = runMeso(d, data, nullptr, nullptr, &kept);
    const auto type = std::find_if(run.classes.begin(), run.classes.end(), [](const auto& cls) { return cls.id == "type:heavy-vehicle"; });
    CHECK(run.classes.size() == 2 && type != run.classes.end() && type->pcu == 2.5); // forcing
    const auto entered = enteredVehicles(run);
    CHECK(entered.at("type:heavy-vehicle") > 29 && entered.at("composition:heavy-vehicle") > 49);
    std::vector<ValidationIssue> advisories;
    const auto flows = mesoGateFlows(d, data, &advisories);
    CHECK(!has(advisories, "MICRO_AREA_CLASS_UNSUPPORTED"));
    double heavy = 0, cars = 0;
    for (const auto& flow : flows) {
        CHECK(flow.start >= 0 && flow.end > flow.start && flow.periodId.empty());
        if (flow.vehicleTypeId == "heavy-vehicle") {
            CHECK(flow.compositionId.empty() && flow.modeId.empty());
            CHECK(flow.from == entryGateName(road(d, c.aw).links[0]));
            heavy += flowVehicles(flow);
        } else {
            CHECK(flow.vehicleTypeId.empty() && flow.compositionId == "heavy-vehicle");
            CHECK(flow.from == entryGateName(road(d, c.aw).links[0]));
            cars += flowVehicles(flow);
        }
        CHECK(flow.to == exitGateName(road(d, c.m3).links[1]));
    }
    test::near(heavy, entered.at("type:heavy-vehicle"), 1e-9);
    test::near(cars, entered.at("composition:heavy-vehicle"), 1e-9);
    const auto copy = studyAreaDocument(d, nullptr, data);
    CHECK(copy.definition && !copy.definition->externalCompositions);
    CHECK(copy.definition->compositions == d.definition->compositions);
    double matrixHeavy = 0, matrixCars = 0;
    for (const auto& matrix : copy.zones.odMatrices) {
        double trips = 0;
        for (const auto& row : matrix.cells) for (const auto vehicles : row) trips += vehicles;
        if (matrix.vehicleTypeId == "heavy-vehicle") {
            CHECK(matrix.compositionId.empty() && matrix.modeId.empty());
            matrixHeavy += trips;
        } else {
            CHECK(matrix.vehicleTypeId.empty() && matrix.compositionId == "heavy-vehicle");
            matrixCars += trips;
        }
    }
    test::near(matrixHeavy, heavy, 1e-9); test::near(matrixCars, cars, 1e-9);
    const auto snapshot = compileDocument(d, data);
    double compiledHeavy = 0, compiledCars = 0;
    for (const auto& input : snapshot.scenario.inputs) {
        const auto vehicles = input.vehiclesPerHour * (input.endTime - input.startTime) / 3600;
        if (input.vehicleTypeId == "heavy-vehicle") compiledHeavy += vehicles;
        else { CHECK(input.vehicleTypeId == "car"); compiledCars += vehicles; }
    }
    test::near(compiledHeavy, heavy, 1e-9); test::near(compiledCars, cars, 1e-9);
    const auto rows = runDiagnostics(d, data);
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& row) { return row.severity == DiagnosticSeverity::runtime; }));
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0);
    CHECK(first == runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42));
    // A fresh side file preserves exactly the same classes, windows and vehicle counts.
    const auto side = std::make_shared<const Json>(mesoSideJson(run, projectSha256(d), kept));
    const StudyAreaSources sources{nullptr, side, {}, inputSha256(side->dump())};
    CHECK(mesoGateFlows(d, data, nullptr, &sources) == flows);
    const auto restored = compileDocument(d, data, nullptr, &sources);
    CHECK(restored.scenario.inputs == snapshot.scenario.inputs);
    CHECK(restored.studyArea && restored.studyArea->sourceSha256 == sources.mesoSha256 && !restored.studyArea->stale);
    CHECK(documentBytes(d) == bytes);
}

TEST(studyarea, meso_vehicle_type_cut_preserves_external_compositions) {
    auto c = typedCorridor(true);
    const auto& d = c.d;
    const auto data = test::root() / "data";
    CHECK(d.definition->externalCompositions && d.definition->compositions.empty()); // forcing
    const auto original = resolveDemandCatalog(*d.definition, data).compositions;
    CHECK(d.definition->vehicleTypes.size() == 2 && !original.empty());
    CHECK(std::any_of(original.begin(), original.end(), [](const auto& comp) {
        return std::any_of(comp.types.begin(), comp.types.end(), [](const auto& share) { return share.vehicleTypeId == "motorcycle"; });
    })); // forcing: an unused external composition names a type absent from the owned library
    const auto bytes = documentBytes(d);
    const auto copy = studyAreaDocument(d, nullptr, data);
    CHECK(copy.definition && copy.definition->externalCompositions && copy.definition->compositions.empty());
    CHECK(resolveDemandCatalog(*copy.definition, data).compositions == original);
    CHECK(std::any_of(copy.zones.odMatrices.begin(), copy.zones.odMatrices.end(), [](const auto& matrix) {
        return matrix.vehicleTypeId == "heavy-vehicle" && matrix.compositionId.empty() && matrix.modeId.empty();
    }));
    InputManifest manifest;
    const auto snapshot = compileDocument(d, data, &manifest);
    const auto recorded = manifest.json();
    bool recordedComposition = false;
    for (const auto& input : recorded["files"]) {
        const auto name = input["logicalPath"].get<std::string>();
        if (!name.starts_with("catalog/compositions/")) continue;
        recordedComposition = true;
        std::ifstream file(data / name.substr(8), std::ios::binary);
        CHECK(file.is_open());
        const std::string bytesRead{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        CHECK(!file.bad() && input["sha256"] == inputSha256(bytesRead) && input["bytes"] == bytesRead.size());
    }
    CHECK(recordedComposition);
    CHECK(std::any_of(snapshot.scenario.inputs.begin(), snapshot.scenario.inputs.end(), [](const auto& input) { return input.vehicleTypeId == "heavy-vehicle"; }));
    CHECK(std::any_of(snapshot.scenario.inputs.begin(), snapshot.scenario.inputs.end(), [](const auto& input) { return input.vehicleTypeId == "car"; }));
    CHECK(documentBytes(d) == bytes && d.definition->externalCompositions && d.definition->compositions.empty());
}

TEST(studyarea, meso_multiple_vehicle_types_share_gates_and_slices) {
    auto c = typedCorridor();
    auto& d = c.d;
    auto car = d.definition->inputs.front();
    CHECK(car.vehicleTypeId == "heavy-vehicle" && car.compositionId.empty());
    car.id = "lone-cars"; car.vehicleTypeId = "car"; car.vehiclesPerHour = 240;
    d.definition->inputs.push_back(car);
    const auto data = test::root() / "data";
    const auto kept = studyAreaCut(d.network, {c.a}).links;
    const auto run = runMeso(d, data, nullptr, nullptr, &kept);
    const auto entered = enteredVehicles(run);
    CHECK(run.classes.size() == 3 && entered.at("type:car") > 39 && entered.at("type:heavy-vehicle") > 29 &&
          entered.at("composition:heavy-vehicle") > 49); // forcing: three classes at the same entry
    const auto copy = studyAreaDocument(d, nullptr, data);
    std::map<std::string, double> matrices;
    for (const auto& matrix : copy.zones.odMatrices) {
        const auto cls = matrix.vehicleTypeId.empty() ? "composition:" + matrix.compositionId : "type:" + matrix.vehicleTypeId;
        for (const auto& row : matrix.cells) for (const auto vehicles : row) matrices[cls] += vehicles;
    }
    CHECK(matrices.size() == 3);
    for (const auto& [cls, vehicles] : entered) test::near(matrices.at(cls), vehicles, 1e-9);
    const auto snapshot = compileDocument(d, data);
    std::map<std::string, double> compiled;
    for (const auto& input : snapshot.scenario.inputs)
        compiled[input.vehicleTypeId] += input.vehiclesPerHour * (input.endTime - input.startTime) / 3600;
    CHECK(compiled.size() == 2);
    test::near(compiled.at("heavy-vehicle"), entered.at("type:heavy-vehicle"), 1e-9);
    test::near(compiled.at("car"), entered.at("type:car") + entered.at("composition:heavy-vehicle"), 1e-9);
}

TEST(studyarea, meso_unknown_class_still_advises) {
    auto c = typedCorridor();
    const auto& d = c.d;
    const auto data = test::root() / "data";
    const auto kept = studyAreaCut(d.network, {c.a}).links;
    const auto run = runMeso(d, data, nullptr, nullptr, &kept);
    auto side = mesoSideJson(run, projectSha256(d), kept);
    std::size_t changed = 0;
    for (auto& cls : side["classes"]) if (cls["id"] == "type:heavy-vehicle") { cls["id"] = "unknown:heavy-vehicle"; ++changed; }
    CHECK(changed == 1); // forcing: only the type class was replaced
    const StudyAreaSources sources{nullptr, std::make_shared<const Json>(side), {}, inputSha256(side.dump())};
    std::vector<ValidationIssue> advisories;
    const auto flows = mesoGateFlows(d, data, &advisories, &sources);
    CHECK(has(advisories, "MICRO_AREA_CLASS_UNSUPPORTED"));
    CHECK(!flows.empty());
    for (const auto& flow : flows) CHECK(flow.vehicleTypeId.empty() && flow.compositionId == "heavy-vehicle");
}

TEST(studyarea, meso_unknown_vehicle_type_refuses_run) {
    const auto c = typedCorridor();
    const auto& d = c.d;
    const auto data = test::root() / "data";
    const auto kept = studyAreaCut(d.network, {c.a}).links;
    const auto run = runMeso(d, data, nullptr, nullptr, &kept);
    auto side = mesoSideJson(run, projectSha256(d), kept);
    std::size_t changed = 0;
    for (auto& cls : side["classes"]) if (cls["id"] == "type:heavy-vehicle") { cls["id"] = "type:unknown-id"; ++changed; }
    CHECK(changed == 1);
    const StudyAreaSources sources{nullptr, std::make_shared<const Json>(side), {}, inputSha256(side.dump())};
    const auto flows = mesoGateFlows(d, data, nullptr, &sources);
    CHECK(std::any_of(flows.begin(), flows.end(), [](const auto& flow) { return flow.vehicleTypeId == "unknown-id" && flow.vehiclesPerHour > 0; }));
    // A recognized type: class is not an unknown-prefix advisory: its actual type must exist.
    const auto issues = refusal([&] { compileDocument(d, data, nullptr, &sources); });
    CHECK(has(issues, "UNKNOWN_VEHICLE_TYPE"));
    const auto rows = runDiagnostics(d, data, &sources);
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& row) {
        return row.code == "UNKNOWN_VEHICLE_TYPE" && row.severity == DiagnosticSeverity::runtime;
    }));
}

TEST(studyarea, vehicle_type_cut_fields_remain_runtime_only) {
    auto c = demandCorridor(20);
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    const auto ends = studyAreaEnds(d);
    CHECK(!ends.origins.empty() && !ends.destinations.empty());
    d.zones.studyArea.gateFlows = {{"p", ends.origins.front(), ends.destinations.front(), "car-only", "", 120}};
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 55 && documentJson(parseDocument(saved)) == saved);
    CHECK(saved["odMatrices"].size() == 1 && saved["studyArea"]["gateFlows"].size() == 1);
    for (const bool replaceComposition : {false, true}) {
        auto matrix = saved;
        matrix["odMatrices"][0]["vehicleTypeId"] = "heavy-vehicle";
        if (replaceComposition) matrix["odMatrices"][0].erase("compositionId");
        CHECK(matrix["odMatrices"][0]["vehicleTypeId"] == "heavy-vehicle" &&
              matrix["odMatrices"][0].contains("compositionId") != replaceComposition);
        test::throws([&] { parseDocument(matrix); }, replaceComposition ? "OD_TYPES_AMBIGUOUS" : "EDIT_ZONE_INVALID");
        auto gate = saved;
        gate["studyArea"]["gateFlows"][0]["vehicleTypeId"] = "heavy-vehicle";
        if (replaceComposition) gate["studyArea"]["gateFlows"][0].erase("compositionId");
        CHECK(gate["studyArea"]["gateFlows"][0]["vehicleTypeId"] == "heavy-vehicle" &&
              gate["studyArea"]["gateFlows"][0].contains("compositionId") != replaceComposition);
        test::throws([&] { parseDocument(gate); }, "EDIT_STUDY_AREA_INVALID");
    }
}
