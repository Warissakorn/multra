#pragma once
#include "../project/document.hpp"
#include <QJsonObject>
#include <QJsonArray>
namespace multra {
ProjectDocument parseAutomationPatch(const ProjectDocument&,const QJsonArray&,Json* rejected=nullptr);
QJsonObject discoverDocument(const ProjectDocument&,const QJsonObject&);
QJsonObject validateAutomationDocument(const ProjectDocument&,const QJsonObject&);
}
