// DYNAMIC_ASSIGNMENT §5-§6 (D549): the command-line side of the route-costs file.
#include "dynamic_assignment_cli.hpp"
#include "../src/project/dynamic_assignment_loop.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace multra {
ProjectDocument withRouteCostsFile(ProjectDocument document, const std::filesystem::path& file, bool meso) {
    if (file.empty()) return document;
    std::ifstream stream(file);
    if (!stream) throw std::runtime_error("Cannot read route costs: " + file.string());
    return applyRouteCosts(std::move(document), parseRouteCosts(nlohmann::json::parse(stream)), meso ? DaLevel::meso : DaLevel::micro);
}
int runDynamicAssignmentProject(const std::filesystem::path& project, const std::filesystem::path& out,
                                const std::filesystem::path& data, std::uint32_t seed, int iterations, double gap, bool meso) {
    if (std::filesystem::exists(out)) throw std::invalid_argument("Route costs output already exists: " + out.string());
    std::ifstream stream(project);
    if (!stream) throw std::runtime_error("Cannot read project: " + project.string());
    const auto document = parseDocument(nlohmann::json::parse(stream));
    const auto costs = runDynamicAssignment(document, data, {iterations, gap}, seed, project.filename().string(), std::cerr,
                                            meso ? DaLevel::meso : DaLevel::micro);
    std::ofstream file(out);
    file << routeCostsJson(costs).dump(2) << '\n';
    if (!file) throw std::runtime_error("Failed writing route costs: " + out.string());
    return 0;
}
}
