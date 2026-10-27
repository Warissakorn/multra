#pragma once
#include "../project/document.hpp"
#include <QJsonObject>
#include <QJsonArray>
namespace multra {
QJsonObject suggestConnector(const ProjectDocument&,const QJsonObject& params);
QJsonArray geometryValidationIssues(const std::vector<ValidationIssue>&,const Json* candidate=nullptr);
}
