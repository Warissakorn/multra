#pragma once
#include "../project/object_csv.hpp"
#include <QStringList>
#include <functional>
class QWidget;
namespace multra {
void writeObjectTableCsv(const QString& file,const ProjectDocument&,ObjectTable,const QStringList& protectedFiles={});
// Returns false on cancellation. Caller presents translated errors through its normal surface.
bool exportObjectTableDialog(QWidget*,const ProjectDocument&,ObjectTable,
    const std::function<QString(const char*)>& text,const QStringList& protectedFiles={});
}
