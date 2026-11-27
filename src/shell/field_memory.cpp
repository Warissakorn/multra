#include "field_memory.hpp"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHash>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QStringList>
#include <QVariant>
#include <vector>
namespace multra {
namespace {
QHash<QString, QVariant>& memory() { static QHash<QString, QVariant> values; return values; }
std::vector<std::function<void()>>& clears() { static std::vector<std::function<void()>> list; return list; }
}
void rememberField(QCheckBox* on, QAbstractSpinBox* field, const QString& key) {
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(field)) {
        if (on->isChecked()) memory()[key] = spin->value();
        else if (memory().contains(key)) spin->setValue(memory()[key].toDouble());
        // A kept value outside the field's range was clamped by a narrower range (a shorter Link), not
        // edited: keep it while the field sits on the bound that clamped it, so `refitField` can bring it back when the range allows it again (D683).
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, spin, [spin, key](double v) {
            const auto kept = memory().find(key);
            if (kept != memory().end() && (v == spin->minimum() || v == spin->maximum()) &&
                (kept->toDouble() < spin->minimum() || kept->toDouble() > spin->maximum())) return;
            memory()[key] = v;
        });
    } else if (auto* whole = qobject_cast<QSpinBox*>(field)) {
        if (on->isChecked()) memory()[key] = whole->value();
        else if (memory().contains(key)) whole->setValue(memory()[key].toInt());
        QObject::connect(whole, &QSpinBox::valueChanged, whole, [whole, key](int v) {
            const auto kept = memory().find(key);
            if (kept != memory().end() && (v == whole->minimum() || v == whole->maximum()) &&
                (kept->toInt() < whole->minimum() || kept->toInt() > whole->maximum())) return;
            memory()[key] = v;
        });
    }
}
void refitField(QAbstractSpinBox* field, const QString& key) {
    const auto kept = memory().find(key);
    if (kept == memory().end()) return;
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(field)) {
        if (const double v = kept->toDouble(); v >= spin->minimum() && v <= spin->maximum()) spin->setValue(v);
    } else if (auto* whole = qobject_cast<QSpinBox*>(field)) {
        if (const int v = kept->toInt(); v >= whole->minimum() && v <= whole->maximum()) whole->setValue(v);
    }
}
void rememberField(QCheckBox* on, QLineEdit* field, const QString& key) { rememberField(on->isChecked(), field, key); }
void rememberField(bool used, QLineEdit* field, const QString& key) {
    if (used) memory()[key] = field->text();
    else if (memory().contains(key)) field->setText(memory()[key].toString());
    QObject::connect(field, &QLineEdit::textEdited, field, [key](const QString& v) { memory()[key] = v; });
}
namespace {
QVariant cells(const QTableWidget* t) {
    QVariantList rows;
    for (int r = 0; r < t->rowCount(); ++r) {
        QStringList row;
        for (int c = 0; c < t->columnCount(); ++c) row << (t->item(r, c) ? t->item(r, c)->text() : QString());
        rows << row;
    }
    return rows;
}
}
void rememberTable(QCheckBox* on, QTableWidget* table, const QString& key) { rememberTable(on->isChecked(), table, key); }
void rememberTable(bool used, QTableWidget* table, const QString& key) {
    if (used) memory()[key] = cells(table);
    else if (memory().contains(key)) {
        const auto rows = memory()[key].toList();
        table->setRowCount(0);
        for (const auto& row : rows) {
            const int r = table->rowCount(); table->insertRow(r); const auto values = row.toStringList();
            for (int c = 0; c < table->columnCount() && c < values.size(); ++c) table->setItem(r, c, new QTableWidgetItem(values[c]));
        }
    }
    const auto keep = [table, key] { memory()[key] = cells(table); };
    QObject::connect(table->model(), &QAbstractItemModel::dataChanged, table, keep);
    QObject::connect(table->model(), &QAbstractItemModel::rowsInserted, table, keep);
    QObject::connect(table->model(), &QAbstractItemModel::rowsRemoved, table, keep);
}
void onClearFieldMemory(std::function<void()> clear) { clears().push_back(std::move(clear)); }
void clearFieldMemory() { memory().clear(); for (const auto& clear : clears()) clear(); }
}
