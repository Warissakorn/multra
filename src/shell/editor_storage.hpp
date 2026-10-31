#pragma once
#include "../project/document.hpp"
#include "../project/study_area_run.hpp"
#include <QString>
#include <QStringList>
#include <stdexcept>
namespace multra {
struct RecoveryError : std::runtime_error {using std::runtime_error::runtime_error;};
ProjectDocument readEditorDocument(const QString&);
inline constexpr std::size_t kEditorDocumentByteLimit=48*1024*1024;
std::string editorDocumentBytes(const Json&); // exact bounded Save representation, also used by J7 snapshots
struct EditorDocumentSnapshot {std::string bytes;bool saved{};};
EditorDocumentSnapshot captureEditorDocument(const ProjectDocument&,const QString& file);
void writeEditorDocument(const QString&,const Json&);
// Reject aliases of the active project/recovery before any export write.
void checkExportDestination(const QString&,const QStringList&,const char* errorCode);
// Atomic replacement of any file the shell writes; throws errorCode (a locale key) on failure.
void writeEditorBytes(const QString& file,const std::string& bytes,const char* errorCode);
// S5.5b (D332), editor_side_files.cpp: the side files beside `project` (none when it has none).
StudyAreaSources sideFilesBeside(const QString& project);
}
