#pragma once
#include <nlohmann/json_fwd.hpp>

namespace multra {
// Partial draft-2020-12 schema for editor schemas 1..kLatestSchema.
// Composes the stable envelope API with versioned Network/Link/Lane/Connector/SignalHead/geometry/reference and signal/right-of-way control measurement and base Route/VehicleInput fields.
// Does not validate model invariants or other demand features.
nlohmann::json projectDocumentSchema();
}
