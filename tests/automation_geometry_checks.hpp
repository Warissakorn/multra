#pragma once
#include "../src/shell/automation_geometry.hpp"
#include "../src/shell/automation_error.hpp"
#include "../src/model/network/connector_attachment.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/evaluation_period.hpp"
inline void connectorAutomationChecks(const std::filesystem::path& data) {
    using namespace multra;
    const auto demand=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
    EditorWindow w(data);w.show();QTest::qWait(10);
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    const auto state=[&]{return w.automationRequest({{"command","status"}}).value("state").toObject();};
    const auto call=[&](const QString& command,const QJsonObject& params=QJsonObject{}) {
        const auto s=state();return w.automationRequest({{"command",command},{"params",params},
            {"documentId",s.value("documentId")},{"expectedRevision",s.value("revision")}});
    };
    demand(call("simulation.step",{{"steps",3}}).value("ok").toBool(),"Suggestion run fixture failed");
    w.canvas()->select("west");const auto selected=w.canvas()->selection();
    const auto before=w.history().document();
    const QJsonObject from{{"linkId","west"},{"laneId","west-1"}},to{{"linkId","north"},{"laneId","north-1"}};
    const QJsonObject params{{"from",from},{"to",to}};
    const auto proposal=w.automationRequest({{"command","connector.suggest"},{"params",params}});
    demand(proposal.value("ok").toBool(),"Unguarded connector suggestion failed");
    const auto result=proposal.value("result").toObject();
    auto expected=before;const auto id=addConnectorRange(expected,{"west","west-1"},{"north","north-1"},1,1);
    demand(result.value("connector").toObject().value("id")==QJsonValue(QString::fromStdString(id)),"Suggestion did not use existing allocator");
    demand(w.history().document()==before&&w.runState().tick==3&&w.canvas()->selection()==selected&&!w.history().canUndo(),"Suggestion changed live work");
    demand(w.automationRequest({{"command","connector.suggest"},{"params",params}}).value("result")==proposal.value("result"),"Suggestion reserved an ID or changed state");
    const auto s=proposal.value("state").toObject();
    auto apply=QJsonObject{{"command","document.patch"},{"documentId",s.value("documentId")},{"expectedRevision",s.value("revision")},
        {"params",QJsonObject{{"patch",result.value("patch")}}}};
    demand(w.automationRequest(apply).value("ok").toBool(),"Suggested patch could not be applied");
    demand(w.history().document().network.connectors.back()==expected.network.connectors.back()&&w.history().document().nextId==expected.nextId&&
        !w.runState().scenario&&w.history().canUndo(),"Applied proposal differs from UI command or retained stale run");
    demand(call("history.undo").value("ok").toBool()&&w.history().document().network==before.network&&w.history().document().nextId==before.nextId,"Suggestion Undo lost work");
    demand(call("history.redo").value("ok").toBool()&&w.history().document().network.connectors.back()==expected.network.connectors.back(),"Suggestion Redo differs");
    call("history.undo");
    const QJsonArray rename{QJsonObject{{"op","replace"},{"path","/network/links/0/name"},{"value","concurrent user edit"}}};
    demand(call("document.patch",{{"patch",rename}}).value("ok").toBool()&&w.history().revision()!=s.value("revision").toString().toULongLong(),"Stale revision fixture not established");
    demand(w.automationRequest(apply).value("error").toObject().value("code")==QJsonValue("STALE_DOCUMENT"),"Old proposal accepted after another edit");
    call("history.undo");
    auto bodyFrom=from,bodyTo=to;bodyFrom["station"]=10.0;bodyTo["station"]=10.0;
    const auto interior=call("connector.suggest",{{"from",bodyFrom},{"to",bodyTo}});
    demand(interior.value("ok").toBool()&&interior.value("result").toObject().value("connector").toObject().value("from").toObject().value("station")==QJsonValue(10.0),"Interior station suggestion failed");
    for(const auto& invalid:std::vector<std::pair<QJsonObject,QString>>{
        {{{"from",QJsonObject{{"linkId","missing"},{"laneId","west-1"}}},{"to",to}},"params.from.linkId"},
        {{{"from",QJsonObject{{"linkId","west"},{"laneId","north-1"}}},{"to",to}},"params.from.laneId"},
        {{{"from",QJsonObject{{"linkId","west"},{"laneId","west-1"},{"station",-1}}},{"to",to}},"params.from.station"},
        {{{"from",from},{"to",to},{"fromLaneCount",1.5}},"params.fromLaneCount"},
        {{{"from",from},{"to",to},{"fromLaneCount",2}},"params.fromLaneCount"}}) {
        const auto r=call("connector.suggest",invalid.first);const auto error=r.value("error").toObject();
        demand(error.value("path")==QJsonValue(invalid.second)&&error.contains("expected"),"Suggestion input error lacked field details");
    }
    demand(!call("connector.suggest",{{"from",from},{"to",QJsonObject{{"linkId","east"},{"laneId","east-1"}}}}).value("ok").toBool(),"Duplicate connection was suggested");
    call("simulation.step",{{"steps",3}});const auto stable=w.history().document();
    const auto& c=stable.network.connectors.front();
    const QJsonArray bad{QJsonObject{{"op","replace"},{"path","/network/connectors/0/geometry/0/x"},{"value",c.geometry.front().x+0.04}}};
    auto rejected=call("document.patch",{{"patch",bad}});
    demand(rejected.value("error").toObject().value("code")==QJsonValue("VALIDATION_FAILED"),"Bad endpoint was not rejected");
    bool measured=false;
    for(const auto& value:rejected.value("error").toObject().value("issues").toArray()) {
        const auto issue=value.toObject();if(issue.value("code")!=QJsonValue("DISCONNECTED_GEOMETRY"))continue;
        measured=true;demand(std::abs(issue.value("distance").toDouble()-0.04)<1e-9&&issue.value("tolerance").toDouble()==connectorAttachmentTolerance&&
            issue.value("geometryPointer")==QJsonValue("/network/connectors/0/geometry/0")&&issue.value("objectId")==QJsonValue(QString::fromStdString(c.id)),"Attachment measurement incorrect");
        const auto actual=issue.value("actual").toObject(),anchor=issue.value("expected").toObject();
        demand(std::abs(actual.value("x").toDouble()-anchor.value("x").toDouble()-0.04)<1e-9,"Diagnostic measured live geometry instead of rejected geometry");
    }
    demand(measured&&w.history().document()==stable&&w.runState().tick==3,"Invalid patch changed work or lacked measurements");
    const QJsonArray shifted{QJsonObject{{"op","add"},{"path","/network/connectors/0/from/station"},{"value",10.0}}};
    rejected=call("document.patch",{{"patch",shifted}});measured=false;
    auto candidateNetwork=stable.network;candidateNetwork.connectors.front().from.station=10;
    const auto anchor=laneAttachment(candidateNetwork,candidateNetwork.connectors.front().from,true);
    for(const auto& value:rejected.value("error").toObject().value("issues").toArray()) {
        const auto issue=value.toObject();if(issue.value("path")!=QJsonValue("connectors[0].from"))continue;
        measured=true;demand(std::abs(issue.value("expected").toObject().value("x").toDouble()-anchor.x)<1e-9&&
            std::abs(issue.value("expected").toObject().value("y").toDouble()-anchor.y)<1e-9,"Diagnostic used live reference instead of candidate reference");
    }
    demand(measured,"Changed station lacked attachment details");
    auto below=stable.network;const auto actualAnchor=laneAttachment(below,below.connectors.front().from,true);
    below.connectors.front().geometry.front()={actualAnchor.x+connectorAttachmentTolerance/2,actualAnchor.y};
    const auto issuesBelow=validateNetwork(below);
    demand(std::none_of(issuesBelow.begin(),issuesBelow.end(),[](const auto& i){return i.code=="DISCONNECTED_GEOMETRY";}),"Existing tolerance was tightened");
    ProjectDocument range=newProjectDocument();const auto a=addLink(range,{{0,0},{20,0}},4,3.5),b=addLink(range,{{40,0},{60,0}},1,3.5);
    const LaneReference rf{a,range.network.links[0].lanes[0].id},rt{b,range.network.links[1].lanes[0].id};
    const auto ranged=suggestConnector(range,{{"from",QJsonObject{{"linkId",QString::fromStdString(a)},{"laneId",QString::fromStdString(rf.laneId)}}},
        {"to",QJsonObject{{"linkId",QString::fromStdString(b)},{"laneId",QString::fromStdString(rt.laneId)}}},{"fromLaneCount",4},{"toLaneCount",1}});
    demand(ranged.value("rangeAdjusted").toBool()&&ranged.value("actualCounts").toObject().value("from")==QJsonValue(3),"UI range narrowing was hidden");
    const auto proposalRefs=QJsonObject{{"from",ranged.value("connector").toObject().value("from")},
        {"to",ranged.value("connector").toObject().value("to")}};
    range.network.drivingSide=DrivingSide::right;
    const auto right=suggestConnector(range,proposalRefs);
    const auto first=right.value("connector").toObject().value("geometry").toArray().at(0).toObject();
    const auto rightAnchor=laneAttachment(range.network,rf,true);
    demand(first.value("x").toDouble()==rightAnchor.x&&first.value("y").toDouble()==rightAnchor.y,"Suggestion ignored driving side");
    range.nextId=9007199254740991ULL;bool bounded=false;
    try{suggestConnector(range,proposalRefs);}catch(const CommandError& error){bounded=error.code=="SUGGESTION_ID_LIMIT";}
    demand(bounded,"Suggestion returned an inexact JSON allocator patch");
    call("file.new",{{"discardChanges",true}});
    std::cout<<"Connector proposal/apply/Undo, input diagnostics and candidate endpoint measurements passed\n";
}
