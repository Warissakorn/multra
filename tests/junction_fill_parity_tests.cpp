#include "junction_fill_fixture.hpp"
#include "reference/j3b/api.hpp"
#include "../src/commands/network_commands.hpp"
using namespace multra;
using namespace junctionTest;
namespace {
std::vector<FillOffer> kind(std::vector<FillOffer> offers,FillKind wanted) {
    std::erase_if(offers,[&](const auto& x){return x.kind!=wanted;});return offers;
}
void noJunctionFields(const ProjectDocument& d) {
    for(const auto& n:d.network.nodes)CHECK(!n.junction&&n.approachDirections.empty());
    for(const auto& l:d.network.links)CHECK(!l.generated);
    CHECK(documentJson(d).at("schemaVersion")<=60);
}
}
TEST(junctionfill,jt7_legacy_f3_f4_documents_match_pre_j3b_source) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool signal:{false,true}) {
        auto d=cross(side,false);const auto major=node(d).junction->major;
        node(d).junction.reset();std::vector<std::string> links;
        for(const auto& l:d.network.links)links.push_back(l.id);
        deleteObjects(d,links);
        node(d).coarse=NodeCoarse{std::nullopt,major,{{"ew","ns"}}};
        CHECK(d.network.links.empty()&&!nodeHasDetail(d.network,node(d)));
        noJunctionFields(d);
        History old,now;old.reset(d);now.reset(d);
        const auto c=catalogs(d);
        const auto f3=kind(fillOffers(d,c),FillKind::streetSegmentLinks);
        const auto legacyF3=kind(legacy_fillOffers(d,c),FillKind::streetSegmentLinks);
        CHECK(f3.size()==4&&f3==legacyF3);
        CHECK(old.execute("editorFillDefaults",[&](auto& x){legacy_applyFills(x,legacyF3,c);}));
        CHECK(now.execute("editorFillDefaults",[&](auto& x){applyFills(x,f3,c);},true));
        noJunctionFields(old.document());noJunctionFields(now.document());
        CHECK(documentJson(now.document())==documentJson(old.document()));
        const auto f3bytes=documentJson(now.document());
        now.undo();CHECK(documentJson(now.document())==documentJson(d));now.redo();CHECK(documentJson(now.document())==f3bytes);
        if(signal) {
            const auto signalise=[&](ProjectDocument& x) {
                CoarseSignal s{90,{{40,{}},{40,{}}}};
                for(const auto& a:x.network.streetSegments)for(const auto& b:x.network.streetSegments) {
                    const auto pair=std::pair{a.id,b.id};
                    if(a.id==b.id||pair==std::pair{std::string("ew"),std::string("ns")})continue;
                    const bool main=std::find(major.begin(),major.end(),a.id)!=major.end();
                    s.stages[main?0:1].turns.push_back(pair);
                }
                node(x).coarse->control=CoarseControl::signalised;node(x).coarse->signal=s;
            };
            CHECK(old.execute("signal",signalise));CHECK(now.execute("signal",signalise));
        }
        CHECK(node(now.document()).coarse->bannedTurns.size()==1);
        const auto f4=kind(fillOffers(now.document(),c),FillKind::nodeDetail);
        const auto legacyF4=kind(legacy_fillOffers(old.document(),c),FillKind::nodeDetail);
        CHECK(f4.size()==1&&f4==legacyF4&&f4[0].refusal.empty());
        CHECK(old.execute("editorFillDefaults",[&](auto& x){legacy_applyFills(x,legacyF4,c);}));
        CHECK(now.execute("editorFillDefaults",[&](auto& x){applyFills(x,f4,c);},true));
        CHECK(nodeHasDetail(now.document().network,node(now.document())));
        noJunctionFields(old.document());noJunctionFields(now.document());
        CHECK(documentJson(now.document())==documentJson(old.document()));
        const auto after=documentJson(now.document());
        const auto oldReapply=old.execute("template",[&](auto& x){for(const auto& r:x.network.streetSegments)legacy_applyTemplate(x,r.id,c.templates);});
        const auto newReapply=now.execute("template",[&](auto& x){for(const auto& r:x.network.streetSegments)applyTemplate(x,r.id,c.templates);},true);
        CHECK(oldReapply==newReapply&&documentJson(now.document())==documentJson(old.document())&&documentJson(now.document())==after);
    }
}
