// Real CLI coverage for the study-area cut and side-file provenance (STUDY_AREA §8–§11).
// Generate this small synthetic project instead of freezing another large drawing in the tree.
#include "study_area_fixture.hpp"
#include "../src/project/export_marking.hpp"
#include "../src/project/od_demand.hpp"
#include <iostream>

namespace {
void write(const std::filesystem::path& file, const std::string& bytes) {
    std::ofstream out(file, std::ios::binary);
    out << bytes;
    out.close();
    CHECK(out);
}
}
int main(int argc, char** argv) {
    try {
        CHECK(argc == 2);
        const std::filesystem::path directory = argv[1];
        std::filesystem::create_directories(directory);
        auto c = mesoCorridor(20);
        auto& d = c.d;
        // A direct type input exercises the type: Meso class through the real CLI too.
        const auto expanded = withOdDemand(d);
        CHECK(expanded.inputs.size() == 1 && !expanded.routes.empty());
        d.definition->routes = expanded.routes;
        d.definition->inputs = expanded.inputs;
        d.definition->inputs.front().compositionId.clear();
        d.definition->inputs.front().vehicleTypeId = "heavy-vehicle";
        d.zones.odMatrices.clear();
        setStudyAreaNodes(d, {c.a});
        d.zones.studyArea.flowSource = "meso";
        const auto kept = studyAreaCut(d.network, d.zones.studyArea.nodes).links;
        const auto run = runMeso(d, test::root() / "data", nullptr, nullptr, &kept);
        CHECK(run.classes.size() == 1 && run.classes.front().id == "type:heavy-vehicle");
        CHECK(!run.loading.watched.empty());
        const auto sha = projectSha256(d);
        const auto fresh = mesoSideJson(run, sha, kept);
        auto stale = fresh;
        stale["projectSha256"] = std::string(64, '0');
        CHECK(parseMesoSide(stale).projectSha256 != sha); // assert the stale setup first
        const auto freshBytes = fresh.dump(2) + "\n", staleBytes = stale.dump(2) + "\n";
        CHECK(inputSha256(freshBytes) != inputSha256(staleBytes));
        const auto generated = generatedInArea(d);
        CHECK(generated > 0);
        for (const auto* name : {"base", "alternative", "no-side"})
            write(directory / (std::string(name) + ".traffic.json"), documentBytes(d));
        write(sideFilePath(directory / "base.traffic.json", "meso"), freshBytes);
        write(sideFilePath(directory / "alternative.traffic.json", "meso"), staleBytes);
        const Json expected{{"project", sha}, {"baseSource", inputSha256(freshBytes)},
                            {"alternativeSource", inputSha256(staleBytes)}, {"generated", generated}};
        write(directory / "expected.json", expected.dump(2));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
