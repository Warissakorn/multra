#pragma once
#include <QString>
#include <QStringList>
#include <functional>
class QMenu;
namespace multra {
// File → *Open recent* (D504). The list is the person's, not a project's: `recent-files.json` in the
// application's local data folder, newest first, at most kRecentFileLimit absolute paths. A missing or
// malformed file is an empty list, never an error.
inline constexpr int kRecentFileLimit = 8;
QString recentFilesPath();
QStringList loadRecentFiles();
// Moves `file` to the top (as an absolute path), drops its earlier entry and the oldest past the limit.
void rememberRecentFile(const QString& file);
void clearRecentFiles();
// Whether the list holds a file; read from disk once, then kept in step by the three calls above.
bool hasRecentFiles();
// Replaces the menu's entries: one action per file (`editorRecent:<index>`, the file name as text, the
// path as tool tip; disabled when the file is gone), then *Clear list*; or one disabled line when empty.
// `none` and `clearText` are the translated captions.
void fillRecentMenu(QMenu& menu, const QStringList& files, const QString& none, const QString& clearText,
                    const std::function<void(const QString&)>& open, const std::function<void()>& clear);
}
