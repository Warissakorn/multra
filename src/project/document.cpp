#include "document.hpp"
#include "zone_codec.hpp"
#include "demand_time_types.hpp"
#include "behaviour_library.hpp"
#include "evaluation_period.hpp"
#include "clamp_rules.hpp"
#include "macro_settings.hpp"
#include "meso_settings.hpp"
#include "study_period_codec.hpp"
#include "mode_codec.hpp"
#include "link_type_codec.hpp"
#include "road_codec.hpp"
#include "node_codec.hpp"
#include "counted_volumes.hpp"
#include "route_destination.hpp"
#include "speed_decisions.hpp"
#include "speed_areas.hpp"
#include "speed_curve.hpp"
#include "lane_preference.hpp"
#include "../core/validate.hpp"
#include "../model/network/diagnostics.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <set>
#include <limits>

namespace multra {
namespace {
// Written as names, not as the enum's integers: a project file a human opens should say `dashed`,
// and an added marking kind must not silently renumber what older files meant.
Json markingNames(const std::vector<MarkingType>& markings) {
    Json result=Json::array();
    for(const auto m:markings)result.push_back(markingName(m));
    return result;
}
}

namespace {
Json points(const std::vector<Point>& ps) {
    Json out = Json::array();
    for (const auto& p : ps) out.push_back({{"x", p.x}, {"y", p.y}});
    return out;
}
Json controlPath(const ControlPathRef& p) {
    if(p.connectorId.empty())return {{"linkId",p.linkId},{"laneId",p.laneId}};
    return {{"connectorId",p.connectorId},{"fromLaneId",p.fromLaneId},{"toLaneId",p.toLaneId}};
}
Json side(const ConflictSide& s) {
    return {{"path",controlPath(s.path)},{"entryStation",s.entryStation},{"exitStation",s.exitStation},
            {"waitingLineId",s.waitingLineId}};
}
// M3.2.2. Written only when there is something to write, so a project without controls saves
// exactly the keys it always did.
Json rightOfWayJson(const RightOfWay& row) {
    Json lines=Json::array(),areas=Json::array(),rules=Json::array();
    for(const auto& w:row.waitingLines)
        lines.push_back({{"id",w.id},{"name",w.name},{"point",{{"path",controlPath(w.point.path)},{"station",w.point.station}}}});
    for(const auto& a:row.conflictAreas)
        areas.push_back({{"id",a.id},{"name",a.name},{"kind",conflictKindName(a.kind)},{"first",side(a.first)},
                         {"second",side(a.second)},{"priority",conflictPriorityName(a.priority)}});
    for(const auto& r:row.priorityRules)
        rules.push_back({{"id",r.id},{"name",r.name},{"conflictAreaId",r.conflictAreaId},{"gapTime",r.gapTime},{"headway",r.headway}});
    Json result={{"waitingLines",lines},{"conflictAreas",areas},{"priorityRules",rules}};
    // M3.2.5, schema 15: only when there is one, so a file without controls keeps its keys.
    if(!row.stopControls.empty()) {
        Json controls=Json::array();
        for(const auto& c:row.stopControls)
            controls.push_back({{"id",c.id},{"name",c.name},{"waitingLineId",c.waitingLineId},
                                {"mode",stopModeName(c.mode)},{"conflictAreaIds",c.conflictAreaIds}});
        result["stopControls"]=controls;
    }
    return result;
}
Json reference(const LaneReference& r) {
    Json result={{"linkId",r.linkId},{"laneId",r.laneId}};
    if(r.station)result["station"]=*r.station;
    return result;
}
}
Json documentJson(const ProjectDocument& d) {
    Json network = {{"id", d.network.id}, {"drivingSide", d.network.drivingSide == DrivingSide::left ? "left" : "right"},
        {"links", Json::array()}, {"connectors", Json::array()}, {"signalHeads", Json::array()}};
    for (const auto& l : d.network.links) {
        Json lanes = Json::array();
        for (const auto& lane : l.lanes) lanes.push_back({{"id", lane.id}, {"width", lane.width}});
        network["links"].push_back({{"id", l.id}, {"geometry", points(l.geometry)}, {"lanes", lanes}, {"level",l.level}, {"displayType",l.displayType}, {"laneOffset",l.laneOffset}, {"name",l.name},
            {"boundaryMarkings",markingNames(l.boundaryMarkings)}});
        if (l.behaviourTypeId) network["links"].back()["behaviourType"] = *l.behaviourTypeId; // M3.3.2a, schema 21
        if (l.linkTypeId) network["links"].back()["linkType"] = *l.linkTypeId; // S2.1, schema 50
        if (l.roadId) network["links"].back()["roadId"] = *l.roadId; // S3.3, schema 53
    }
    for (const auto& c : d.network.connectors) {
        network["connectors"].push_back({{"id", c.id}, {"from", reference(c.from)}, {"to", reference(c.to)}, {"geometry", points(c.geometry)}, {"fromLaneCount",c.fromLaneCount}, {"toLaneCount",c.toLaneCount},
            {"level",c.level}, {"displayType",c.displayType}, {"laneBlend",c.laneBlend}, {"name",c.name},
            {"laneWidths",c.laneWidths}, {"laneMarkings",markingNames(c.laneMarkings)}});
        // M3.2.9a, schema 17: only when chosen, so the kerb-side default keeps a file's keys.
        if (c.laneChangeSide) network["connectors"].back()["laneChangeSide"] = *c.laneChangeSide == LaneSide::left ? "left" : "right";
        if (c.behaviourTypeId) network["connectors"].back()["behaviourType"] = *c.behaviourTypeId;
    }
    for (const auto& h : d.network.signalHeads) {
        network["signalHeads"].push_back({{"id", h.id}, {"lane", reference(h.lane)}, {"position", h.position}, {"programId", h.programId}, {"connectorId",h.connectorId}, {"name",h.name}});
        if (!h.controllerId.empty()) { network["signalHeads"].back()["controllerId"] = h.controllerId; network["signalHeads"].back()["groupNumber"] = h.groupNumber; }
    }
    if (!d.network.rightOfWay.empty()) network["rightOfWay"] = rightOfWayJson(d.network.rightOfWay);
    // M3.2.6b, schema 16: only when there is one, so a file without counters keeps its keys.
    if (!d.network.queueCounters.empty()) {
        Json counters = Json::array();
        for (const auto& c : d.network.queueCounters) {
            Json lines = Json::array();
            for (const auto& l : c.lines) {
                Json line = Json::object();
                if (!l.referenceId.empty()) line["referenceId"] = l.referenceId;
                if (l.point) line["point"] = {{"path", controlPath(l.point->path)}, {"station", l.point->station}};
                lines.push_back(line);
            }
            counters.push_back({{"id", c.id}, {"name", c.name}, {"lines", lines}});
        }
        network["queueCounters"] = counters;
    }
    // M5.4, schema 23: likewise only when there is one.
    if (!d.network.travelTimeSections.empty()) {
        Json sections = Json::array();
        const auto line = [](const SectionLine& l) { return Json{{"linkId", l.linkId}, {"station", l.station}}; };
        for (const auto& s : d.network.travelTimeSections)
        {
            sections.push_back({{"id", s.id}, {"name", s.name}, {"start", line(s.start)}, {"end", line(s.end)}});
            if (s.controlType) sections.back()["controlType"] = sectionControlName(*s.controlType); // M5.5, schema 24
        }
        network["travelTimeSections"] = sections;
    }
    addNodesJson(d.network, network); // S0.1, schema 48
    addRoadsJson(d.network, network); // S3.3, schema 53
    const auto& b = d.background;
    // Preserve legacy bytes; use 18 for owned catalogs and 19 for time/type rules.
    const bool positioned=d.definition && std::any_of(d.definition->routingDecisions.begin(),d.definition->routingDecisions.end(),[](const auto& x){return x.position.has_value();});
    // M3.3.2a: 21 only when the behaviour library or a road assignment is used (D126).
    const bool library=usesBehaviourLibrary(d);
    // M5.3: 22 only when an evaluation period is set (D132).
    const bool period=d.definition && d.definition->evaluation;
    const bool controlled=std::any_of(d.network.travelTimeSections.begin(),d.network.travelTimeSections.end(),[](const auto& s){return s.controlType.has_value();});
    // M3.3.3a: 25 only when an owned behaviour is w74 (D136).
    // D142: 26 only when an input takes its volume from the counts.
    // M5.9: 27 only when the evaluation period has a cool-down (D146).
    const bool cooled=period && d.definition->evaluation->cooldown>0;
    // M4.2: 28 only when an owned behaviour carries amberDeceleration (D147).
    // M8.2: 31 only when there is a zone layer (D154).
    // M3.5: 32 only when an owned behaviour carries a perception key (D181).
    // M3.7: 33 only when an owned behaviour carries a lateral key (D187).
    // M3.6: 34 only when an owned behaviour carries curveLateralAcceleration (D212).
    // M3.4: 35 only when an owned behaviour carries kerbLaneBias (D215); B-2: 36 for commitmentDeceleration (D219); B-3: 37 (D221).
    // B-6: 38 for speed decisions (D223), 39 for reduced-speed areas (D225); B-5: 40 for a speed curve (D229); B-8: 41 for stopDwellTime (D231); B-7: 42 for kerbLanes (D234); B-4: 43 for driverFactorSpread (D236); B-9: 44 for red running (D238); M3.2.8a.1: 45 for clampRules (D251), 46 for its joinAnticipation (D255), 47 for standingStopFloor off (D258); S0.1: 48 for nodes (D261); S1.1: 49 for study periods (D268); S1.2: 49 for modes and pcu (D270); S2.1: 50 for link types (D273); S2.3: 51 for macroSettings (D275); M11.2: 52 for mesoSettings and a link type's jamDensity (D293); S3.2: 53 for a link type's capacityBasis (D304); S3.3: 53 for roads, a Link's roadId and owned crossSections (D305); S3.5: 53 for overpasses (D309).
    const int schema=usesRoadFields(d)||usesCapacityBasis(d)?53:usesMesoFields(d)?52:usesMacroSettings(d)?51:usesLinkTypes(d)?50:usesStudyPeriods(d)||usesModes(d)?49:!d.network.nodes.empty()?48:ownsStandingStopFloor(d)?47:ownsJoinAnticipation(d)?46:ownsClampRules(d)?45:ownsRedRunning(d)?44:ownsDriverFactorSpread(d)?43:ownsKerbLanes(d)?42:ownsStopDwellBehaviour(d)?41:ownsSpeedCurve(d)?40:d.definition && !d.definition->reducedSpeedAreas.empty()?39:d.definition && !d.definition->speedDecisions.empty()?38:ownsMandatoryBehaviour(d)?37:ownsCommitmentBehaviour(d)?36:ownsKerbBehaviour(d)?35:ownsCurveBehaviour(d)?34:ownsLateralBehaviour(d)?33:ownsPerceptionBehaviour(d)?32:!d.zones.empty()?31:d.definition && !d.definition->routeDestinations.empty()?30:d.map?29:ownsAmberBehaviour(d)?28:cooled?27:usesCountedVolumes(d)?26:ownsW74Behaviour(d)?25:controlled?24:!d.network.travelTimeSections.empty()?23:period?22:library?21:positioned?20:d.definition && hasTimeTypeDemand(*d.definition)?19:d.definition && (!d.definition->externalCompositions ||
        (!d.definition->externalVehicleTypes && !d.definition->vehicleTypeNames.empty()))?18:17;
    Json definition = d.definition ? definitionJson(*d.definition) : Json(nullptr);
    // From schema 21 the reader requires every owned behaviour's model, so any file at 21 or later
    // tags them, not only one with a library: owned catalogs with an evaluation period (22) and no
    // library were written untagged and could not be reopened (found by M4.2, D147).
    if ((library || schema >= 21) && d.definition) addBehaviourLibraryJson(*d.definition, definition);
    if (d.definition) {addEvaluationPeriodJson(*d.definition, definition);addClampRulesJson(*d.definition, definition);addMacroSettingsJson(*d.definition, definition);addMesoSettingsJson(*d.definition, definition);addClockStartJson(*d.definition, definition);addRouteDestinationsJson(*d.definition,definition);addSpeedDecisionsJson(*d.definition,definition);addSpeedAreasJson(*d.definition,definition);}
    Json result={{"format", "TrafficSim"}, {"schemaVersion", schema}, {"nextId", d.nextId}, {"revision", d.revision}, {"network", network},
        {"definition", definition}, {"background", {{"pngBase64", *b.pngBase64}, {"x", b.x}, {"y", b.y},
            {"metresPerPixel", b.metresPerPixel}, {"rotation", b.rotation}, {"opacity", b.opacity}}}};
    if (d.map) result["map"]=mapSettingsJson(*d.map);
    addZoneLayerJson(d.zones,result);
    addStudyPeriodsJson(d.zones,result); // S1.1
    return result;
}
void validateDocument(const ProjectDocument& d) {
    auto issues = validateNetwork(d.network);
    // blocksDraft owns the "an empty network is a legal draft" rule, for this and the editor alike.
    std::erase_if(issues, [](const auto& i) { return !blocksDraft(i.code); });
    if (!issues.empty()) throw ValidationError(issues);
    if (!d.nextId || d.nextId == std::numeric_limits<std::uint64_t>::max() || d.revision == std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("EDIT_ID_LIMIT");
    const auto& b = d.background;
    if (!b.pngBase64 || !std::isfinite(b.x) || !std::isfinite(b.y) || !std::isfinite(b.rotation) ||
        !std::isfinite(b.metresPerPixel) || b.metresPerPixel <= 0 || !std::isfinite(b.opacity) ||
        b.opacity < 0 || b.opacity > 1 || b.pngBase64->size() > 32 * 1024 * 1024)
        throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
    if (d.map) validateMapSettings(*d.map);
    validateAuthoredDemand(d);
    validateRouteDestinations(d);
    if (d.definition) validateEvaluationPeriod(*d.definition);
    if (d.definition && !(std::isfinite(d.definition->clockStart) && d.definition->clockStart >= 0 && d.definition->clockStart <= kClockLimit))
        throw ValidationError({{"INVALID_NUMBER", "definition.clockStart"}}); // S1.1
    if (auto issues = behaviourLibraryIssues(d); !issues.empty()) throw ValidationError(std::move(issues));
    if (auto issues = zoneLayerIssues(d); !issues.empty()) throw ValidationError(std::move(issues));
}
ProjectDocument parseDocument(const Json& j) {
    ProjectDocument d;
    if (!j.is_object()) throw std::invalid_argument("EDIT_VERSION");
    if (j.contains("schemaVersion")) {
        // Every read here is guarded: a hand-edited null section must name itself, not surface
        // as an nlohmann type_error the user cannot act on.
        if (!present(j, "schemaVersion") || !j.at("schemaVersion").is_number_integer() || (j.at("schemaVersion") < 1 || j.at("schemaVersion") > 53) ||
            !present(j, "format") || j.at("format") != "TrafficSim")
            throw std::invalid_argument("EDIT_VERSION");
        if (!present(j, "nextId") || !j.at("nextId").is_number_unsigned() ||
            !present(j, "revision") || !j.at("revision").is_number_unsigned()) throw std::invalid_argument("EDIT_ID_LIMIT");
        d.nextId = j.at("nextId").get<std::uint64_t>();
        d.revision = j.at("revision").get<std::uint64_t>();
        if (!present(j, "background")) throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        const auto& b = j.at("background");
        if (!b.is_object() || !b.contains("pngBase64") || !b.at("pngBase64").is_string()) throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        for (const char* number : {"x", "y", "metresPerPixel", "rotation", "opacity"})
            if (!present(b, number) || !b.at(number).is_number()) throw std::invalid_argument("EDIT_BACKGROUND_INVALID");
        d.background = {std::make_shared<const std::string>(b.at("pngBase64").get<std::string>()), b.at("x").get<double>(), b.at("y").get<double>(),
            b.at("metresPerPixel").get<double>(), b.at("rotation").get<double>(), b.at("opacity").get<double>()};
    }
    if (!present(j, "network")) throw std::invalid_argument("EDIT_NO_NETWORK");
    if(j.contains("schemaVersion") && j.at("schemaVersion")<20 && present(j,"definition") && present(j.at("definition"),"routingDecisions") && j.at("definition").at("routingDecisions").is_array())
        for(const auto& x:j.at("definition").at("routingDecisions"))
            if(x.contains("position"))throw std::invalid_argument("UNSUPPORTED_FIELD: routingDecision.position");
    const int version = j.contains("schemaVersion") ? j.at("schemaVersion").get<int>() : 0;
    d.map=parseMapSettings(j,version);
    if (version < 49) { rejectStudyPeriodsBefore49(j); rejectModesBefore49(j); } // S1.1, S1.2: before the matrices are read
    if (version < 50) rejectLinkTypesBefore50(j); // S2.1, before the network is read
    if (version < 51) rejectMacroSettingsBefore51(j); // S2.3
    if (version < 52) rejectMesoFieldsBefore52(j); // M11.2
    if (version < 53) rejectCapacityBasisBefore53(j); // S3.2
    if (version < 53) rejectRoadFieldsBefore53(j); // S3.3
    d.zones=parseZoneLayer(j,version);
    d.zones.studyPeriods=parseStudyPeriods(j);
    if (version < 21) rejectBehaviourLibraryBefore21(j);
    if (version < 22) rejectEvaluationPeriodBefore22(j);
    if (version < 25) rejectW74Before25(j);
    if (version < 26) rejectCountedVolumesBefore26(j);
    if (version < 27) rejectCooldownBefore27(j);
    if (version < 28) rejectAmberBefore28(j);
    if (version < 32) rejectPerceptionBefore32(j);
    if (version < 33) rejectLateralBefore33(j);
    if (version < 34) rejectCurveBefore34(j);
    if (version < 35) rejectKerbBefore35(j);
    if (version < 36) rejectCommitmentBefore36(j);
    if (version < 37) rejectMandatoryBefore37(j);
    if (version < 40) rejectSpeedCurveBefore40(j);
    if (version < 41) rejectStopDwellBefore41(j);
    if (version < 42) rejectKerbLanesBefore42(j);
    if (version < 43) rejectDriverFactorSpreadBefore43(j);
    if (version < 44) rejectRedRunningBefore44(j);
    if (version < 45) rejectClampRulesBefore45(j);
    if (version < 46) rejectJoinAnticipationBefore46(j);
    if (version < 47) rejectStandingStopFloorBefore47(j);
    d.network = parseNetwork(j.at("network"), version);
    d.network.nodes = parseNodes(j.at("network"), version); // S0.1 (NODES §2)
    readLinkTypeIds(j.at("network"), d.network); // S2.1 (MACRO_NETWORK §4)
    d.network.roads = parseRoads(j.at("network"), version); // S3.3 (ROADS §3)
    d.network.overpasses = parseOverpasses(j.at("network"), version); // S3.5 (ROADS §5)
    readRoadIds(j.at("network"), d.network);
    if (present(j, "definition")) {
        d.definition = parseAuthoringDefinition(j.at("definition"));
        parseRouteDestinations(j.at("definition"),version,*d.definition);
        parseSpeedDecisions(j.at("definition"),version,*d.definition); // B-6, schema 38
        parseSpeedAreas(j.at("definition"),version,*d.definition); // B-6 §7, schema 39
        if (version >= 21) parseBehaviourLibrary(j.at("definition"), *d.definition);
        if (version >= 22) d.definition->evaluation = parseEvaluationPeriod(j.at("definition"));
        if (version >= 45) d.definition->clampRules = parseClampRules(j.at("definition")); // D251
        d.definition->clockStart = parseClockStart(j.at("definition")); // S1.1, gated above
        d.definition->macroSettings = parseMacroSettings(j.at("definition")); // S2.3, gated above
        d.definition->mesoSettings = parseMesoSettings(j.at("definition")); // M11.2, gated above
        // M4.2 (D147): a bare M0 scenario keeps amber as red, as the CLI reads it (loadScenario).
        if (!j.contains("schemaVersion")) d.definition->provenance.legacyAmber = true;
        // Schema 7 and earlier stored a route as lanes and Connector paths. Schema 8 stores the
        // Links and Connectors those belong to, so that narrowing a Connector cannot invalidate
        // a route. The mapping is idempotent, which is what lets it run on every read.
        migrateRoutesToObjects(d.network, *d.definition);
        // Projects only: an M0 scenario (no schemaVersion) keeps its programs, exactly as the CLI
        // compiles it, so the frozen fixtures and `multra-cli 42` cannot move.
        if (j.contains("schemaVersion") && j.at("schemaVersion").get<int>() < 13) migrateSignalPrograms(d);
    }
    validateDocument(d);
    return d;
}
std::set<std::string> documentIds(const ProjectDocument& d) {
    std::set<std::string> used{d.network.id};
    for (const auto& l : d.network.links) { used.insert(l.id); for (const auto& lane : l.lanes) used.insert(lane.id); }
    for (const auto& c : d.network.connectors)
        for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)used.insert(connectorPathId(c,i));
    for (const auto& h : d.network.signalHeads) used.insert(h.id);
    for (const auto& w : d.network.rightOfWay.waitingLines) used.insert(w.id);
    for (const auto& a : d.network.rightOfWay.conflictAreas) used.insert(a.id);
    for (const auto& r : d.network.rightOfWay.priorityRules) used.insert(r.id);
    for (const auto& c : d.network.rightOfWay.stopControls) used.insert(c.id);
    for (const auto& c : d.network.queueCounters) used.insert(c.id);
    for (const auto& s : d.network.travelTimeSections) used.insert(s.id);
    for (const auto& n : d.network.nodes) used.insert(n.id);
    for (const auto& r : d.network.roads) used.insert(r.id);
    if (d.definition) {
        for (const auto& r : d.definition->routes) used.insert(r.id);
        for (const auto& i : d.definition->inputs) used.insert(i.id);
        for (const auto& p : d.definition->signalPrograms) used.insert(p.id);
        for (const auto& c : d.definition->signalControllers) used.insert(c.id);
        for (const auto& decision : d.definition->routingDecisions) used.insert(decision.id);
    }
    return used;
}
std::string allocateId(ProjectDocument& d, const std::string& prefix) {
    auto used = documentIds(d);
    for (const auto& z : d.zones.zones) used.insert(z.id);
    for (const auto& c : d.zones.zoneConnectors) used.insert(c.id);
    for (const auto& m : d.zones.odMatrices) used.insert(m.id);
    for (const auto& l : d.zones.parkingLots) used.insert(l.id);
    for (const auto& p : d.zones.studyPeriods) used.insert(p.id);
    if (d.definition) for (const auto& m : d.definition->modes) used.insert(m.id);
    for (;;) {
        if (d.nextId >= std::numeric_limits<std::uint64_t>::max() - 1) throw std::invalid_argument("EDIT_ID_LIMIT");
        auto id = prefix + "-" + std::to_string(d.nextId++);
        if (!used.contains(id)) return id;
    }
}
}
