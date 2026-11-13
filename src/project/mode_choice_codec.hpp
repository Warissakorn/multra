#pragma once
// M10.4 E1 (MODE_CHOICE §6, D478): a mode-choice set (per purpose, MNL alternatives with a constant, a cost
// coefficient, a network mode and/or an owned cost matrix, and an optional occupancy), its JSON form and the
// catalog in `data/mode-choice/`. None ships (rule 4); project-owned sets are E2.
#include "json.hpp"
#include "../core/validate.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace multra {
class InputManifest;
struct ModeAlternative {
    std::string id;
    std::optional<std::string> modeId, costMatrix;  // at least one
    double constant{}, timeCoefficient{};            // utility = constant + timeCoefficient * minutes; coefficient <= 0
    std::optional<double> occupancy;                 // persons per vehicle, >= 1; absent: persons only
    bool operator==(const ModeAlternative&) const = default;
};
// M10.4 N1 (NESTED_LOGIT §3, D481): a nest of the two-level nested logit; phi in (0, 1] (Ortuzar Eq. 7.20).
struct ModeChoiceNest {
    std::string id;
    double phi{1};
    std::vector<std::string> alternatives;   // ids of the purpose's alternatives, each in at most one nest
    bool operator==(const ModeChoiceNest&) const = default;
};
struct ModeChoicePurpose {
    std::string id;
    std::vector<ModeAlternative> alternatives;
    std::vector<ModeChoiceNest> nests;       // empty: the MNL
    std::optional<std::string> distributionCost; // D548 (MODE_CHOICE_LOGSUM §2): "logsum" or absent (the skim)
    bool operator==(const ModeChoicePurpose&) const = default;
};
struct ModeChoiceSet {
    std::string id, name, citation, table;
    std::string costUnit{"min"};
    std::vector<ModeChoicePurpose> purposes;
    bool operator==(const ModeChoiceSet&) const = default;
};
struct ResolvedModeChoiceSet { ModeChoiceSet set; std::string path; }; // e.g. catalog/mode-choice/x.json
// Every shape fault at once, as MODE_CHOICE_INVALID_SET / INVALID_ID / DUPLICATE_ID at `path` + its JSON
// path: costUnit "min"; a purpose needs a non-empty list of alternatives with unique ids and at least one
// constant equal to 0 (only differences are identified, Ortuzar §7.3.1; refused at the purpose);
// timeCoefficient finite and <= 0; a modeId or a costMatrix (non-blank strings); occupancy finite and >= 1.
// N1: optional `nests`, present means non-empty; nest ids unique; phi finite in (0, 1]; members a non-empty
// list naming the purpose's alternatives, each alternative in at most one nest (DUPLICATE_ID at the member).
ModeChoiceSet parseModeChoiceSet(const Json&, const std::string& path, bool requireCitation);
Json modeChoiceSetJson(const ModeChoiceSet&);
// M10.4 E2 (§6, D479): the project's own sets, root `modeChoiceSets` at schema 70, as D474's deterrence sets:
// a present list is a non-empty array (MODE_CHOICE_INVALID_SET at `modeChoiceSets`), rows parsed at
// `modeChoiceSets[i]` with the citation optional, ids unique. Absent or empty lists are not written.
std::vector<ModeChoiceSet> parseOwnedModeChoiceSets(const Json& file);
void addOwnedModeChoiceSetsJson(const std::vector<ModeChoiceSet>&, Json& file);
std::vector<ValidationIssue> ownedModeChoiceSetIssues(const std::vector<ModeChoiceSet>&);
// The installed `data/mode-choice/*.json` in name order (none when the directory is missing); ids unique.
std::vector<ResolvedModeChoiceSet> resolveModeChoiceSets(const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
}
