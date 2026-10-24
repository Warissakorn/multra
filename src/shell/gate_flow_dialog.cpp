#include "gate_flow_dialog.hpp"
#include "dialog_fit.hpp"
#include "editor_interaction.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <map>
#include <tuple>

namespace multra {
namespace {
void fill(QComboBox* combo, const std::vector<Choice>& choices) {
    for (const auto& [id, label] : choices) combo->addItem(label, QString::fromStdString(id));
}
QStringList labels(const std::vector<Choice>& choices) {
    QStringList out;
    for (const auto& c : choices) out << c.second;
    return out;
}
}
std::optional<std::vector<GateFlow>> editGateFlows(QWidget* parent, const std::vector<GateFlow>& current,
                                                   const std::vector<Choice>& origins, const std::vector<Choice>& destinations,
                                                   const std::vector<Choice>& compositions, const std::vector<Choice>& periods,
                                                   const std::vector<Choice>& modes, const DialogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorGateFlowDialog"); dialog.setWindowTitle(text("editorGateFlows"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout; layout->addLayout(form);
    auto* period = new QComboBox(&dialog); period->setObjectName("editorGateFlowPeriod"); fill(period, periods);
    auto* source = new QComboBox(&dialog); source->setObjectName("editorGateFlowTypes");
    source->addItem(text("editorOdComposition")); source->addItem(text("editorOdMode"));
    auto* types = new QComboBox(&dialog); types->setObjectName("editorGateFlowComposition"); fill(types, compositions);
    auto* mode = new QComboBox(&dialog); mode->setObjectName("editorGateFlowMode"); fill(mode, modes);
    form->addRow(text("editorOdStudyPeriod"), period); form->addRow(text("editorOdTypes"), source);
    form->addRow(text("editorOdComposition"), types); form->addRow(text("editorOdMode"), mode);
    // The first stored flow chooses what opens, so an edit starts where the last one left off.
    if (!current.empty()) {
        period->setCurrentIndex(std::max(0, period->findData(QString::fromStdString(current.front().periodId))));
        source->setCurrentIndex(current.front().modeId.empty() ? 0 : 1);
        types->setCurrentIndex(std::max(0, types->findData(QString::fromStdString(current.front().compositionId))));
        mode->setCurrentIndex(std::max(0, mode->findData(QString::fromStdString(current.front().modeId))));
    }
    auto* help = new ReflowLabel(text("editorGateFlowHelp"), &dialog); help->setWordWrap(true); layout->addWidget(help);
    auto* grid = new QTableWidget(static_cast<int>(origins.size()), static_cast<int>(destinations.size()), &dialog);
    grid->setObjectName("editorGateFlowGrid"); grid->setProperty("editorCustomRows", true);
    grid->setVerticalHeaderLabels(labels(origins)); grid->setHorizontalHeaderLabels(labels(destinations));
    layout->addWidget(grid, 1);
    using Key = std::tuple<std::string, std::string, std::string, std::string, std::string>; // period, composition, mode, from, to
    std::map<Key, double> flows;
    for (const auto& f : current) flows[{f.periodId, f.compositionId, f.modeId, f.from, f.to}] = f.vehiclesPerHour;
    const auto key = [&](std::size_t r, std::size_t c) {
        const bool byMode = source->currentIndex() == 1;
        return Key{period->currentData().toString().toStdString(), byMode ? std::string{} : types->currentData().toString().toStdString(),
                   byMode ? mode->currentData().toString().toStdString() : std::string{}, origins[r].first, destinations[c].first};
    };
    const auto show = [&] {
        const bool byMode = source->currentIndex() == 1;
        types->setEnabled(!byMode); mode->setEnabled(byMode);
        for (std::size_t r = 0; r < origins.size(); ++r)
            for (std::size_t c = 0; c < destinations.size(); ++c) {
                auto* field = new QDoubleSpinBox(grid); field->setRange(0, 1e6); field->setDecimals(1);
                field->setButtonSymbols(QAbstractSpinBox::NoButtons); // a grid cell is typed into
                field->setFont(editorDesign::numericFont()); field->setProperty("numeric", true);
                const auto at = key(r, c);
                field->setValue(flows.contains(at) ? flows.at(at) : 0);
                QObject::connect(field, &QDoubleSpinBox::valueChanged, &dialog, [&flows, at](double v) { flows[at] = v; });
                grid->setCellWidget(static_cast<int>(r), static_cast<int>(c), field);
                grid->setColumnWidth(static_cast<int>(c), std::max(grid->columnWidth(static_cast<int>(c)), field->sizeHint().width()));
                grid->setRowHeight(static_cast<int>(r), std::max(grid->rowHeight(static_cast<int>(r)), field->sizeHint().height()));
            }
    };
    show();
    for (auto* combo : {period, source, types, mode})
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [&](int) { show(); });
    auto* error = new ReflowLabel(&dialog); error->setObjectName("editorGateFlowError"); error->setWordWrap(true); layout->addWidget(error);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    box->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    box->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    layout->addWidget(box);
    QObject::connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    std::optional<std::vector<GateFlow>> result;
    QObject::connect(box, &QDialogButtonBox::accepted, &dialog, [&] {
        if (period->currentIndex() < 0) { error->setText(text("STUDY_AREA_PERIOD_UNKNOWN")); return; }
        if (source->currentIndex() == 1 ? mode->currentIndex() < 0 : types->currentIndex() < 0) { error->setText(text("OD_COMPOSITION_MISSING")); return; }
        std::vector<GateFlow> out;
        for (const auto& [k, v] : flows)
            if (v > 0) out.push_back({std::get<0>(k), std::get<3>(k), std::get<4>(k), std::get<1>(k), std::get<2>(k), v});
        result = std::move(out); dialog.accept();
    });
    dialog.resize(760, 460);
    fitToScreen(dialog); // D318: a large area's grid never pushes the buttons off screen
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
}
