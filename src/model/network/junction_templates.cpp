#include "junction_templates.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace multra {
namespace {
bool blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
bool positive(double n) { return std::isfinite(n) && n > 0; }
}
std::vector<ValidationIssue> junctionLaneIssues(const std::vector<JunctionLane>& lanes, const std::string& path) {
    std::vector<ValidationIssue> issues;
    const auto bad = [&](const std::string& at) { issues.push_back({"JUNCTION_TEMPLATE_INVALID",at}); };
    if (lanes.empty()) bad(path);
    int previous = -1;
    for (std::size_t i = 0; i < lanes.size(); ++i) {
        const auto& row = lanes[i]; const auto at = path+"["+std::to_string(i)+"]";
        const std::vector<std::string> valid{"F","T","K","FT","TK","FTK"};
        if (std::find(valid.begin(),valid.end(),row.movements) == valid.end()) { bad(at+".movements"); continue; }
        const auto first = std::string("FTK").find(row.movements.front());
        const auto last = std::string("FTK").find(row.movements.back());
        if (static_cast<int>(first) < previous) bad(at+".movements");
        previous = static_cast<int>(last);
        if (row.pocket && row.movements.find('T') != std::string::npos) bad(at+".pocket");
        if (row.side && (!row.pocket || (*row.side != PocketSide::far && *row.side != PocketSide::kerb))) bad(at+".side");
    }
    return issues;
}
std::vector<ValidationIssue> junctionTemplateIssues(const JunctionTemplate& t, const std::string& path) {
    std::vector<ValidationIssue> issues;
    const auto bad = [&](const std::string& field) { issues.push_back({"JUNCTION_TEMPLATE_INVALID",path+"."+field}); };
    if (blank(t.id)) bad("id");
    if (blank(t.name)) bad("name");
    if (t.kind == "transition") {
        if(t.legs!=2)bad("legs");
        if(!t.control.empty())bad("control");
        if(!t.phasing.empty())bad("phasing");
        if(!t.roles.empty())bad("roles");
        for(const auto* direction:{"forward","backward"})if(!t.directions.contains(direction))bad(std::string("directions.")+direction+".align");
        for(const auto& [direction,side]:t.directions) {
            if(direction!="forward"&&direction!="backward")bad("directions."+direction);
            if(side!=PocketSide::kerb&&side!=PocketSide::far)bad("directions."+direction+".align");
        }
        return issues;
    }
    if (t.kind != "junction") bad("kind");
    if (!t.directions.empty())bad("directions");
    if (t.legs != 3 && t.legs != 4) bad("legs");
    if (t.control != "signalised" && t.control != "twoWayStop" && t.control != "allWayStop") bad("control");
    const std::set<std::string> presets{"P1","P2","P3","P4"};
    if (t.control == "signalised" ? !presets.contains(t.phasing) : !t.phasing.empty()) bad("phasing");
    const std::vector<std::string> roles = t.legs == 3 ? std::vector<std::string>{"major-far","major-kerb","minor"}
                                                   : std::vector<std::string>{"major","minor"};
    for (const auto& role : roles) if (!t.roles.contains(role)) bad("roles."+role);
    for (const auto& [role,lanes] : t.roles) {
        if (std::find(roles.begin(),roles.end(),role) == roles.end()) bad("roles."+role);
        const auto rows = junctionLaneIssues(lanes,path+".roles."+role);
        issues.insert(issues.end(),rows.begin(),rows.end());
    }
    return issues;
}
std::vector<ValidationIssue> junctionRecipeIssues(const NodeJunction& r, const std::string& path) {
    std::vector<ValidationIssue> issues;
    const auto bad = [&](const std::string& at) { issues.push_back({"EDIT_NODE_INVALID",at}); };
    if (blank(r.templateId)) bad(path+".templateId");
    std::set<std::string> major;
    for (std::size_t i = 0; i < r.major.size(); ++i)
        if (blank(r.major[i]) || !major.insert(r.major[i]).second) bad(path+".major["+std::to_string(i)+"]");
    for (const auto& [id,leg] : r.legs) {
        const auto at = path+".legs."+id;
        if (blank(id)) bad(at);
        for (const auto* key : {"storage","taper"}) {
            const auto& values = std::string(key) == "storage" ? leg.storage : leg.taper;
            for (std::size_t i = 0; i < values.size(); ++i)
                if (values[i] && !positive(*values[i])) bad(at+"."+key+"["+std::to_string(i)+"]");
        }
        if (leg.lanes) for (auto issue : junctionLaneIssues(*leg.lanes,at+".lanes")) { issue.code = "EDIT_NODE_INVALID"; issues.push_back(issue); }
    }
    if (r.signal) {
        const auto at = path+".signal";
        if (r.signal->cycle && !positive(*r.signal->cycle)) bad(at+".cycle");
        std::set<int> numbers;
        for (std::size_t i = 0; i < r.signal->phases.size(); ++i) {
            const auto& p = r.signal->phases[i]; const auto phase = at+".phases["+std::to_string(i)+"]";
            if (p.number < 1 || p.number > 8 || !numbers.insert(p.number).second) bad(phase+".number");
            if (p.green && !positive(*p.green)) bad(phase+".green");
            if (p.yellow && (!std::isfinite(*p.yellow) || *p.yellow < 0)) bad(phase+".yellow");
            if (p.allRed && (!std::isfinite(*p.allRed) || *p.allRed < 0)) bad(phase+".allRed");
        }
    }
    return issues;
}
}
