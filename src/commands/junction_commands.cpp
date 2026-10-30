#include "junction_commands.hpp"
#include <algorithm>
#include <stdexcept>
namespace multra {
namespace {
Node& nodeOf(ProjectDocument& d, const std::string& id) {
    const auto node = std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& n) { return n.id == id; });
    if (node == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *node;
}
bool sameStructure(const NodeJunction& a,const NodeJunction& b) {
    if(a.templateId!=b.templateId||a.major!=b.major)return false;
    const auto lanes=[](const NodeJunction& r){std::map<std::string,std::vector<JunctionLane>> out;
        for(const auto& [id,leg]:r.legs)if(leg.lanes)out[id]=*leg.lanes;
        return out;};
    return lanes(a)==lanes(b);
}
}
void setNodeJunction(ProjectDocument& d, const std::string& id, std::optional<NodeJunction> value) {
    auto& node = nodeOf(d,id);
    if (value && value->major.empty() && node.coarse) value->major = node.coarse->major;
    auto previous=node.junction;
    if(previous&&previous->major.empty()&&node.coarse)previous->major=node.coarse->major;
    if(!previous||!value||!sameStructure(*previous,*value))node.generated=false;
    node.junction = std::move(value); // B36: site edits keep the current flag, never restore it.
}
void chooseNodeJunctionTemplate(ProjectDocument& d,const std::string& id,const std::optional<std::string>& type) {
    if(!type){setNodeJunction(d,id,std::nullopt);return;}
    auto recipe=nodeOf(d,id).junction.value_or(NodeJunction{});recipe.templateId=*type;
    setNodeJunction(d,id,std::move(recipe));
}
void setNodeApproachDirections(ProjectDocument& d, const std::string& id, ApproachDirections directions) {
    auto& node = nodeOf(d,id);
    node.approachDirections = std::move(directions); node.generated = false;
}
void putJunctionTemplateCatalog(ProjectDocument& d, std::vector<JunctionTemplate> templates) {
    if (!d.definition) d.definition.emplace();
    d.definition->externalJunctionTemplates = false;
    d.definition->junctionTemplates = std::move(templates);
}
void renameJunctionStreetSegment(NodeJunction& r, const std::string& from, const std::string& to) {
    if (from == to) return;
    for (auto& id : r.major) if (id == from) id = to;
    auto entry = r.legs.extract(from);
    if (entry.empty()) return;
    // Refuse a collision rather than discard entered site values; History restores the candidate.
    if (r.legs.contains(to) && r.legs.at(to) != entry.mapped()) throw std::invalid_argument("EDIT_NODE_INVALID");
    entry.key() = to; r.legs.insert(std::move(entry));
}
}
