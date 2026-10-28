#include "street_segment_zone_dialog.hpp"
#include "dialog_fit.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <cmath>

namespace multra {
std::optional<ZoneConnector> editStreetSegmentZoneConnector(QWidget* parent, const ZoneConnector& value,
                                                  const std::vector<Choice>& roads, const DialogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorStreetSegmentZoneConnectorDialog");
    dialog.setWindowTitle(text(value.id.empty() ? "editorAddStreetSegmentZoneConnector" : "editorEditZoneConnector"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* target = new QComboBox(&dialog); target->setObjectName("editorZoneStreetSegmentTarget");
    for (const auto& [id, name] : roads) target->addItem(name, QString::fromStdString(id));
    const int selected = target->findData(QString::fromStdString(value.streetSegmentId));
    if (selected >= 0) target->setCurrentIndex(selected);
    auto* direction = new QComboBox(&dialog); direction->setObjectName("editorZoneStreetSegmentDirection");
    direction->addItem(text("editorZoneOrigin")); direction->addItem(text("editorZoneDestination"));
    direction->setCurrentIndex(value.direction == ZoneDirection::origin ? 0 : 1);
    auto* travel = new QComboBox(&dialog); travel->setObjectName("editorZoneStreetSegmentTravel");
    travel->addItem(text("editorZoneStreetSegmentForward")); travel->addItem(text("editorZoneStreetSegmentBackward"));
    travel->setCurrentIndex(value.streetSegmentForward ? 0 : 1);
    auto* share = new QLineEdit(QString::number(value.share, 'g', 17), &dialog); share->setObjectName("editorZoneStreetSegmentShare");
    form->addRow(text("editorStreetSegmentTable"), target); form->addRow(text("editorZoneDirection"), direction);
    form->addRow(text("editorZoneStreetSegmentTravel"), travel); form->addRow(text("editorZoneConnectorShare"), share);
    auto* hint = new QLabel(text("editorZoneStreetSegmentLevels"), &dialog); hint->setWordWrap(true); form->addRow(hint);
    auto* error = new QLabel(&dialog); error->setObjectName("editorZoneStreetSegmentError"); error->setWordWrap(true); form->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel")); form->addRow(buttons);
    ZoneConnector result = value;
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        bool ok = false; const double weight = share->text().toDouble(&ok);
        if (!ok || !std::isfinite(weight) || weight <= 0) { error->setText(text("ZONE_CONNECTOR_SHARE_INVALID")); return; }
        if (target->currentIndex() < 0) { error->setText(text("ZONE_CONNECTOR_UNKNOWN_REF")); return; }
        result.linkId.clear(); result.streetSegmentId = target->currentData().toString().toStdString();
        result.direction = direction->currentIndex() == 0 ? ZoneDirection::origin : ZoneDirection::destination;
        result.streetSegmentForward = travel->currentIndex() == 0; result.share = weight;
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
}
