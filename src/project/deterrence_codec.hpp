#pragma once
// M10.3 D1 (TRIP_DISTRIBUTION §6, D473): a deterrence set (beta per purpose for the exponential
// gravity model, with an optional intrazonal share), its JSON form and the catalog in
// `data/deterrence/`. None ships (choice 3a); project-owned sets are D2.
#include "json.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace multra {
class InputManifest;
struct DeterrencePurpose {
    std::string id;
    double beta{};                          // per minute, finite and > 0
    std::optional<double> intrazonalShare;  // in [0, 1); absent means 0
    bool operator==(const DeterrencePurpose&) const = default;
};
struct DeterrenceSet {
    std::string id, name, citation, table;  // `table` may be blank
    std::string function{"exponential"}, costUnit{"min"};
    std::vector<DeterrencePurpose> purposes;
    bool operator==(const DeterrenceSet&) const = default;
};
struct ResolvedDeterrenceSet { DeterrenceSet set; std::string path; }; // path: e.g. catalog/deterrence/x.json

// Every shape fault at once, as INVALID_DETERRENCE_SET / INVALID_ID / DUPLICATE_ID at `path` + its
// JSON path. `function` must be "exponential" and `costUnit` "min". `requireCitation` for a catalog set.
DeterrenceSet parseDeterrenceSet(const Json&, const std::string& path, bool requireCitation);
Json deterrenceSetJson(const DeterrenceSet&);
// `data/deterrence/*.json` in file-name order; a missing folder is an empty catalog. Duplicate ids are
// DUPLICATE_ID at the later file's `.id`.
std::vector<ResolvedDeterrenceSet> resolveDeterrenceSets(const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
}
