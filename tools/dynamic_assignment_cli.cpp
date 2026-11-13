// DYNAMIC_ASSIGNMENT §5-§6 (D549): the command-line side of the route-costs file.
#include "dynamic_assignment_cli.hpp"
#include "../src/project/route_costs.hpp"
#include <fstream>
#include <stdexcept>

namespace multra {
ProjectDocument withRouteCostsFile(ProjectDocument document, const std::filesystem::path& file) {
    if (file.empty()) return document;
    std::ifstream stream(file);
    if (!stream) throw std::runtime_error("Cannot read route costs: " + file.string());
    return applyRouteCosts(std::move(document), parseRouteCosts(nlohmann::json::parse(stream)));
}
}
