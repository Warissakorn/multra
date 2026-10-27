#pragma once
#include <cmath>
#include "../project/document.hpp"
#include "../model/network/display.hpp"
#include "../model/network/connector_surface.hpp"
#include "../model/network/right_of_way.hpp"
#include "rear_axle_pose.hpp"
#include "lane_change_pose.hpp"
#include "road_crossbar.hpp"
#include <QGraphicsView>
#include <QCursor>
#include <QPainterPath>
#include <functional>
#include <optional>
#include <map>
#include <QTimer>
#include "map_provider.hpp"
class QLabel;
class QImage;

namespace multra {
class MapLayer;
enum class MapStatus;
struct MapLoadState;
struct CanvasImageOptions;
class EditorCanvas : public QGraphicsView {
public:
    enum class Tool { select, draw, split, measure, calibrate, connect, route, input, head, conflict, counter, section, zone, node, road };
    explicit EditorCanvas(QWidget* parent = nullptr);
    ~EditorCanvas() override;
    void setMapProviders(std::vector<MapProvider>);
    void setMapVisible(bool);
    // Retry the visible map's failed tiles without changing the document or camera.
    void retryMapTiles();
    MapLoadState mapLoadState() const;
    // Unit formats for the HUD's scale and zoom labels, each with one %1 (D197).
    void setHudUnits(const QString& metres, const QString& zoom);
    void setMapStatusText(std::function<QString(MapStatus)>);
    bool centerOnGeographic(LonLat);
    void setDisplayCatalog(DisplayCatalog catalog) { display_=std::move(catalog); redraw(); }
    void setVisibleLevel(std::optional<int> level);
    // Explicit selection from a table/inspector reveals hidden objects. Changing the
    // level filter instead drops hidden selections before they can be edited.
    std::function<void(std::optional<int>)> visibleLevelChanged;
    void setBackgroundVisible(bool visible) { backgroundVisible_=visible; redraw(); }
    // Vissim's simple link display (Ctrl+A): Links and Connectors draw, hit and select as their
    // centre line only. View state, never saved; vehicles keep their lane positions.
    void setWireframe(bool on) { wireframe_=on; redraw(); }
    bool wireframe() const { return wireframe_; }
    QImage renderImage(const CanvasImageOptions&) const;
    void cycleOverlap();
    std::vector<std::pair<std::string,double>> hitObjects(Point,bool connectors = true) const;
    void setDocument(const ProjectDocument* document);
    void setTool(Tool tool);
    // One object is "primary": the last one added. Property edits act on it alone, so every
    // single-object gesture behaves exactly as it did before multi-selection existed.
    void select(const std::string& id);                   // replaces the selection with this object
    void setSelection(std::vector<std::string> ids);      // replaces; notifies once
    bool canSelect(const std::string& id) const { return objectLevel(id).has_value(); }
    void toggle(const std::string& id);                   // Shift-click selection semantics
    void frame(const std::string& id);                    // centre it, zooming only if it does not fit
    const std::vector<std::string>& selection() const { return selection_; }
    std::string selected() const { return selection_.empty() ? std::string{} : selection_.back(); }
    bool isSelected(const std::string& id) const;
    std::vector<std::string> inRectangle(Point a, Point b) const;
    const Connector* selectedConnector() const;
    bool pickingConnectorTarget() const { return connectorFrom_.has_value(); }
    void setRunNetwork(const Network&);
    void setRunFrame(const SimState&);
    void clearRunFrame();
    // D281: on by default; off paints the static network live, as before (tests compare the two).
    void setRunLayerCache(bool on) { runLayerCache_=on; redraw(); }
    std::size_t renderedVehicles() const { return runFrame_.vehicles.size(); }
    // Pixels per metre, whatever the view's rotation (D166); equals |m11| when north is up.
    double viewScale() const { return std::hypot(transform().m11(), transform().m12()); }
    double minimumViewScale() const;
    // D165 behaviour lab: keep this vehicle at the view centre and outline it; nullopt follows none.
    void setRunFollow(std::optional<std::uint64_t> vehicleId, bool headingUp = false);
    bool runHeadingUp() const { return runHeadingUp_; }
    std::optional<std::uint64_t> runFollow() const { return runFollow_; }
    // A left click on a run vehicle, when set; the editor leaves it unset, so its clicks are unchanged.
    std::function<void(std::uint64_t)> runVehicleClicked;
    std::function<void()> stopRequested;
    std::function<void()> deleteRequested;
    void redraw();
    void fitNetwork();
    void cancel();                                        // forget the gesture AND repaint
    void clearSelection(bool cancelGesture = true);       // includes table-owned highlights
    std::function<void()> selectionCleared;
    void removeVertex();
    bool snap{true};
    double grid{1};
    std::function<void(const std::vector<Point>&)> createLinkGesture;
    std::function<void(LaneReference,LaneReference,const std::vector<Point>&)> createRangeGesture;
    // The head tool's click: the lane or Connector path under the pointer and the station on
    // it, which is where the stop line goes. A drag of a selected head slides it along its lane.
    std::function<void(const HeadPlacement&)> headPlaced;
    std::function<void(const std::string&, double)> headMoved;
    // A route drawn by pointer: the segments, in travel order, exactly as the dialog would
    // have stored them. The canvas never writes to the document itself.
    std::function<void(std::vector<std::string>)> routeDraftCommitted;
    std::function<void(std::vector<std::string>,std::optional<double>)> positionedRouteCommitted;
    std::function<void(std::vector<std::string>,std::optional<double>,std::optional<RouteDestination>)> routeDestinationCommitted;
    std::optional<double> routeStartPosition() const { return routeStartPosition_; }
    std::function<void(const std::string&,double)> decisionMoved;
    std::function<void(std::string)> inputPlaced;
    const std::vector<std::string>& routeDraft() const { return routeDraft_; }
    const std::vector<std::string>& routePreview() const { return routePreview_; }
    std::function<void()> routeDraftChanged;
    void setDemandLabels(QString rate, QString periods) { inputRateLabel_=std::move(rate); inputPeriodsLabel_=std::move(periods); redraw(); }
    void commitRouteDraft();
    void dropLastRouteSegment();
    // Which route is drawn on the canvas. The shell owns table selection, so it says.
    void setHighlightedRoute(std::string id);
    // Begin a route at this segment without a click: the input tool offers it when no route
    // starts where the author wants an input.
    void startRouteDraft(const std::string& objectId);
    // The demand object drawn under this viewport position, for the context menu. Returns the
    // id of a vehicle input or of the drawn route, and an empty string for anything else.
    std::pair<std::string,std::string> demandObjectAt(QPoint viewportPosition) const;
    std::function<void(QPoint)> contextMenuRequested;
    const std::string& highlightedRoute() const { return highlightedRoute_; }
    // M3.2.4: the conflict area the shell's table has selected, drawn outlined. Areas are shown
    // green where a side has priority, red where it gives way and amber while undetermined.
    void setHighlightedConflict(std::string id);
    const std::string& highlightedConflict() const { return highlightedConflict_; }
    // S0.7 (NODES §16): the Node the shell's Nodes table has selected, drawn lit with its legs.
    void setHighlightedNode(std::string id);
    const std::string& highlightedNode() const { return highlightedNode_; }
    // S3.3b (ROADS §16): the Road the Roads table has selected: its centreline and generated Links lit,
    // in the warning colour while `detached` (a direction its template gives is no longer generated).
    void setHighlightedRoad(std::string id, bool detached);
    const std::string& highlightedRoad() const { return highlightedRoad_; }
    // M3.2.4b, the Conflict area tool (docs/audits/VISSIM_PARITY.md §2b). Only that tool hit-tests areas,
    // so a click at a junction under Select still selects the Link. A click picks an area; a click
    // on the highlighted one asks to cycle its priority; a drag on a waiting line slides it along
    // its own path. The canvas never writes to the document itself.
    std::function<void(const std::string&)> conflictPicked, conflictCycled;
    std::function<void(const std::string&, double)> waitingLineMoved;
    std::vector<std::string> conflictsAt(Point) const;   // groups whose visible lane bands contain it
    // M3.2.4c (D68): the areas the drawing implies (automaticConflicts), drawn under the Conflict
    // area tool only -- passive crossings grey, derived merges in their derived colours, both
    // dashed. A click on one where no authored area is asks to author it; the shell decides how.
    void setAutomaticConflicts(std::vector<AutomaticConflict>);
    std::string automaticAt(Point) const;               // the key of the automatic area under it
    std::function<void(const std::string&)> conflictAuthored;
    // M3.2.6c, the Queue counter tool (docs/audits/VISSIM_PARITY.md §2b). A click adds one measurement
    // line to an open draft: a stop line references its head, a waiting line references itself,
    // and anywhere else on a Link lane is an explicit point. Enter hands the draft over as one
    // counter; Backspace drops its last line; Esc drops it all.
    std::function<void(std::vector<MeasurementLine>)> counterDraftCommitted;
    const std::vector<MeasurementLine>& counterDraft() const { return counterDraft_; }
    void commitCounterDraft();
    // M5.4b, the Section tool (src/editor/canvas_sections.cpp): two Ctrl+right-clicks on Links
    // hand over a start and an end line; Esc drops a half-placed one.
    std::function<void(SectionLine start, SectionLine end)> sectionCommitted;
    const std::optional<SectionLine>& sectionStart() const { return sectionStart_; }
    // M8.6a (D161): a zone polygon is Ctrl+right-clicked vertex by vertex; Enter hands it over once it
    // has three, and the draft stays when the receiver refuses it (returns false).
    std::function<bool(std::vector<Point>)> zoneCommitted;
    std::function<bool(std::vector<Point>)> nodeCommitted; // S0.1 (NODES §6): the Node tool shares the draft
    std::function<bool(std::vector<Point>)> roadCommitted; // S3.3b: the Road tool's centreline, open, 2+ points
    const std::vector<Point>& zoneDraft() const { return zoneDraft_; }
    std::function<void(Point)> duplicateRequested;
    std::function<void(Point)> translateRequested;
    std::function<void(Point,double)> rotateRequested;
    std::optional<Point> rotationPivot() const;
    std::function<void(int,int,bool)> resizeRangeRequested;
    std::function<void(int,bool)> resizeLinkRequested;
    std::function<void()> creationRejected;
    std::function<void()> selectionChanged;
    std::function<void(const std::vector<Point>&)> createLink;
    std::function<void(const LaneReference&, const LaneReference&)> createConnector;
    // Dragging a connector's end grip onto another lane or another position along it.
    std::function<void(bool leading, LaneReference)> moveConnectorEndpoint;
    std::function<void(const LaneReference&)> connectorSourcePicked;
    std::function<void()> connectorDraftChanged;
    std::function<void(const std::string&, const std::vector<Point>&)> editGeometry;
    std::function<void(const std::string&, double)> splitAt;
    std::function<void(Point, Point, bool)> measured;
    std::function<void(Point)> cursorMoved;
protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void leaveEvent(QEvent*) override;
    void drawBackground(QPainter*, const QRectF&) override;
    void drawForeground(QPainter*, const QRectF&) override;
    void scrollContentsBy(int dx, int dy) override;
    bool focusNextPrevChild(bool) override;
    bool event(QEvent*) override;
private:
    std::unique_ptr<MapLayer> mapLayer_;
    QTimer mapViewportTimer_;
    QLabel *mapCredit_{}, *mapStatus_{};
    class CanvasHud* hud_{};
    QTransform hudTransform_; // the transform the HUD last showed
    void updateHud();
    bool mapVisible_{true};
    std::function<QString(MapStatus)> mapStatusText_;
    void initializeMap();
    void scheduleMapUpdate();
    void updateMapViewport();
    void updateMapOverlay();
    void paintMap(QPainter*,const QRectF&);
    // The grid spacing the cached background was drawn with; redraw() drops the cache when
    // `grid` no longer matches it. Zoom and pan invalidate the cache inside QGraphicsView.
    double cachedGrid_{1};
    DisplayCatalog display_;
    std::optional<int> visibleLevel_;
    bool backgroundVisible_{true}, creating_{}, wireframe_{};
    Tool creationTool_{Tool::draw};
    QPoint creationStart_, copyStart_;
    std::string copyPick_;
    bool copyArmed_{}, copyDragging_{};
    Point copyOffset_{};
    // A drag on a multi-selection moves the whole of it. groupDrag_ is armed on the press;
    // groupDragging_ turns on once the pointer has travelled far enough to be a drag and not
    // a click, which is what keeps a plain click on a selected object from moving anything.
    bool groupDrag_{}, groupDragging_{};
    Point groupOffset_{};
    std::optional<Point> rotationPivot_;
    Point rotationStart_{};
    double rotationDegrees_{};
    bool rotationDragging_{};
    void startRotation(QPoint);
    void updateRotation(QPoint, bool angleSnap);
    void drawRotationPreview();
    void drawDemandOverlay();
    void drawConflicts();
    std::string highlightedConflict_;
    std::vector<AutomaticConflict> automatic_;
    std::vector<ConflictGroup> conflictGroups_;
    std::string waitingLineAt(Point) const;
    bool conflictPress(QMouseEvent*);
    void updateLineDrag(QPoint);
    void finishLineDrag(QPoint);
    struct LineDrag { std::string id; std::vector<Point> polyline; double original{}, station{}; bool moved{}; std::optional<ConnectorEquation> equation{}; };
    std::optional<LineDrag> lineDrag_;
    std::vector<MeasurementLine> counterDraft_;
    bool counterPress(QMouseEvent*);
    std::optional<MeasurementLine> counterLineAt(Point) const;
    void drawCounters();
    std::optional<SectionLine> sectionStart_;
    bool sectionPress(QMouseEvent*);
    std::vector<Point> zoneDraft_;
    std::optional<Point> draftPointer_; // where a zone or node draft's pending edges meet the pointer
    bool zonePress(QMouseEvent*);
    // D306: Ctrl+right-click starts a multi-click draft; once it is open a plain left click adds.
    bool authoringPress(const QMouseEvent*, bool draftOpen) const;
    void commitZoneDraft();
    void drawZones();
    void drawNodes();
    void drawRoads();
    bool polygonTool() const { return tool_ == Tool::zone || tool_ == Tool::node || tool_ == Tool::road; }
    void drawNodeLegs(const Node&);
    std::string highlightedNode_;
    std::string highlightedRoad_; bool highlightedRoadDetached_{};
    std::optional<SectionLine> sectionLineAt(Point) const;
    void drawSections();
    void drawRouteOverlay(const std::vector<std::string>&, const std::string& id, bool preview);
    std::string objectAt(Point) const;
    bool isLink(const std::string& objectId) const;
    std::vector<std::string> routeDraftWith(const std::string& target) const;
    bool demandPress(QMouseEvent*);
    bool demandHover(QMouseEvent*);
    void clearRouteDraft();
    void reject();
    std::vector<std::string> routeDraft_;
    std::optional<double> routeStartPosition_;
    std::optional<RouteDestination> routeEnd_;
    QGraphicsLineItem* previewRouteEnd_{}; // owned by scene; cleared before each scene rebuild
    void updateRouteEnd(const std::string& target,Point pointer);
    struct DecisionDrag { std::string id,linkId;double position{};QPoint press;bool moved{}; };
    std::optional<DecisionDrag> decisionDrag_;
    bool startDecisionDrag(QPoint);
    void updateDecisionDrag(QPoint);
    void finishDecisionDrag(QPoint);
    void drawPositionedDecisions();
    std::vector<std::string> routePreview_;
    bool routeTracing_{};
    QString inputRateLabel_, inputPeriodsLabel_;
    std::string hoverSegment_, highlightedRoute_;
    bool hoverReachable_{};
    // Connector geometry is the most expensive thing a frame does, and a frame recomputed all
    // of it even when nothing had moved. What `connectorPaths` and `connectorBoundaries` read is
    // exactly the Connector, the two Links it names and the driving side -- nothing else in the
    // Network can change their answer -- so those values ARE the cache key, compared by value
    // rather than trusted from a revision counter. A preview Connector simply misses.
    struct CachedConnector {
        Connector connector; Link from, to; DrivingSide side{};
        std::optional<std::vector<ConnectorPath>> paths;
        std::optional<std::vector<std::vector<Point>>> boundaries;
        std::optional<ConnectorSurface> surface;
    };
    mutable std::map<std::string,CachedConnector> connectorCache_;
    CachedConnector& connectorEntry(const Connector&) const;
    void pruneConnectorCache();
    const std::vector<ConnectorPath>& cachedPaths(const Connector&) const;
    const std::vector<std::vector<Point>>& cachedBoundaries(const Connector&) const;
    const ConnectorSurface& cachedSurface(const Connector&) const;
    void drawCopyPreview();
    QPainterPath objectShape(const std::string&) const;
    QPainterPath linkShape(const Link&) const;
    QPainterPath connectorShape(const Connector&) const;
    std::optional<std::pair<Point,int>> headPosition(const NetworkSignalHead&) const;
    struct HeadGeometry { std::vector<Point> points; double width{}; int level{}; std::optional<ConnectorEquation> equation{}; };
    std::optional<HeadGeometry> headGeometry(const NetworkSignalHead&) const;
    std::optional<HeadPlacement> headAt(Point) const;
    QPainterPath headShape(const NetworkSignalHead&) const;
    std::optional<std::pair<Point,Point>> headBar(const NetworkSignalHead&) const; // its stop line
    bool headPress(QMouseEvent*);
    bool headHover(QMouseEvent*);
    bool startHeadDrag(const std::string& id, QPoint press);
    void updateHeadDrag(QPoint);
    void finishHeadDrag(QPoint);
    void drawHeads();
    struct HeadDrag { std::string id; std::vector<Point> geometry; double original{}, station{}; bool moved{}; std::optional<ConnectorEquation> equation{}; };
    std::optional<HeadDrag> headDrag_;
    std::optional<HeadPlacement> hoverHead_;
    std::optional<LaneReference> gestureFrom_;
    int rangeCorner_{}, previewFromCount_{1}, previewToCount_{1};
    Point lastPick_{};
    bool levelVisible(int level) const { return !visibleLevel_ || *visibleLevel_==level; }
    std::optional<int> objectLevel(const std::string&) const;
    bool mouseGestureActive() const;
    const DisplayType& style(const std::string&) const;
    std::optional<LaneReference> nearestLane(Point) const;
    void insertVertex(Point);
    void drawRunItems();
    // D281: while a run frame is shown, the static items under the lowest run level are painted
    // into the cached background and hidden from the scene, so a Step repaints only what moves.
    void bakeRunLayer();
    void paintRunLayer(QPainter*, const QRectF&);
    std::vector<QGraphicsItem*> runBaked_;
    bool runLayerBaked_{};
    std::vector<QGraphicsItem*> runItems_;
    // D283: run vehicles by id, moved each frame instead of rebuilt; the body they were drawn with.
    struct RunVehicle { class RunVehicleItem* item{}; std::uint32_t type{}; double scale{}; bool seen{}; };
    std::map<std::uint64_t,RunVehicle> runVehicles_;
    void dropRunVehicles(bool all);
    std::map<std::string,int> runLevels_;
    std::map<std::string,std::string> runStyles_;
    std::map<std::string,RoadCrossbar> runHeadBars_;
    std::map<std::uint64_t,LaneChangePath> runLaneChangePaths_;
    SimState runFrame_;
    std::optional<std::uint64_t> runFollow_;
    bool runHeadingUp_{}, runLayerCache_{true};
    std::map<std::string,std::vector<Point>> runGeometry_;
    std::map<std::string,ConnectorEquation> runEquations_;
    std::map<std::pair<std::uint32_t,std::uint32_t>,RearAxlePath> runAxlePaths_;
    const ProjectDocument* document_{};
    QGraphicsScene scene_;
    std::shared_ptr<const std::string> cachedImage_;
    QPixmap image_;
    Tool tool_{Tool::select};
    std::vector<std::string> selection_;
    std::optional<QRectF> band_;
    bool additive_{};
    std::vector<Point> draft_, preview_, original_;
    std::optional<LaneReference> connectorFrom_, connectorHover_;
    int vertex_{-1};
    bool dragging_{}, panning_{};
    QPoint panStart_, panPress_, dragPress_;
    Point dragStart_{};
    Point world(QPoint position, bool snapped = true) const;
    const Link* selectedLink() const;
    const std::vector<Point>* selectedGeometry() const;
    std::pair<std::string, double> hit(Point p, bool connectors = true) const;
    // Where the primary object's geometry points are shown: the middle of the whole bundle,
    // point for point with the stored polyline. Dragging maps back through the same offsets.
    std::vector<Point> handleGeometry() const;
    Point handleOffset_{};
    std::optional<bool> endpointDrag_;          // set while a connector end grip is held
    std::optional<LaneReference> endpointDraft_; // where that end would land
    std::optional<LaneReference> connectorEndpointTarget(Point, bool leading) const;
    int vertexAt(QPoint position) const;
    std::optional<LaneReference> hitLanePosition(Point p, bool outgoing) const;
    void pickConnector(Point p);
    void notifySelection();
    // cancel() without the repaint, for callers that redraw for their own reasons anyway.
    void resetGesture();
    void drawConnectors();
    void clearSelectionState();
    void clearHighlights();
    void clearHover();
    void updateHover(QPoint);
    // `edges` are the object's two long edges in travel order; a selected object carries its
    // direction on them as arrowheads, not in the middle of the road.
    void drawObjectFeedback(const std::string&, const QPainterPath&, double z,
                            const std::vector<std::vector<Point>>& edges = {});
    void drawGeometryHandles(const std::string&, const std::vector<Point>&, bool connector);
    // Wireframe: a centre line `pixels` wide on screen as a region, and the drawn line itself
    // (tagged "centre-line") with its selection feedback riding it.
    QPainterPath centreStroke(const std::vector<Point>&, double pixels) const;
    void drawCentreLine(const std::string& id, const std::vector<Point>&, const QColor&, double z);
    // A filled arrowhead of constant screen size, centred on `at` and pointing along `angle`.
    void addArrowhead(Point at, double angle, double pixels, QColor, double z, const QString& tag);
    // True once a move is visibly under way: the lane tabs, which follow the committed document,
    // are hidden until it is released rather than left behind at the old place.
    bool movingGeometry() const;
    bool movingBody() const;
    void showMoveCursor();
    // D306: the arrow in Select, the arrow with the tool's glyph beside it in every other tool.
    QCursor idleCursor() const;
    // A preview line: solid, on a translucent Base casing so it reads over roads and maps (D306).
    QGraphicsPathItem* addPreviewPath(const QPainterPath&, double z, const QPen&, const QBrush& = Qt::NoBrush);
    std::string hoverObject_, hoverConflict_, hoverAutomatic_, hoverWaitingLine_;
    int hoverVertex_{-1}, hoverLaneKind_{}, hoverLaneLocation_{-1};
    struct LaneHandle { Point position, anchor, direction; double width; int kind, count, maximum; int location{1}; double tabLength{}; };
    std::vector<LaneHandle> laneHandles() const;
    void previewLinkLanes(Link&) const;
    QPainterPath laneHandlePath(const LaneHandle&, double padding = 0) const;
    std::optional<LaneHandle> laneHandleAt(QPoint) const;
    bool startLaneResize(QPoint);
    void updateLaneResize(QPoint);
    void drawLaneHandles();
    std::optional<LaneHandle> laneResize_;
    int previewLinkCount_{};
};
}
