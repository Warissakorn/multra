#include "utdf_export_dialog.hpp"
#include "dialog_fit.hpp"
#include "editor_storage.hpp"
#include "../geo/projection.hpp"
#include "../project/document.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <memory>
#include <stdexcept>

namespace multra {
UtdfOptions utdfOptionsFor(const ProjectDocument& d, UtdfMode mode, bool unsignalised) {
    UtdfOptions options{mode, unsignalised, {}};
    if (d.map) {
        const auto projection = std::make_shared<GeoProjection>(d.map->reference);
        options.grid = [projection](double x, double y) {
            const auto g = projection->worldToGrid({x, y});
            return std::array<double, 2>{g.x, g.y};
        };
    }
    return options;
}
void writeUtdf(const QString& file, const ProjectDocument& d, const UtdfOptions& options, const QStringList& protectedFiles) {
    checkExportDestination(file, protectedFiles, "EDIT_UTDF_PROJECT");
    const auto result = utdfDocument(d, options);
    if (result.csv.empty()) throw std::invalid_argument("UTDF_NOTHING_TO_EXPORT");
    writeEditorBytes(file, result.csv, "EDIT_UTDF_WRITE");
}
bool exportUtdfDialog(QWidget* parent, const ProjectDocument& d, const std::function<QString(const char*)>& text,
                      const QStringList& protectedFiles) {
    QDialog dialog(parent); dialog.setObjectName("editorUtdfDialog"); dialog.setWindowTitle(text("editorExportUtdf"));
    auto* body = scrollingBody(dialog); auto* layout = new QVBoxLayout(body);
    auto* form = new QFormLayout; layout->addLayout(form);
    auto* mode = new QComboBox(body); mode->setObjectName("editorUtdfMode");
    mode->addItem(text("editorUtdfGeometric")); mode->addItem(text("editorUtdfMirrored"));
    form->addRow(text("editorUtdfModeLabel"), mode);
    auto* unsignalised = new QCheckBox(text("editorUtdfUnsignalised"), body); unsignalised->setObjectName("editorUtdfUnsignalised");
    form->addRow(unsignalised);
    auto* table = new QTableWidget(0, 3, body); table->setObjectName("editorUtdfReport");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->verticalHeader()->hide();
    table->setHorizontalHeaderLabels({text("editorUtdfColumnNote"), text("editorUtdfColumnNode"), text("editorUtdfColumnDetail")});
    table->horizontalHeader()->setStretchLastSection(true); layout->addWidget(table);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); addFooter(dialog, buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorUtdfExportButton"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto options = [&] {
        return utdfOptionsFor(d, mode->currentIndex() == 1 ? UtdfMode::mirrored : UtdfMode::geometric, unsignalised->isChecked());
    };
    const auto refresh = [&] {
        const auto result = utdfDocument(d, options());
        table->setRowCount(static_cast<int>(result.report.size()));
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto& n = result.report[static_cast<std::size_t>(r)];
            table->setItem(r, 0, new QTableWidgetItem(text(n.code.c_str())));
            table->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(n.node)));
            table->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(n.text)));
        }
        table->resizeColumnToContents(0); table->resizeColumnToContents(1);
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!result.csv.empty());
    };
    QObject::connect(mode, &QComboBox::currentIndexChanged, &dialog, [&] { refresh(); });
    QObject::connect(unsignalised, &QCheckBox::toggled, &dialog, [&] { refresh(); });
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    refresh(); dialog.resize(820, 480);
    if (dialog.exec() != QDialog::Accepted) return false;
    const auto chosen = options();
    QFileDialog files(parent, text("editorExportUtdf"), QStringLiteral("network.csv"), text("editorCsvFilter"));
    files.setObjectName("editorUtdfFileDialog"); files.setAcceptMode(QFileDialog::AcceptSave); files.setDefaultSuffix("csv");
    if (files.exec() != QDialog::Accepted || files.selectedFiles().isEmpty()) return false;
    writeUtdf(files.selectedFiles().first(), d, chosen, protectedFiles);
    return true;
}
}
