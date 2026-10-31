#pragma once
#include "run.hpp"
#include "input_manifest.hpp"
#include <nlohmann/json.hpp>
namespace multra {
struct NodeModelDemandRequest {
    double start{},end{}; // explicit run seconds, not clock time
    std::vector<std::string> heavyVehicleTypes; // explicit HCM classification, never inferred
};
struct NodeModelDemand {
    std::vector<NodeAnalysisTurn> turns; // every drawn turn, including explicit zero
    std::uint64_t revision{};
    Json inputManifest; // the exact captured project/catalog bytes used by compileDocument
};
// Expected scheduled demand, not observed throughput: compile the same captured bytes as Run,
// integrate only the requested window and map complete runtime traversals to derived Node turns.
// Refuses unresolved runtime contributions rather than publishing a partial/guessed Fill.
NodeModelDemand previewNodeModelDemand(std::string_view projectBytes,const std::string& nodeId,
    const NodeModelDemandRequest&,const std::filesystem::path& data,const StudyAreaSources* =nullptr);
}
