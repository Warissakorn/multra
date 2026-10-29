#pragma once
// M14 J3a (JUNCTION_TEMPLATES §3–§4): structural catalog entries and site recipes.
// Lane rows run far to kerb, unlike Link lane order. No layout or generated object is stored here.
#include "../../core/validate.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace multra {
enum class PocketSide { far, kerb };
enum class ApproachDirection { north, east, south, west };
using ApproachDirections = std::map<std::string, ApproachDirection>; // in-leg Link ids
struct JunctionLane {
    std::string movements; // F, T, K, FT, TK or FTK, in that order
    bool pocket{};
    std::optional<PocketSide> side{}; // absent: F far, K kerb
    bool operator==(const JunctionLane&) const = default;
};
struct JunctionTemplate {
    std::string id, name, kind{"junction"};
    int legs{4};
    std::string control, phasing;
    std::map<std::string, std::vector<JunctionLane>> roles;
    bool operator==(const JunctionTemplate&) const = default;
};
struct JunctionLeg {
    std::vector<std::optional<double>> storage, taper; // metres, null = not entered
    std::optional<std::vector<JunctionLane>> lanes{}; // override of the role's rows
    bool operator==(const JunctionLeg&) const = default;
};
struct JunctionPhase {
    int number{};
    std::optional<double> green{}, yellow{}, allRed{}; // seconds, absent = not entered
    bool operator==(const JunctionPhase&) const = default;
};
struct JunctionSignal {
    std::optional<double> cycle{};
    std::vector<JunctionPhase> phases;
    bool operator==(const JunctionSignal&) const = default;
};
struct NodeJunction {
    std::string templateId;
    std::vector<std::string> major; // Street segment ids, empty = by rank
    std::map<std::string, JunctionLeg> legs; // Street segment ids, as NodeCoarse
    std::optional<JunctionSignal> signal{};
    bool operator==(const NodeJunction&) const = default;
};
std::vector<ValidationIssue> junctionLaneIssues(const std::vector<JunctionLane>&, const std::string& path);
std::vector<ValidationIssue> junctionTemplateIssues(const JunctionTemplate&, const std::string& path);
std::vector<ValidationIssue> junctionRecipeIssues(const NodeJunction&, const std::string& path);
}
