// M12 R1 row SC7 (docs/reference/SUBAREA_CONSISTENCY.md §7, D553): writes the projects the real CLI runs.
#include "study_area_fixture.hpp"
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
        auto c = mesoCorridor(100);
        auto& d = c.d;
        write(directory / "no-area.traffic.json", documentBytes(d));
        setStudyAreaNodes(d, {c.a});
        d.zones.studyArea.flowSource = "macro";
        const auto sha = projectSha256(d);
        // A stale Macro side file: the period's paths, recorded for other project bytes.
        auto side = macroSideJson(runMacro(d, test::root() / "data", "p", nullptr, nullptr, true), std::string(64, '0'), Json());
        const auto sideBytes = side.dump(2) + "\n";
        for (const auto* name : {"area", "stale"}) write(directory / (std::string(name) + ".traffic.json"), documentBytes(d));
        write(sideFilePath(directory / "stale.traffic.json", "macro"), sideBytes);
        write(directory / "expected.json", Json{{"project", sha}, {"source", inputSha256(sideBytes)}}.dump(2));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
