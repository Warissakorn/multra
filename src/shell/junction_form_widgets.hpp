#pragma once
#include "demand_catalog_dialog.hpp"
#include "../model/network/junction_templates.hpp"
#include "../model/network/network.hpp"
class QTableWidget;
class QLineEdit;
namespace multra {
struct JunctionLaneWidgets {QWidget* body{};QTableWidget* table{};};
JunctionLaneWidgets junctionLaneWidgets(QWidget*,const std::vector<JunctionLane>&,DrivingSide,
    const QString& objectName,const CatalogText&,const std::function<void()>& changed);
std::vector<JunctionLane> junctionLaneValues(const JunctionLaneWidgets&);
QLineEdit* junctionNumber(QWidget*,const std::optional<double>&,const char* objectName);
std::optional<double> junctionNumberValue(const QLineEdit*,bool zeroAllowed=false);
QString junctionIssueText(const std::vector<ValidationIssue>&,const CatalogText&);
}
