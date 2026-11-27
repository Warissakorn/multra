#pragma once
// YIELD_IMPATIENCE §3 (D618): the place's gap reduction in a dialog -- a checkbox and Aimsun's three
// numbers, named as the file spells them. Unchecked is no reduction (the keys absent).
#include "../core/yield_types.hpp"
#include "field_memory.hpp"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <optional>

namespace multra {
struct GapReductionFields {
    QCheckBox* on{}; QDoubleSpinBox *finalGap{}, *start{}, *end{};
    std::optional<GapReduction> value() const {
        if (!on->isChecked()) return std::nullopt;
        return GapReduction{finalGap->value(), start->value(), end->value()};
    }
};
// `suffix` keeps object names unique per marker ("" for an area).
inline GapReductionFields gapReductionFields(QWidget* parent, QFormLayout* form, const std::optional<GapReduction>& r,
                                             const QString& label, const QString& suffix, const QString& scope) {
    GapReductionFields f{new QCheckBox(label, parent), new QDoubleSpinBox(parent), new QDoubleSpinBox(parent), new QDoubleSpinBox(parent)};
    f.on->setObjectName("editorGapReduction" + suffix); f.on->setChecked(r.has_value());
    const auto spin = [&](QDoubleSpinBox* s, const char* name, double lo, double hi, double value, const char* unit) {
        s->setObjectName(QString(name) + suffix); s->setRange(lo, hi); s->setDecimals(1); s->setValue(value);
        if (*unit) s->setSuffix(unit);
        s->setEnabled(f.on->isChecked()); QObject::connect(f.on, &QCheckBox::toggled, s, &QWidget::setEnabled);
    };
    spin(f.finalGap, "editorFinalGapTime", 0, 60, r ? r->finalGapTime : 1.0, " s");
    spin(f.start, "editorGapReductionStart", 0, 99.9, r ? r->start : 1.0, " ×");
    spin(f.end, "editorGapReductionEnd", 0.1, 100, r ? r->end : 2.0, " ×");
    // F12: each number kept while the box is unticked; `scope` names the place, e.g. "conflict/<id>/".
    rememberField(f.on, f.finalGap, scope + "finalGapTime"); rememberField(f.on, f.start, scope + "gapReductionStart");
    rememberField(f.on, f.end, scope + "gapReductionEnd");
    form->addRow(f.on);
    form->addRow("finalGapTime", f.finalGap); form->addRow("gapReductionStart", f.start); form->addRow("gapReductionEnd", f.end);
    return f;
}
}
