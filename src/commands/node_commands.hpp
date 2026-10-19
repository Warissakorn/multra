#pragma once
#include "history.hpp"
namespace trafficsim {
// S0.1 (NODES §2, D261). Empty id allocates `node`; the same id replaces it, and replacing clears
// `generated`: an edit is a person taking the Node over. Checked on commit (nodeIssues), not here.
std::string putNode(ProjectDocument&, Node);
void renameNode(ProjectDocument&, const std::string& id, const std::string& name);
void deleteNode(ProjectDocument&, const std::string& id); // EDIT_UNKNOWN_OBJECT when absent
}
