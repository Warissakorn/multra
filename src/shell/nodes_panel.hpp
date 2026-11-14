#pragma once
#include "object_tab_panel.hpp"
#include "od_panel.hpp"
#include "../model/demand/definition.hpp"
#include "../model/network/node_values.hpp"
#include <functional>
#include <string>
#include <vector>

class QVBoxLayout;
namespace multra {
struct PriorityDefaults;
// D523 (docs/plans/OBJECT_PANELS_NODES_CONFLICTS.md): what the Nodes tab reaches outside itself.
struct NodesHooks {
    std::function<void(const std::string&)> editCoarse; // S3.6b (D311), editor_coarse.cpp
    std::function<QString()> projectFile;               // for the Node analysis dialog
    std::function<void()> analyseAll;                   // the Node analysis results, brought forward
    std::function<std::vector<CrossSection>()> crossSectionRows;
    std::function<PriorityDefaults()> priorityDefaults;
    OdChoices choices;                                  // for the gate-flow dialog
    std::function<void(const QString&, int ms)> status;
    std::function<void(const std::string&)> highlight;  // the canvas's lit Node
    // S4.4 (D317): Node proposals, which read the Macro and Meso results, under the table.
    std::function<void(QVBoxLayout*)> extend;
    std::function<void()> refreshExtension, translateExtension;
};
// S0.1 (D261, NODES §6), D523: the Nodes object tab. The Node tool's Enter commits a polygon through
// execute(), so a drawn Node is one Undo step; rename and delete go through History too.
class NodesPanel final : public ObjectTabPanel {
public:
    NodesPanel(QWidget* parent, ObjectTabContext, NodesHooks);
    void translate();
    void refresh();
    std::string selected() const;
    std::string commitTool(std::vector<Point> polygon); // the new Node's id; empty when refused
private:
    void rename();
    NodesHooks hooks_;
};
}
