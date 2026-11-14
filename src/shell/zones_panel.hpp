#pragma once
#include "object_tab_panel.hpp"
#include "../model/network/node_values.hpp"
#include <functional>
#include <string>
#include <vector>

class QTableWidget;
namespace multra {
// What the Zones tab reaches outside itself: the lot dialog (editor_od.cpp) and the Street segment
// selected in its tab, for a connector to it.
struct ZonesHooks { std::function<void(const std::string& zoneId)> editLot; std::function<std::string()> selectedStreetSegment; };
// M8.6a (D161, ZONES_AND_PARKING §9), D521: the Zones object tab -- zones beside the selected zone's
// connectors, their dialogs, and the Zone tool's commit (one Undo step per drawn zone).
class ZonesPanel final : public ObjectTabPanel {
public:
    ZonesPanel(QWidget* parent, ObjectTabContext, ZonesHooks);
    void translate();
    void refresh();
    void invalidate() { tab_.invalidate(); connectorsOf_.clear(); }
    void edit(const std::string& id);
    void editConnector(const std::string& id);
    void addConnector(bool origin); // to the one Link selected on the canvas
    bool commitTool(std::vector<Point> polygon);
    std::string selected() const;
    std::string selectedConnector() const;
private:
    ZonesHooks hooks_;
    QTableWidget* connectorTable_{};
    std::string connectorsOf_; // the zone the connector table lists
};
}
