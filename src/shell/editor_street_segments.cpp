#include "editor_window.hpp"
#include "street_segments_panel.hpp"
#include "../commands/street_segment_commands.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/link_type_codec.hpp"
#include "../project/street_segment_codec.hpp"
#include <QDoubleSpinBox>
#include <QSpinBox>

// S3.3b (D307, STREET_SEGMENTS §16): the template rows and the Default template the window shares
// with its dialogs; the Street segments tab is StreetSegmentsPanel since D521.
namespace multra {
std::vector<CrossSection> EditorWindow::crossSectionRows() const {
    std::vector<CrossSection> rows;
    try { for (auto& r : resolveCrossSections(history_.document().definition.value_or(AuthoringDefinition{}), data_)) rows.push_back(std::move(r.section)); }
    catch (const std::exception&) {} // an unreadable catalog lists nothing; Apply reports the template unknown
    return rows;
}
CrossSection EditorWindow::defaultCrossSection() const {
    const int lanes = count_ ? count_->value() : 1;
    const double width = width_ ? width_->value() : 3.5;
    return CrossSection{"", "Default", 2, lanes, lanes, width, {}, 0, std::nullopt, "Editor default", std::nullopt};
}
QString EditorWindow::defaultTemplateLabel() const {
    const auto s = defaultCrossSection();
    return text("editorStreetSegmentDefaultTemplate").arg(s.lanesForward).arg(editorDesign::formatValue(s.laneWidth, 2));
}
std::string EditorWindow::selectedStreetSegment() const { return streetSegments_ ? streetSegments_->selected() : std::string{}; }
}
