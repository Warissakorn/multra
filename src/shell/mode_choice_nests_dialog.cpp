#include "mode_choice_nests_dialog.hpp"
#include "dialog_fit.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/mode_choice_codec.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <nlohmann/json.hpp>

namespace multra {
namespace {
QString qs(const Json& j) { return QString::fromStdString(j.get<std::string>()); }
}
std::optional<Json> editModeChoiceNests(QWidget* parent, const Json& set, const DialogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorModeChoiceNestsDialog"); dialog.setWindowTitle(text("editorModeChoiceNests"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    // The note names each purpose's alternatives, the ids a nest's members must use.
    QStringList purposes;
    for (const auto& p : set.at("purposes")) {
        QStringList ids;
        for (const auto& a : p.at("alternatives")) ids << qs(a.at("id"));
        purposes << qs(p.at("id")) + ": " + ids.join(", ");
    }
    auto* note = new QLabel(text("editorModeChoiceNestsNote") + "\n" + purposes.join("\n"), &dialog);
    note->setObjectName("editorModeChoiceNestsNote"); note->setWordWrap(true); layout->addWidget(note);
    auto* table = new QTableWidget(0, 4, &dialog); table->setObjectName("editorModeChoiceNestsTable");
    table->setHorizontalHeaderLabels({text("editorTripPurpose"), text("editorColumnId"), text("editorModeChoiceNestPhi"),
                                      text("editorModeChoiceNestMembers")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumSize(520, 180); layout->addWidget(table, 1);
    const auto addRow = [&](const QString& purpose, const QString& id, const QString& phi, const QString& members) {
        const int row = table->rowCount(); table->insertRow(row);
        auto* choice = new QComboBox(table);
        for (const auto& p : set.at("purposes")) choice->addItem(qs(p.at("id")), qs(p.at("id")));
        if (const int at = choice->findData(purpose); at >= 0) choice->setCurrentIndex(at);
        table->setCellWidget(row, 0, choice);
        table->setItem(row, 1, new QTableWidgetItem(id));
        table->setItem(row, 2, new QTableWidgetItem(phi));
        table->setItem(row, 3, new QTableWidgetItem(members));
    };
    for (const auto& p : set.at("purposes"))
        if (p.contains("nests"))
            for (const auto& n : p.at("nests")) {
                QStringList members;
                for (const auto& m : n.at("alternatives")) members << qs(m);
                addRow(qs(p.at("id")), qs(n.at("id")), QString::number(n.at("phi").get<double>(), 'g', 17), members.join(", "));
            }
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(text("editorModeChoiceNestAdd"), &dialog); add->setObjectName("editorModeChoiceNestAdd");
    auto* remove = new QPushButton(text("editorTripRateRemove"), &dialog); remove->setObjectName("editorModeChoiceNestRemove");
    buttons->addWidget(add); buttons->addWidget(remove); buttons->addStretch(); layout->addLayout(buttons);
    auto* error = new QLabel(&dialog); error->setObjectName("editorModeChoiceNestsError"); error->setWordWrap(true); layout->addWidget(error);
    QObject::connect(add, &QPushButton::clicked, &dialog, [&] {
        addRow(qs(set.at("purposes")[0].at("id")), "", "1", ""); table->selectRow(table->rowCount() - 1);
    });
    QObject::connect(remove, &QPushButton::clicked, &dialog, [&] { if (table->currentRow() >= 0) table->removeRow(table->currentRow()); });
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(box);
    box->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    box->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    Json result;
    QObject::connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(box, &QDialogButtonBox::accepted, &dialog, [&] {
        Json edited = set;
        for (auto& p : edited.at("purposes")) p.erase("nests");
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto cell = [&](int column) { return table->item(r, column) ? table->item(r, column)->text().trimmed() : QString(); };
            const auto purpose = static_cast<QComboBox*>(table->cellWidget(r, 0))->currentData().toString().toStdString();
            Json nest{{"id", cell(1).toStdString()}};
            // A φ that is not a number goes to the codec as text, so the refusal names its path.
            bool ok = false;
            const double phi = cell(2).toDouble(&ok);
            nest["phi"] = ok ? Json(phi) : Json(cell(2).toStdString());
            nest["alternatives"] = Json::array();
            for (const auto& m : cell(3).split(',', Qt::SkipEmptyParts)) nest["alternatives"].push_back(m.trimmed().toStdString());
            for (auto& p : edited.at("purposes"))
                if (p.at("id").get<std::string>() == purpose) p["nests"].push_back(std::move(nest));
        }
        try {
            result = modeChoiceSetJson(parseModeChoiceSet(edited, "set", false));
        } catch (const ValidationError& e) {
            error->setText(text(e.issues.front().code.c_str()) + " (" + QString::fromStdString(e.issues.front().path) + ")");
            return;
        }
        dialog.accept();
    });
    dialog.resize(600, 360); fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
}
