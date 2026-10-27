#include "object_export.hpp"
#include "editor_storage.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <stdexcept>

namespace multra {
void writeObjectTableCsv(const QString& file,const ProjectDocument& d,ObjectTable table,const QStringList& protectedFiles) {
    const QFileInfo destination(file);
#ifdef Q_OS_WIN
    constexpr auto pathCase=Qt::CaseInsensitive;
#else
    constexpr auto pathCase=Qt::CaseSensitive;
#endif
    for(const auto& path:protectedFiles) {
        if(path.isEmpty())continue;
        const QFileInfo protectedFile(path);
        const bool sameName=QDir::cleanPath(destination.absoluteFilePath()).compare(
            QDir::cleanPath(protectedFile.absoluteFilePath()),pathCase)==0;
        const auto canonical=destination.canonicalFilePath();
        if(sameName || (!canonical.isEmpty() && canonical.compare(protectedFile.canonicalFilePath(),pathCase)==0))
            throw std::runtime_error("EDIT_OBJECT_CSV_PROJECT");
    }
    writeEditorBytes(file,objectTableCsv(d,table),"EDIT_OBJECT_CSV_WRITE");
}
bool exportObjectTableDialog(QWidget* parent,const ProjectDocument& d,ObjectTable table,
    const std::function<QString(const char*)>& text,const QStringList& protectedFiles) {
    QFileDialog dialog(parent,text("editorExportObjects"),QString::fromLatin1(objectTableName(table))+".csv",text("editorCsvFilter"));
    dialog.setObjectName("editorObjectCsvDialog");dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("csv");
    if(dialog.exec()!=QDialog::Accepted || dialog.selectedFiles().isEmpty())return false;
    writeObjectTableCsv(dialog.selectedFiles().first(),d,table,protectedFiles);return true;
}
}
