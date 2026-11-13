// M9 dynamic assignment on the command line (docs/reference/DYNAMIC_ASSIGNMENT.md §5-§6, D549).
#pragma once
#include "../src/project/document.hpp"
#include <cstdint>
#include <filesystem>

namespace multra {
// --route-costs: the file's weights replace the project's decision flows; an empty path leaves the document as it is.
ProjectDocument withRouteCostsFile(ProjectDocument, const std::filesystem::path& file);
// --dynamic-assignment OUT: the MSA loop over the project's decisions (§4, §6). Refuses an existing OUT before running;
// prints one stderr line per iteration and returns 0 even when the loop did not converge.
int runDynamicAssignmentProject(const std::filesystem::path& project, const std::filesystem::path& out,
                                const std::filesystem::path& data, std::uint32_t seed, int iterations, double gap);
}
