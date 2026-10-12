#include "canvas.hpp"
#include "map_layer.hpp"
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
}
}
