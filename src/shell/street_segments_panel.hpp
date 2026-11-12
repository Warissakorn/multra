#pragma once
#include "object_tab_panel.hpp"
#include "../model/demand/definition.hpp"
#include "../model/network/network.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

class QComboBox;
namespace multra {
struct PriorityDefaults;
enum class StreetSegmentStatus { noTemplate, unknownTemplate, notApplied, applied, detached };
StreetSegmentStatus streetSegmentStatus(const StreetSegment&, const std::vector<CrossSection>&);
// What the Street segments tab reaches outside itself: the window's template rows and Default
// template (from the Properties lane count and width), its dialogs, priority defaults, and the canvas.
struct StreetSegmentsHooks {
    std::function<std::vector<CrossSection>()> crossSectionRows;
    std::function<CrossSection()> defaultCrossSection;
    std::function<QString()> defaultTemplateLabel;
    std::function<PriorityDefaults()> priorityDefaults;
    std::function<void()> findCrossings, seamChecks, redraw;
    std::function<void(std::string id, bool detached)> highlight;
};
// S3.3b (D307, STREET_SEGMENTS §16), D521: the Street segments object tab, its template combo, its
// dialog, and the Street segment tool's preview and commit (one Undo step, the template applied).
class StreetSegmentsPanel final : public ObjectTabPanel {
public:
    StreetSegmentsPanel(QWidget* parent, ObjectTabContext, StreetSegmentsHooks);
    void translate();
    void refresh();
    void edit(const std::string& id);
    std::string selected() const;
    std::vector<std::string> selectedRows() const; // table order (D490)
    std::vector<std::string> selectedLinks() const; // the Links selected on the canvas
    std::vector<Link> preview(const std::vector<Point>& centreline) const; // D433
    bool commitTool(std::vector<Point> centreline, int level);
    // D433: the combo's template for a Command; Default reuses an equal catalog template or adds one.
    std::optional<std::string> chosenTemplate(ProjectDocument&, const std::vector<CrossSection>& installed) const;
    QComboBox* templateBox() const { return template_; }
private:
    StreetSegmentsHooks hooks_;
    QComboBox* template_{};
};
}
