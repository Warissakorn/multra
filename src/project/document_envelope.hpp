#pragma once
#include <nlohmann/json_fwd.hpp>

namespace multra {
struct ProjectDocument;
// Partial draft-2020-12 schema: envelope/background only, independent of any sample.
// Native integer-token and UTF-8 byte constraints are annotated, not JSON Schema rules.
nlohmann::json projectEnvelopeSchema();
// The existing versioned-project read and typed validation stages, with unchanged errors.
// Call read only for a project object containing schemaVersion, never a legacy scenario.
void readProjectEnvelope(const nlohmann::json&,ProjectDocument&);
void validateProjectEnvelope(const ProjectDocument&);
}
