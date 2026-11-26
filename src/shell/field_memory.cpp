#include "field_memory.hpp"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHash>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QStringList>
#include <QVariant>
namespace multra {
namespace {
QHash<QString, QVariant>& memory() { static QHash<QString, QVariant> values; return values; }
}
void rememberField(QCheckBox* on, QAbstractSpinBox* field, const QString& key) {
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(field)) {
        if (on->isChecked()) memory()[key] = spin->value();
        else if (memory().contains(key)) spin->setValue(memory()[key].toDouble());
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, spin, [key](double v) { memory()[key] = v; });
    } else if (auto* whole = qobject_cast<QSpinBox*>(field)) {
        if (on->isChecked()) memory()[key] = whole->value();
        else if (memory().contains(key)) whole->setValue(memory()[key].toInt());
        QObject::connect(whole, &QSpinBox::valueChanged, whole, [key](int v) { memory()[key] = v; });
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
}
