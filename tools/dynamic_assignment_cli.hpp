// M9 dynamic assignment on the command line (docs/reference/DYNAMIC_ASSIGNMENT.md §5-§6, D549).
#pragma once
#include "../src/project/document.hpp"
#include <filesystem>

namespace multra {
// --route-costs: the file's weights replace the project's decision flows; an empty path leaves the document as it is.
ProjectDocument withRouteCostsFile(ProjectDocument, const std::filesystem::path& file);
}
