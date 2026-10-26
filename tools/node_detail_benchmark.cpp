// F4 benchmark: unconnected four-arm coarse Nodes, matching ENGINEERING_NOTES' D343 recipe.
// Not in check: timings are observations, never acceptance assertions.
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/node_detail_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace multra;
namespace {
ProjectDocument drawing(int count) {
    ProjectDocument d;
    d.definition = AuthoringDefinition{};
    putCrossSection(d, {"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::nullopt, "Test template"}, {});
    for (int k = 0; k < count; ++k) {
        const double x = k * 1000.;
        std::vector<std::string> roads;
        for (const auto& line : std::vector<std::vector<Point>>{
                 {{x, 100}, {x, 0}}, {{x + 100, 0}, {x, 0}},
                 {{x, -100}, {x, 0}}, {{x - 100, 0}, {x, 0}}}) {
            const auto id = putRoad(d, {"", "", line, std::string("ns"), 0, {}, {}});
            roads.push_back(id);
            applyTemplate(d, id, d.definition->crossSections);
        }
        const auto id = putNode(d, {"", "", {{x - 10, -10}, {x + 10, -10},
                                                    {x + 10, 10}, {x - 10, 10}}, false});
        setNodeCoarse(d, id, NodeCoarse{std::nullopt, {roads[0], roads[2]}, {}, std::nullopt});
    }
    return d;
}
}
// Usage: multra-node-detail-benchmark [count=4] [first-document.json]
int main(int argc, char** argv) {
    try {
        const std::string argument = argc > 1 ? argv[1] : "4";
        std::size_t consumed{};
        const int count = std::stoi(argument, &consumed);
        if (argc > 3 || consumed != argument.size() || count < 1 || count > 100)
            throw std::runtime_error("Usage: multra-node-detail-benchmark [count: 1..100] [output.json]");
        for (int repeat = 0; repeat < 3; ++repeat) {
            auto d = drawing(count);
            std::vector<std::string> nodes;
            for (const auto& node : d.network.nodes) nodes.push_back(node.id);
            const NodeFillCatalogs catalogs{d.definition->crossSections, {}, {3, 7}};
            const auto start = std::chrono::steady_clock::now();
            for (const auto& id : nodes) generateNodeDetail(d, id, catalogs);
            std::cout << std::chrono::duration<double, std::milli>(
                             std::chrono::steady_clock::now() - start).count() << " ms\n";
            if (repeat == 0 && argc > 2) {
                std::ofstream output;
                output.exceptions(std::ios::failbit | std::ios::badbit);
                output.open(argv[2]);
                output << documentJson(d).dump(2) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
