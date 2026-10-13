#pragma once
// M8.2 (D150, docs/reference/ZONES_AND_OD.md §1, §2): the schema 29 codec and the document checks
// of zones, zone connectors and OD matrices. Nothing here expands a matrix (M8.3).
#include "document.hpp"
#include "../core/validate.hpp"

namespace trafficsim {
// EDIT_UNSUPPORTED_FIELD on network.zones, network.zoneConnectors or definition.odMatrices below
// schema 29. Called by parseDocument and by the CLI's M0 loader (loadScenario).
void rejectZonesAndOdBefore29(const Json& file);
void parseZones(const Json& network, Network&);
std::vector<OdMatrix> parseOdMatrices(const Json& definition);
// Each key only when non-empty, and a connector's share only when it is not 1.
void addZonesJson(const Network&, Json& network);
void addOdMatrixJson(const AuthoringDefinition&, Json& definition);
// What makes a file schema 29.
bool usesZonesOrOd(const ProjectDocument&);
// A matrix with a positive cell is demand, wherever inputs are (§2).
bool hasOdDemand(const AuthoringDefinition&);
// The §1 structural checks of the matrices and the `od:` id reservation; zones and connectors
// are checked with the network (zoneIssues). Paths name the authored object.
std::vector<ValidationIssue> odMatrixIssues(const ProjectDocument&);
// M8.2 only, removed by M8.3: one OD_NOT_EXPANDED on odMatrices[i] for each matrix with a
// positive cell, so no trip is dropped silently while the expansion does not exist.
std::vector<ValidationIssue> odNotExpandedIssues(const AuthoringDefinition&);
// Whether an input, a routing decision's type rule or an OD matrix names the type or composition;
// the catalog refuses to delete one that is named.
bool definitionNamesVehicleType(const AuthoringDefinition&, const std::string& id);
bool definitionNamesComposition(const AuthoringDefinition&, const std::string& id);
}
