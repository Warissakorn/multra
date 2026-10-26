#pragma once
namespace multra {
struct ProjectDocument;
// Feature-selected writer version (17..kLatestSchema), independent of the input version.
// Inspection only: does not validate or serialize; nested codec rules still govern loading.
int documentSchemaVersion(const ProjectDocument& document);
}
