#pragma once
#include "document.hpp"

namespace multra {
// M11.2 (MESO_NETWORK §8, D293): `definition.mesoSettings{step, slice}` and a link type's
// `jamDensity`, each written only when set, which is what makes a file schema 52; absent settings
// mean kMesoDefaults.
bool usesMesoFields(const ProjectDocument&);
void rejectMesoFieldsBefore52(const Json& file);
// INVALID_MESO_SETTING at the field's path: a step outside (0, 60] s, a slice that is not a whole
// number of steps of at least one step, a missing or unknown key.
std::optional<MesoSettings> parseMesoSettings(const Json& definition);
std::vector<ValidationIssue> mesoSettingsIssues(const MesoSettings&);
void addMesoSettingsJson(const AuthoringDefinition&, Json& definition);
// The settings a run uses: the project's, else the defaults.
MesoSettings mesoSettingsOf(const ProjectDocument&);
}
