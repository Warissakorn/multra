#pragma once
#include <nlohmann/json_fwd.hpp>

namespace multra {
// Partial draft-2020-12 schema for editor schemas 1..kLatestSchema.
// Composes the stable envelope API with versioned Network/Link/Lane/geometry fields.
// Does not validate model invariants or nested Connector/control/demand features.
nlohmann::json projectDocumentSchema();
}
