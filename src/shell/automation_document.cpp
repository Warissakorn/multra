#include "automation_document.hpp"
#include "automation_error.hpp"
#include "automation_geometry.hpp"
#include "../commands/history.hpp"
#include "../project/observed_schema.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <QJsonDocument>
namespace multra {
namespace {
Json native(const QJsonValue& value) {
    const auto bytes=QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return Json::parse(bytes.constData(),bytes.constData()+bytes.size()).at(0);
}
QJsonValue qt(const Json& value) {
    const auto bytes=Json::array({value}).dump();
    return QJsonDocument::fromJson(QByteArray::fromStdString(bytes)).array().at(0);
}
}
ProjectDocument parseAutomationPatch(const ProjectDocument& current,const QJsonArray& patch,Json* rejected) {
    auto imageless=current;imageless.background.pngBase64=std::make_shared<const std::string>();
    const auto original=documentJson(imageless);auto changed=original.patch(native(patch));
    if(rejected)*rejected=changed;
    if(!changed.is_object() || !changed.contains("schemaVersion") || changed.value("format",Json{})!=original.at("format") ||
       changed.value("revision",Json{})!=original.at("revision") || !changed.contains("background") ||
       !changed.at("background").is_object() || changed.at("background").value("pngBase64",Json{})!=Json(""))
        throw CommandError("PROTECTED_FIELD","Keep schemaVersion; format, revision and embedded image bytes cannot be changed");
    auto result=parseDocument(changed);result.revision=current.revision;result.background.pngBase64=current.background.pngBase64;return result;
}
QJsonObject discoverDocument(const ProjectDocument& current,const QJsonObject& params) {
    for(auto it=params.begin();it!=params.end();++it)if(it.key()!="pointer")
        throw CommandError("INVALID_PARAMS","Only optional pointer is accepted");
    if(params.contains("pointer") && !params.value("pointer").isString())throw CommandError("INVALID_PARAMS","pointer must be a JSON Pointer string");
    auto imageless=current;imageless.background.pngBase64=std::make_shared<const std::string>();
    const auto document=documentJson(imageless);const auto pointer=params.value("pointer").toString();
    Json value;
    try {value=pointer.isEmpty()?document:document.at(Json::json_pointer(pointer.toStdString()));}
    catch(const Json::exception&) {throw CommandError("INVALID_PARAMS","pointer must address the current document",{{"path","params.pointer"}});}
    const auto observed=observeJsonSchema(value);
    return {{"schema",qt(observed.schema)},{"coverage","observed-shape"},{"authoritative",false},{"truncated",observed.truncated},
        {"schemaVersion",qt(document.at("schemaVersion"))},{"latestSchemaVersion",kLatestSchema},{"validationCommand","document.validate"},
        {"protectedPatchFields",QJsonArray{"format","revision","background.pngBase64"}}};
}
QJsonObject validateAutomationDocument(const ProjectDocument& current,const QJsonObject& params) {
    for(auto it=params.begin();it!=params.end();++it)if(it.key()!="patch" && it.key()!="document")
        throw CommandError("INVALID_PARAMS","Use exactly one of patch or document");
    if(params.contains("patch")==params.contains("document"))throw CommandError("INVALID_PARAMS","Use exactly one of patch or document");
    Json rejected;
    try {
        if(params.contains("patch")) {
            if(!params.value("patch").isArray())throw CommandError("INVALID_PARAMS","patch must be an RFC6902 array");
            auto candidate=parseAutomationPatch(current,params.value("patch").toArray(),&rejected);
            History dry;dry.reset(current);const bool changed=dry.execute("editorAutomationChange",[&](auto& d){d=std::move(candidate);});
            return {{"valid",true},{"changed",changed},{"schemaVersion",qt(documentJson(dry.document()).at("schemaVersion"))}};
        }
        if(!params.value("document").isObject())throw CommandError("INVALID_PARAMS","document must be a canonical editor document object");
        rejected=native(params.value("document"));
        if(rejected.value("format",Json{})!=Json("TrafficSim") || !rejected.contains("schemaVersion"))
            throw CommandError("INVALID_PARAMS","document must contain format TrafficSim and schemaVersion");
        const auto candidate=parseDocument(rejected);validateDocument(candidate);
        return {{"valid",true},{"changed",candidate!=current},{"schemaVersion",qt(documentJson(candidate).at("schemaVersion"))}};
    } catch(const ValidationError& e) {
        throw CommandError("VALIDATION_FAILED",e.what(),{{"issues",geometryValidationIssues(e.issues,&rejected)}});
    }
}
}
