#include "automation_geometry.hpp"
#include "automation_error.hpp"
#include "../commands/connector_commands.hpp"
#include "../model/network/connector_attachment.hpp"
#include "../project/json.hpp"
#include <nlohmann/json.hpp>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
void parameter(const QString& path,const QString& expected) {
    throw CommandError("INVALID_PARAMS","Invalid connector parameter",{{"path",path},{"expected",expected}});
}
void known(const QJsonObject& object,const QStringList& keys,const QString& path) {
    for(auto it=object.begin();it!=object.end();++it)if(!keys.contains(it.key()))parameter(path+"."+it.key(),"a supported field");
}
LaneReference reference(const Network& network,const QJsonObject& params,const char* side) {
    const QString path=QString("params.")+side;
    if(!params.value(side).isObject())parameter(path,"object with linkId, laneId and optional station");
    const auto object=params.value(side).toObject();known(object,{"linkId","laneId","station"},path);
    for(const auto* key:{"linkId","laneId"})
        if(!object.value(key).isString() || object.value(key).toString().isEmpty())parameter(path+"."+key,"nonempty ID string");
    LaneReference result{object.value("linkId").toString().toStdString(),object.value("laneId").toString().toStdString()};
    const auto link=std::find_if(network.links.begin(),network.links.end(),[&](const auto& l){return l.id==result.linkId;});
    if(link==network.links.end())parameter(path+".linkId","an existing Link ID");
    if(std::none_of(link->lanes.begin(),link->lanes.end(),[&](const auto& l){return l.id==result.laneId;}))
        parameter(path+".laneId","a lane belonging to the specified Link");
    if(object.contains("station")) {
        const auto value=object.value("station");const auto station=value.toDouble();const auto length=polylineLength(link->geometry);
        if(!value.isDouble() || !std::isfinite(station) || station<0 || station>length)
            parameter(path+".station",QString("finite reference-polyline metres in [0,%1]").arg(length,0,'g',17));
        result.station=station;
    }
    return result;
}
QJsonObject point(Point p) {return {{"x",p.x},{"y",p.y}};}
QJsonObject refJson(const LaneReference& ref) {
    QJsonObject result{{"linkId",QString::fromStdString(ref.linkId)},{"laneId",QString::fromStdString(ref.laneId)}};
    if(ref.station)result.insert("station",*ref.station);return result;
}
}
QJsonObject suggestConnector(const ProjectDocument& document,const QJsonObject& params) {
    known(params,{"from","to","fromLaneCount","toLaneCount"},"params");
    const auto from=reference(document.network,params,"from"),to=reference(document.network,params,"to");
    const auto count=[&](const char* key,const LaneReference& ref) {
        if(!params.contains(key))return 1;
        const auto value=params.value(key);const auto n=value.toDouble();
        const int limit=std::min(12,lanesFromReference(document.network,ref));
        if(!value.isDouble() || !std::isfinite(n) || std::floor(n)!=n || n<1 || n>limit)
            parameter(QString("params.")+key,QString("integer in [1,%1] for the selected starting lane").arg(limit));
        return static_cast<int>(n);
    };
    const auto fromCount=count("fromLaneCount",from),toCount=count("toLaneCount",to);
    constexpr std::uint64_t exactJsonInteger=9007199254740991ULL;
    if(document.nextId>=exactJsonInteger)throw CommandError("SUGGESTION_ID_LIMIT","nextId is outside exact JSON proposal range");
    auto candidate=document;
    std::string id;
    try {id=addConnectorRange(candidate,from,to,fromCount,toCount);}
    catch(const std::exception& e){throw CommandError("CONNECTOR_SUGGEST_FAILED",e.what());}
    validateDocument(candidate);
    if(candidate.nextId>exactJsonInteger)throw CommandError("SUGGESTION_ID_LIMIT","Allocated nextId is outside exact JSON proposal range");
    const auto& c=candidate.network.connectors.back();
    QJsonArray geometry;for(const auto& p:c.geometry)geometry.append(point(p));
    QJsonObject connector{{"id",QString::fromStdString(id)},{"from",refJson(c.from)},{"to",refJson(c.to)},
        {"geometry",geometry},{"fromLaneCount",c.fromLaneCount},{"toLaneCount",c.toLaneCount},
        {"level",c.level},{"displayType",QString::fromStdString(c.displayType)}};
    // JSON numbers use the same representation as document.get/patch for nextId.
    const QJsonArray patch{QJsonObject{{"op","test"},{"path","/nextId"},{"value",static_cast<qint64>(document.nextId)}},
        QJsonObject{{"op","replace"},{"path","/nextId"},{"value",static_cast<qint64>(candidate.nextId)}},
        QJsonObject{{"op","add"},{"path","/network/connectors/-"},{"value",connector}}};
    return {{"connector",connector},{"patch",patch},{"rangeAdjusted",c.fromLaneCount!=fromCount || c.toLaneCount!=toCount},
        {"requestedCounts",QJsonObject{{"from",fromCount},{"to",toCount}}},
        {"actualCounts",QJsonObject{{"from",c.fromLaneCount},{"to",c.toLaneCount}}}};
}
QJsonArray geometryValidationIssues(const std::vector<ValidationIssue>& issues,const Json* candidate) {
    std::optional<Network> network;
    if(candidate && std::any_of(issues.begin(),issues.end(),[](const auto& i){return i.code=="DISCONNECTED_GEOMETRY";}))
        try {network=parseNetwork(candidate->at("network"),candidate->at("schemaVersion").get<int>());}catch(const std::exception&) {}
    QJsonArray rows;
    static const QRegularExpression endpointPattern("^connectors\\[([0-9]+)\\]\\.(from|to)$");
    for(const auto& issue:issues) {
        QJsonObject row{{"code",QString::fromStdString(issue.code)},{"path",QString::fromStdString(issue.path)}};
        const auto match=endpointPattern.match(QString::fromStdString(issue.path));
        if(network && issue.code=="DISCONNECTED_GEOMETRY" && match.hasMatch())try {
            bool ok{};const auto index=match.captured(1).toULongLong(&ok);
            if(ok && index<network->connectors.size()) {
                const auto& c=network->connectors[index];const bool outgoing=match.captured(2)=="from";
                if(!c.geometry.empty()) {
                    const auto m=measureConnectorAttachment(*network,outgoing?c.from:c.to,outgoing,outgoing?c.geometry.front():c.geometry.back());
                    row.insert("objectId",QString::fromStdString(c.id));row.insert("distance",m.distance);
                    row.insert("tolerance",connectorAttachmentTolerance);row.insert("expected",point(m.expected));row.insert("actual",point(m.actual));
                    row.insert("geometryPointer",QString("/network/connectors/%1/geometry/%2").arg(index).arg(outgoing?0:c.geometry.size()-1));
                }
            }
        } catch(const std::exception&) {} // Keep original issue if an independent invalid field prevents measurement.
        rows.append(row);
    }
    return rows;
}
}
