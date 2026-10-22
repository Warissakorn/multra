#pragma once
#include "document.hpp"
#include "../model/demand/meso_compile.hpp"
#include <filesystem>

namespace multra {
class InputManifest;
// M11.2 (docs/reference/MESO_NETWORK.md §3, §8, D293): what a Meso run reads from a document. The
// resolved link types (owned or data/link-types/) and the run settings compiled into the graph, with
// INVALID_LINK_TYPE (modes checked against the resolved modes) added to its issues.
MesoCompile mesoNetwork(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
}
