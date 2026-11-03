#pragma once
// UTDF U2b (UTDF_EXPORT §7, D442, D443): File -> Export -> UTDF... The dialog shows the report for the
// chosen mode and NEMA option before anything is written; the file is replaced atomically.
#include "../project/utdf_export.hpp"
#include <QStringList>
#include <functional>
class QWidget;
namespace multra {
// The writer's options for this document: UTM X, Y when it has a map reference (PROJ), else world metres.
UtdfOptions utdfOptionsFor(const ProjectDocument&, UtdfMode, bool unsignalisedOnNemaFailure);
// Refuses the open project and its recovery file (EDIT_UTDF_PROJECT), and a document with nothing to
// write (UTDF_NOTHING_TO_EXPORT); a failed write is EDIT_UTDF_WRITE and leaves the destination intact.
void writeUtdf(const QString& file, const ProjectDocument&, const UtdfOptions&, const QStringList& protectedFiles = {});
// Returns false on cancellation. Caller presents translated errors through its normal surface.
bool exportUtdfDialog(QWidget*, const ProjectDocument&, const std::function<QString(const char*)>& text,
                      const QStringList& protectedFiles = {});
}
