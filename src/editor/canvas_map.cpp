#include "canvas.hpp"
#include "map_layer.hpp"
#include "canvas_hud.hpp"
#include <QTimer>
#include <QLabel>
#include <QScrollBar>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <stdexcept>
#include <algorithm>
#include <cmath>
namespace trafficsim {
EditorCanvas::~EditorCanvas()=default;
void EditorCanvas::initializeMap() {
    const auto directory=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/map-tiles";
    mapLayer_=std::make_unique<MapLayer>(directory);
    mapCredit_=new QLabel(viewport());mapCredit_->setObjectName("editorMapCredit");
    mapCredit_->setTextFormat(Qt::RichText);mapCredit_->setOpenExternalLinks(true);
    mapCredit_->setAutoFillBackground(true);mapCredit_->setMargin(4);mapCredit_->hide();
    mapStatus_=new QLabel(viewport());mapStatus_->setObjectName("editorMapStatus");
    mapStatus_->setAutoFillBackground(true);mapStatus_->setMargin(4);mapStatus_->hide();
    hud_=new CanvasHud(viewport());
    mapLayer_->changed=[this]{
        resetCachedContent();viewport()->update();updateMapOverlay();
    };
    mapViewportTimer_.setSingleShot(true);mapViewportTimer_.setInterval(80);
    connect(&mapViewportTimer_,&QTimer::timeout,this,[this]{updateMapViewport();});
    for (auto* bar:{horizontalScrollBar(),verticalScrollBar()})
        connect(bar,&QScrollBar::valueChanged,this,[this]{scheduleMapUpdate();});
}
void EditorCanvas::setMapProviders(std::vector<MapProvider> providers) {
    mapLayer_->setProviders(std::move(providers));scheduleMapUpdate();
}
void EditorCanvas::setMapVisible(bool visible) {
    mapVisible_=visible;mapLayer_->setVisible(visible);scheduleMapUpdate();
}
void EditorCanvas::setMapStatusText(std::function<QString(MapStatus)> translator) {
    mapStatusText_=std::move(translator);updateMapOverlay();
}
bool EditorCanvas::centerOnGeographic(LonLat point) {
    const auto p=mapLayer_->geographicToWorld(point);if (!p) return false;
    centerOn(p->x,p->y);scheduleMapUpdate();return true;
}
void EditorCanvas::scheduleMapUpdate() {
    if (mapLayer_ && !mapViewportTimer_.isActive()) mapViewportTimer_.start();
}
void EditorCanvas::updateMapViewport() {
    if (!mapLayer_) return;
    const double scale=std::hypot(transform().m11(),transform().m12())*viewport()->devicePixelRatioF();
    mapLayer_->setView(mapToScene(viewport()->rect()),scale);updateMapOverlay();
}
void EditorCanvas::paintMap(QPainter* painter,const QRectF& rect) {
    if (mapLayer_) mapLayer_->paint(*painter,rect);
}
void EditorCanvas::updateMapOverlay() {
    if (!mapLayer_ || !mapCredit_) return;
    const auto credit=mapLayer_->attribution();
    mapCredit_->setText("<a href=\""+mapLayer_->attributionUrl().toString().toHtmlEscaped()+"\">"+credit.toHtmlEscaped()+"</a>");
    mapCredit_->setVisible(!credit.isEmpty());mapCredit_->adjustSize();
    mapCredit_->move(8,std::max(0,viewport()->height()-mapCredit_->height()-8));mapCredit_->raise();
    const auto status=mapLayer_->status();
    const auto label=mapStatusText_?mapStatusText_(status):QString();
    mapStatus_->setText(label);mapStatus_->setVisible(status!=MapStatus::disabled && status!=MapStatus::ready && !label.isEmpty());
    mapStatus_->adjustSize();mapStatus_->move(8,8);mapStatus_->raise();
    updateHud();
}
void EditorCanvas::setHudUnits(const QString& metres,const QString& zoom) {
    if (hud_) {hud_->setUnits(metres,zoom);updateHud();}
}
void EditorCanvas::updateHud() {
    if (!hud_ || !mapLayer_) return;
    hudTransform_=transform();
    // True north from the map's own projection: step north from the view centre and see where
    // that lands on screen. Convergence, the georeference rotation and any view rotation are all in.
    std::optional<double> north;
    const auto centre=mapToScene(viewport()->rect().center());
    if (const auto here=mapLayer_->worldToGeographic({centre.x(),centre.y()}))
        if (const auto ahead=mapLayer_->geographicToWorld({here->longitude,here->latitude+1e-4})) {
            const auto d=transform().map(QPointF(ahead->x,ahead->y))-transform().map(centre);
            if (std::hypot(d.x(),d.y())>1e-9) north=std::atan2(d.x(),-d.y())*180/M_PI;
        }
    hud_->setView(viewScale(),north);
    hud_->place(viewport()->rect(),mapCredit_ && mapCredit_->isVisible()?mapCredit_->geometry():QRect());
}
void EditorCanvas::scrollContentsBy(int dx,int dy) {
    // The view scrolls the viewport's child widgets with the scene; the corner overlays stay put.
    QGraphicsView::scrollContentsBy(dx,dy);updateMapOverlay();
}
void EditorCanvas::drawForeground(QPainter*,const QRectF&) {
    // A transform set without a map update (Lab heading-up frames) still refreshes the HUD, after paint.
    if (hud_ && transform()!=hudTransform_) QTimer::singleShot(0,this,[this]{updateHud();});
}
}
