#pragma once
#include "document.hpp"

namespace multra {
// M3.3.2a (D126, docs/reference/DRIVING_BEHAVIOUR.md §1): the project-owned behaviour library --
// behaviour names and model tags, vehicle classes, link behaviour types -- and the Link/Connector
// assignment. Schema 21. Stored and validated only; nothing reaches the engine before M3.3.2b.
bool usesBehaviourLibrary(const ProjectDocument&); // anything that needs schema 21 (or 25) to be saved
// A file below schema 21 carrying a library key is refused (EDIT_UNSUPPORTED_FIELD), never dropped.
void rejectBehaviourLibraryBefore21(const Json& document);
// D136: a file below schema 25 naming the w74 model is refused (UNSUPPORTED_BEHAVIOUR_MODEL).
void rejectW74Before25(const Json& document);
// Schema 25 is needed exactly when an owned behaviour is `w74`.
bool ownsW74Behaviour(const ProjectDocument&);
// M4.2 (D147): schema 28 exactly when an owned behaviour carries `amberDeceleration`; a file below
// 28 carrying the key in an owned behaviour is refused (EDIT_UNSUPPORTED_FIELD).
bool ownsAmberBehaviour(const ProjectDocument&);
void rejectAmberBefore28(const Json& document);
// M3.5 (D181, PERCEPTION.md §6): schema 32 exactly when an owned behaviour carries a perception
// key; a file below 32 carrying one is refused (EDIT_UNSUPPORTED_FIELD, path to the key).
bool ownsPerceptionBehaviour(const ProjectDocument&);
void rejectPerceptionBefore32(const Json& document);
// M3.7 (D187, LATERAL.md §9): the same for the lateral keys at schema 33.
bool ownsLateralBehaviour(const ProjectDocument&);
void rejectLateralBefore33(const Json& document);
bool ownsCurveBehaviour(const ProjectDocument&);
void rejectCurveBefore34(const Json& document);
bool ownsMandatoryBehaviour(const ProjectDocument&); // B-3 (D221): schema 37
void rejectMandatoryBefore37(const Json& document);
bool ownsStopDwellBehaviour(const ProjectDocument&); // B-8 (D231): schema 41
void rejectStopDwellBefore41(const Json& document);
bool ownsDriverFactorSpread(const ProjectDocument&); // B-4 (D236): schema 43
void rejectDriverFactorSpreadBefore43(const Json& document);
bool ownsRedRunning(const ProjectDocument&); // B-9 (D238): schema 44
void rejectRedRunningBefore44(const Json& document);
bool ownsCommitmentBehaviour(const ProjectDocument&); // B-2 (D219): schema 36
void rejectCommitmentBefore36(const Json& document);
bool ownsKerbBehaviour(const ProjectDocument&);
void rejectKerbBefore35(const Json& document);
// Schema 21 and later: reads the library and rejects unknown keys in every owned catalog entry.
void parseBehaviourLibrary(const Json& definition, AuthoringDefinition&);
// Adds the schema-21 keys to an already written definition: names, the explicit model tag on
// every owned behaviour, classes and behaviour types (the last two only when non-empty).
void addBehaviourLibraryJson(const AuthoringDefinition&, Json& definition);
// Every rule applies to unused entries too. Empty when the document uses no library feature.
std::vector<ValidationIssue> behaviourLibraryIssues(const ProjectDocument&);
// M3.3.2b (D127): one entry per runtime segment of an assigned road and per vehicle type -- the
// type's class override, else the behaviour type's default. Every section of a Link inherits the
// Link's assignment; every path of a Connector the Connector's own. A segment has exactly one
// owner, so routes can never disagree about it. Empty when no road is assigned.
std::vector<SegmentBehaviour> compileBehaviourAssignments(const Network&, const AuthoringDefinition&);
// The behaviour each vehicle type uses on a road with this assignment, and why (M3.3.2c shows it
// beside the road; the compiler above uses the same function, so the two cannot disagree).
// Unassigned: every type's own behaviourId. An unknown behaviour type is UNKNOWN_BEHAVIOUR_TYPE.
enum class BehaviourSource { inherited, typeDefault, classOverride };
struct RoadBehaviour {
    std::string vehicleTypeId, behaviourId; BehaviourSource source{};
    bool operator==(const RoadBehaviour&) const = default;
};
std::vector<RoadBehaviour> effectiveRoadBehaviours(const AuthoringDefinition&, const std::optional<std::string>& behaviourTypeId);
}
