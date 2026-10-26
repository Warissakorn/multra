#pragma once
#include <nlohmann/json.hpp>

namespace multra {
struct ObservedJsonSchema {
    nlohmann::json schema;
    bool truncated{};
};
// Structural hints only: no required fields, ranges, references or codec version rules.
// Visit at most 50,000 values through depth 32. Unvisited array items remain unconstrained.
ObservedJsonSchema observeJsonSchema(const nlohmann::json& value);
}
