// SUBAREA_CONSISTENCY §7 (D553): --consistency-report.
#include "consistency_cli.hpp"
#include "../src/project/load.hpp"
#include "../src/project/side_files.hpp"
#include "../src/project/subarea_consistency.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace multra {
std::optional<int> consistencyCommand(int argc, char** argv) {
    bool wanted = false;
    for (int i = 1; i < argc; ++i) wanted = wanted || std::string(argv[i]) == "--consistency-report";
    if (!wanted) return std::nullopt;
    std::filesystem::path project, out, csv, data;
    std::vector<std::uint32_t> seeds;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg != "--project" && arg != "--consistency-report" && arg != "--seeds" && arg != "--csv" && arg != "--data-dir")
            throw std::invalid_argument("--consistency-report takes only --project, --seeds, --csv and --data-dir");
        if (++i == argc) throw std::invalid_argument("Missing value for " + arg);
        auto& slot = arg == "--project" ? project : arg == "--consistency-report" ? out : arg == "--csv" ? csv : data;
        if (arg == "--seeds") {
            if (!seeds.empty()) throw std::invalid_argument("Specify --seeds only once");
            seeds = parseSeedList(argv[i]);
        } else {
            if (!slot.empty()) throw std::invalid_argument("Specify " + arg + " only once");
            slot = argv[i];
        }
    }
    if (project.empty()) throw std::invalid_argument("--consistency-report needs --project");
    if (seeds.empty()) seeds = {42, 43, 44, 45, 46};
    if (data.empty()) data = findDataDirectory(argv[0]);
    for (const auto& f : {out, csv})
        if (!f.empty() && std::filesystem::exists(f)) throw std::invalid_argument("Output already exists: " + f.string());
    std::ifstream stream(project);
    if (!stream) throw std::runtime_error("Cannot read project: " + project.string());
    const auto document = parseDocument(nlohmann::json::parse(stream));
    const auto report = consistencyReport(document, data, readStudyAreaSources(project), seeds, project.filename().string());
    std::ofstream json(out);
    json << consistencyJson(report).dump(2) << '\n';
    if (!json) throw std::runtime_error("Failed writing the report: " + out.string());
    if (!csv.empty()) {
        std::ofstream file(csv);
        file << consistencyCsv(report);
        if (!file) throw std::runtime_error("Failed writing the CSV: " + csv.string());
    }
    return 0;
}
}
