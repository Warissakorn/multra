#pragma once
// S0.1 (NODES §2, D261): `network.nodes`, written only when a Node exists, which makes the file
// schema 48. Below 48 the network's known fields refuse the key (EDIT_UNSUPPORTED_FIELD network.nodes).
#include "json.hpp"

namespace multra {
void addNodesJson(const Network&, Json& network);
// A malformed Node, an unknown key or a point that is not two numbers is EDIT_NODE_INVALID at its
// path; what the shape cannot say (ids, a simple polygon, overlap) is nodeIssues' on validation.
std::vector<Node> parseNodes(const Json& network, int schema);
// S4.5a (READINESS §13.6, D323): an object's `generated`, written only when true from schema 54. The
// object without the key, for its known-fields check (unchanged below 54, where the key is refused);
// and the flag itself, EDIT_GENERATED_INVALID at `where.generated` when it is anything but true.
// usesGeneratedFlags: whether any object carries the flag, which makes the file schema 54.
Json withoutGenerated(const Json& object, int schema);
bool generatedFlag(const Json& object, const std::string& where, int schema);
bool usesGeneratedFlags(const Network&);
}
