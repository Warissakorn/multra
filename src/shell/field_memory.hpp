#pragma once
// F12 (owner, 2026-11-26): a value behind a tick box is kept while its box is unticked, so ticking it
// again brings back the number the author had, not 0. The memory lasts for this run of the program
// only and is never written to a file: an unticked key stays absent from the project (D178).
#include <QString>
#include <functional>
#include <map>
#include <string>
class QAbstractSpinBox;
class QCheckBox;
class QLineEdit;
class QTableWidget;
namespace multra {
// `key` names the field and what it belongs to, e.g. "behaviour/w99-lab/lineSetback". Call once the
// field holds its value from the document: a ticked box records it; an unticked one shows the value
// recorded under `key` before, if any. Every later edit of the field is recorded.
void rememberField(QCheckBox* on, QAbstractSpinBox* field, const QString& key);
void rememberField(QCheckBox* on, QLineEdit* field, const QString& key);
// After the field's range changed: shows the value kept under `key` when the new range holds it. A
// narrower range clamps the field without overwriting what is kept (D683), so a wider one brings it back.
void refitField(QAbstractSpinBox* field, const QString& key);
// The same for a field or table that a choice other than a tick box puts in use (`used`), e.g.
// CC1's Fixed value beside its Distribution curve. A table is kept cell by cell, rows included.
void rememberField(bool used, QLineEdit* field, const QString& key);
void rememberTable(bool used, QTableWidget* table, const QString& key);
void rememberTable(QCheckBox* on, QTableWidget* table, const QString& key);
// Forgets every kept value. Called when another project opens or a new one starts, since ids repeat
// between projects (a behaviour named the same in two files must not carry one's number into the other).
void clearFieldMemory();
void onClearFieldMemory(std::function<void()> clear);
// A kept whole object (a parking lot, a lane table) by key, of one type; cleared with the rest.
template <class T> std::map<std::string, T>& keptObjects() {
    static std::map<std::string, T> kept;
    static const bool registered = (onClearFieldMemory([] { kept.clear(); }), true);
    (void)registered;
    return kept;
}
}
