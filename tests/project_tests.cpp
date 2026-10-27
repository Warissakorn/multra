#include "test.hpp"
#include "../src/project/document.hpp"
#include <fstream>
#include <tuple>

using namespace multra;
TEST(project, strict_seed_parser) {
    CHECK(parseSeed("0") == 0); CHECK(parseSeed("42") == 42); CHECK(parseSeed("4294967295") == 4294967295U);
    for (const auto* invalid : {"", "-1", "+1", "2.5", "NaN", "Infinity", "4294967296", "1x", " 1"})
        test::throws([&] { parseSeed(invalid); });
}
TEST(project, strict_json_shapes) {
    std::ifstream file(test::root() / "data/scenarios/crossing.json");
    const auto source = Json::parse(file);
    auto n = source.at("network"); n["links"] = Json::object();
    test::throws([&] { parseNetwork(n,0); }, "Expected array");
    n = source.at("network"); n["links"][0]["lanes"][0]["width"] = true;
    test::throws([&] { parseNetwork(n,0); }, "Expected number");
    n = source.at("network"); n["drivingSide"] = "unknown";
    test::throws([&] { parseNetwork(n,0); }, "INVALID_DRIVING_SIDE");
    auto d = source.at("definition"); d["signalPrograms"][0]["phases"][0]["color"] = "blue";
    test::throws([&] { parseDefinition(d); }, "INVALID_SIGNAL_COLOR");
    d = source.at("definition"); d.erase("timeStep");
    test::throws([&] { parseDefinition(d); });
}
TEST(project, m1_26_1_lane_shares_round_trip_and_stay_out_of_an_unedited_file) {
    AuthoringDefinition d;
    d.duration = 60; d.timeStep = 0.1;
    d.routes.push_back({"r", {"a"}});
    // Unedited: no laneShares at all. It must not appear in the saved file, so an older reader
    // (and a byte-for-byte diff against a schema-8 save) sees exactly what it always did.
    d.inputs.push_back({"i1", "r", "car", 900, 0, 60});
    auto j = definitionJson(d);
    CHECK(!j["inputs"][0].contains("laneShares"));
    const auto reparsed = parseAuthoringDefinition(j);
    CHECK(reparsed.inputs[0].laneShares.empty());
    // Edited: the weights round-trip exactly.
    d.inputs[0].laneShares = {1, 2, 3};
    j = definitionJson(d);
    CHECK(j["inputs"][0]["laneShares"] == Json({1, 2, 3}));
    const auto withShares = parseAuthoringDefinition(j);
    CHECK(withShares.inputs[0].laneShares == std::vector<double>{1, 2, 3});
}
TEST(project, missing_file) {
    test::throws([&] { loadScenario(test::root() / "missing.json", test::root() / "data"); }, "Cannot read JSON");
}
TEST(project, catalog_and_translation_keys) {
    const auto s = loadScenario(test::root()/"data/scenarios/crossing.json",test::root()/"data").scenario;
    // Installed types are sorted by ID; adding a catalog entry must not require engine code.
    std::vector<std::string> ids;for(const auto& t:s.vehicleTypes)ids.push_back(t.id);
    CHECK(ids == std::vector<std::string>({"bus","car","heavy-vehicle","motorcycle","pickup","rigid-truck","van"}));
    CHECK(s.behaviours.size() == 1);
    std::ifstream english(test::root() / "data/locales/en.json"), thai(test::root() / "data/locales/th.json");
    CHECK(english.good()); CHECK(thai.good());
    const auto en = Json::parse(english), th = Json::parse(thai);
    CHECK(en.size() == th.size());
    for (const auto& [key, value] : en.items()) {
        CHECK(value.is_string()); CHECK(!value.get<std::string>().empty());
        CHECK(th.at(key).is_string()); CHECK(!th.at(key).get<std::string>().empty());
    }
}
// The reported failure: an editor project opened in the simulation window used to surface
// "[json.exception.type_error.304] cannot use at() with null" because a drawn network is
// legitimately saved with "definition": null. It must name the file kind instead.
TEST(project, file_kind_is_named_not_thrown_as_a_parser_error) {
    const auto directory = std::filesystem::temp_directory_path() / "multra-project-tests";
    std::filesystem::create_directories(directory);
    const auto data = test::root() / "data";
    const auto write = [&](const std::string& name, const Json& value) {
        const auto path = directory / name;
        std::ofstream(path) << value.dump(2);
        return path;
    };
    ProjectDocument drawn;
    drawn.network = test::demo().network;
    const auto project = documentJson(drawn);
    CHECK(project.at("definition").is_null()); // the shape that produced the report
    for (const auto& [name, value, code] : std::vector<std::tuple<std::string, Json, std::string>>{
            {"network.traffic.json", project, "SCENARIO_IS_PROJECT"},
            {"no-definition.json", Json{{"network", project.at("network")}}, "SCENARIO_NO_DEFINITION"},
            {"null-network.json", Json{{"network", nullptr}, {"definition", Json::object()}}, "SCENARIO_NO_NETWORK"},
            {"array.json", Json::array(), "SCENARIO_NOT_JSON_OBJECT"}}) {
        const auto path = write(name, value);
        try {
            loadScenario(path, data);
            throw std::runtime_error("Expected a load failure for " + name);
        } catch (const ScenarioLoadError& error) {
            CHECK(error.code == code);
            CHECK(std::string(error.what()).find("json.exception") == std::string::npos);
        }
    }
    std::filesystem::remove_all(directory);
}
TEST(project, editor_rejects_null_sections_with_named_codes) {
    auto project = documentJson(ProjectDocument{});
    CHECK(parseDocument(project).network.links.empty()); // the round trip still works
    auto broken = project; broken["background"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_BACKGROUND_INVALID");
    broken = project; broken["background"]["metresPerPixel"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_BACKGROUND_INVALID");
    broken = project; broken["network"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_NO_NETWORK");
    broken = project; broken["nextId"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_ID_LIMIT");
    broken = project; broken["format"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_VERSION");
    test::throws([&] { parseDocument(Json(nullptr)); }, "EDIT_VERSION");
}
// No load path may surface an nlohmann exception: a null anywhere in a scenario must produce
// a sentence naming the field, not "cannot use at() with null" / "type must be ... but is null".
TEST(project, no_null_field_leaks_a_parser_exception) {
    std::ifstream file(test::root() / "data/scenarios/crossing.json");
    const auto source = Json::parse(file);
    const auto nulled = [&](const std::function<void(Json&)>& breakIt) {
        auto copy = source; breakIt(copy);
        try { parseNetwork(copy.at("network"),0); parseDefinition(copy.at("definition")); }
        catch (const std::exception& error) {
            CHECK(std::string(error.what()).find("json.exception") == std::string::npos);
            return;
        }
        throw std::runtime_error("Expected a rejection");
    };
    nulled([](Json& j) { j["network"]["links"][0]["id"] = nullptr; });
    nulled([](Json& j) { j["network"]["links"][0]["lanes"][0]["width"] = nullptr; });
    nulled([](Json& j) { j["network"]["connectors"][0]["from"] = nullptr; });
    nulled([](Json& j) { j["network"]["signalHeads"][0]["lane"]["laneId"] = nullptr; });
    nulled([](Json& j) { j["definition"]["routes"][0]["segmentIds"][0] = nullptr; });
    nulled([](Json& j) { j["definition"]["signalPrograms"][0]["phases"] = nullptr; });
    nulled([](Json& j) { j["definition"]["inputs"][0]["vehiclesPerHour"] = nullptr; });
}
// Root-level metadata is where classification itself reads, and it was the one place a
// parser exception still escaped: value("format", std::string{}) throws when the key is
// present but not a string. Drive these through loadScenario, not the section parsers.
TEST(project, file_kind_classification_survives_broken_metadata) {
    const auto directory = std::filesystem::temp_directory_path() / "multra-metadata-tests";
    std::filesystem::create_directories(directory);
    std::ifstream source(test::root() / "data/scenarios/crossing.json");
    const auto scenario = Json::parse(source);
    for (const auto& metadata : {Json(nullptr), Json(42), Json(Json::array()), Json("TrafficSim"), Json("other")})
        for (const auto* key : {"format", "schemaVersion"}) {
            auto broken = scenario; broken.erase("definition"); broken[key] = metadata;
            const auto path = directory / "broken.json";
            std::ofstream(path) << broken.dump(2);
            try {
                loadScenario(path, test::root() / "data");
                throw std::runtime_error("Expected a load failure");
            } catch (const ScenarioLoadError& error) {
                CHECK(std::string(error.what()).find("json.exception") == std::string::npos);
                CHECK(error.code == "SCENARIO_IS_PROJECT" || error.code == "SCENARIO_NO_DEFINITION");
            }
        }
    std::filesystem::remove_all(directory);
}

TEST(project, allocated_ids_skip_every_existing_namespace) {
    // Each fixture forces the candidate into a different id-owning collection, including the
    // allocation-only zone/mode namespaces. Check the collision before checking the skip.
    const auto append = [](auto& objects) {
        objects.emplace_back(); objects.back().id = "taken-1";
        CHECK(!objects.empty() && objects.back().id == "taken-1");
    };
    const std::vector<std::function<void(ProjectDocument&)>> fixtures{
        [](auto& d) { d.network.id = "taken-1"; CHECK(d.network.id == "taken-1"); },
        [&](auto& d) { append(d.macroRuns); },
        [&](auto& d) { append(d.network.links); },
        [&](auto& d) { d.network.links.emplace_back(); append(d.network.links.back().lanes); },
        [&](auto& d) { append(d.network.connectors); d.network.connectors.back().fromLaneCount = 1; d.network.connectors.back().toLaneCount = 1; },
        [&](auto& d) { append(d.network.signalHeads); },
        [&](auto& d) { append(d.network.rightOfWay.waitingLines); },
        [&](auto& d) { append(d.network.rightOfWay.conflictAreas); },
        [&](auto& d) { append(d.network.rightOfWay.priorityRules); },
        [&](auto& d) { append(d.network.rightOfWay.stopControls); },
        [&](auto& d) { append(d.network.queueCounters); },
        [&](auto& d) { append(d.network.travelTimeSections); },
        [&](auto& d) { append(d.network.nodes); },
        [&](auto& d) { append(d.network.roads); },
        [&](auto& d) { append(d.definition->routes); },
        [&](auto& d) { append(d.definition->inputs); },
        [&](auto& d) { append(d.definition->signalPrograms); },
        [&](auto& d) { append(d.definition->signalControllers); },
        [&](auto& d) { append(d.definition->routingDecisions); },
        [&](auto& d) { append(d.zones.zones); },
        [&](auto& d) { append(d.zones.zoneConnectors); },
        [&](auto& d) { append(d.zones.odMatrices); },
        [&](auto& d) { append(d.zones.parkingLots); },
        [&](auto& d) { append(d.zones.studyPeriods); },
        [&](auto& d) { append(d.definition->modes); }
    };
    for (const auto& fixture : fixtures) {
        ProjectDocument d; d.definition = AuthoringDefinition{};
        fixture(d);
        CHECK(d.nextId == 1);
        CHECK(allocateId(d, "taken") == "taken-2");
        CHECK(d.nextId == 3);
    }
}
TEST(project, allocation_preserves_connector_path_ids_and_enumeration_scope) {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    d.network.connectors.push_back({});
    auto& c = d.network.connectors.back(); c.id = "range"; c.fromLaneCount = 3; c.toLaneCount = 1;
    d.macroRuns.emplace_back(); d.macroRuns.back().id = "macro";
    d.zones.zones.emplace_back(); d.zones.zones.back().id = "zone";
    d.definition->modes.emplace_back(); d.definition->modes.back().id = "mode";
    const std::set<std::string> expected{"network", "range", "range/lane-2", "range/lane-3", "macro"};
    CHECK(documentIds(d) == expected);
    auto withoutMacro = expected; withoutMacro.erase("macro");
    CHECK(documentIds(d, false) == withoutMacro);
    d.nextId = 2;
    CHECK(allocateId(d, "range/lane") == "range/lane-4");
    CHECK(d.nextId == 5);
    CHECK(allocateId(d, "ถนน-ซอย") == "ถนน-ซอย-5");
    CHECK(d.nextId == 6);
}
TEST(project, allocation_retains_exhaustion_and_collision_counter_semantics) {
    const auto limit = std::numeric_limits<std::uint64_t>::max();
    ProjectDocument d;
    for (const auto next : {limit - 1, limit}) {
        d.nextId = next;
        test::throws([&] { allocateId(d, "id"); }, "EDIT_ID_LIMIT");
        CHECK(d.nextId == next);
    }
    d.nextId = limit - 2; d.network.id = "id-" + std::to_string(d.nextId);
    CHECK(documentIds(d).contains(d.network.id));
    test::throws([&] { allocateId(d, "id"); }, "EDIT_ID_LIMIT");
    CHECK(d.nextId == limit - 1); // A rejected candidate still consumes the counter, as before.
    d.network.id = "network"; d.nextId = limit - 2;
    CHECK(allocateId(d, "id") == "id-" + std::to_string(limit - 2));
    CHECK(d.nextId == limit - 1);
}
