#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace multra {
struct NodeAnalysisSourceFile {
    std::string logicalPath,sha256;
    bool operator==(const NodeAnalysisSourceFile&) const = default;
};
// Historical captured input, not a live binding to demand or a second geometry/timing copy.
struct NodeAnalysisModelSource {
    std::uint64_t revision{};
    std::string projectSha256;
    double start{},end{};
    std::vector<std::string> heavyVehicleTypes;
    std::vector<NodeAnalysisSourceFile> files;
    std::vector<std::string> fallbacks;
    bool operator==(const NodeAnalysisModelSource&) const = default;
};
}
