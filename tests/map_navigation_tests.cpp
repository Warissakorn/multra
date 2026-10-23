#include "../src/editor/canvas.hpp"
#include "../src/editor/map_layer.hpp"
#include <QApplication>
#include <QBuffer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace multra;
namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
double distance(QPointF a,QPointF b) {return std::hypot(a.x()-b.x(),a.y()-b.y());}
QPointF centre(const EditorCanvas& canvas) {return canvas.mapToScene(canvas.viewport()->rect().center());}
void unchanged(const EditorCanvas& canvas,QPointF before,const char* message) {
    require(distance(centre(canvas),before)*canvas.viewScale()<1.5,message);
}
void wheel(EditorCanvas& canvas,QPoint pointer,int delta) {
    QWheelEvent event(QPointF(pointer),QPointF(canvas.viewport()->mapToGlobal(pointer)),{},QPoint(0,delta),
        Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(canvas.viewport(),&event);
}
void drag(EditorCanvas& canvas,Qt::MouseButton button,QPoint from,QPoint to) {
    QTest::mousePress(canvas.viewport(),button,{},from);
    QMouseEvent move(QEvent::MouseMove,QPointF(to),QPointF(canvas.viewport()->mapToGlobal(to)),
        Qt::NoButton,button,Qt::NoModifier);
    QApplication::sendEvent(canvas.viewport(),&move);
    QTest::mouseRelease(canvas.viewport(),button,{},to);
}
ProjectDocument mappedDocument() {
    ProjectDocument document;document.map=MapSettings{};
    document.map->providerId="unavailable-fixture";return document;
}
void panAndClick() {
    // No provider is installed: camera regressions must never depend on live OSM.
    auto document=mappedDocument();EditorCanvas canvas;canvas.resize(800,600);canvas.setDocument(&document);
    canvas.show();QApplication::processEvents();canvas.setTransform(QTransform::fromScale(.05,-.05));
    for(const auto button:{Qt::RightButton,Qt::MiddleButton}) {
        canvas.centerOn(0,0);const auto before=centre(canvas);
        const QPoint from(110,120),to(710,420);const auto heldWorld=canvas.mapToScene(from);
        drag(canvas,button,from,to);
        const auto panned=centre(canvas);
        require(distance(before,panned)>10000,"Pan forcing did not move beyond the previous 10 km scene boundary");
        require(distance(canvas.mapToScene(to),heldWorld)*canvas.viewScale()<1.5,"Map did not follow the drag");
        QTest::mouseClick(canvas.viewport(),Qt::LeftButton,{},QPoint(230,330));
        unchanged(canvas,panned,"Clicking the panned map snapped the camera back");
        canvas.redraw();unchanged(canvas,panned,"Redrawing the panned map snapped the camera back");
        QTest::qWait(120);unchanged(canvas,panned,"Deferred map refresh changed the panned camera");
    }
}
void widerZoom() {
    auto document=mappedDocument();EditorCanvas canvas;canvas.resize(800,600);canvas.setDocument(&document);
    canvas.show();QApplication::processEvents();canvas.setTransform(QTransform::fromScale(.05,-.05));canvas.centerOn(0,0);
    const QPoint pointer=canvas.viewport()->rect().center()+QPoint(110,80);
    for(int step=0;step<32;++step) {
        const auto anchored=canvas.mapToScene(pointer);wheel(canvas,pointer,-120);
        require(distance(canvas.mapToScene(pointer),anchored)*canvas.viewScale()<2,"Zoom moved the point under the pointer");
        if(step==0)require(canvas.viewScale()<.05,"Map zoom still stops at the previous minimum");
    }
    require(std::abs(canvas.viewScale()-.0005)<1e-12,"Map zoom did not reach the regional overview minimum");
    const auto zoomedCentre=centre(canvas);canvas.redraw();
    unchanged(canvas,zoomedCentre,"Overview redraw changed the camera");
    ProjectDocument local;canvas.setDocument(&local);canvas.setTransform(QTransform::fromScale(.05,-.05));canvas.centerOn(0,0);
    wheel(canvas,canvas.viewport()->rect().center(),-120);
    require(std::abs(canvas.viewScale()-.05)<1e-12,"Map-free editor minimum zoom changed");
    canvas.setDocument(&document);canvas.resize(3840,2160);QApplication::processEvents();
    canvas.setTransform(QTransform::fromScale(.05,-.05));canvas.centerOn(0,0);
    wheel(canvas,canvas.viewport()->rect().center(),-10000);
    require(canvas.viewScale()>.0005,"Large viewport did not adapt its overview minimum");
    for(const auto p:canvas.mapToScene(canvas.viewport()->rect()))
        require(std::hypot(p.x(),p.y())<mapOverviewRadius,"Minimum zoom pushed a large viewport beyond its overview");
}
class Server {
public:
    QTcpServer server;
    int requests{};
    QByteArray png;
    Server() {
        QImage image(256,256,QImage::Format_RGB32);image.fill(Qt::green);
        QBuffer output(&png);output.open(QIODevice::WriteOnly);require(image.save(&output,"PNG"),"Overview fixture PNG failed");
        require(server.listen(QHostAddress::LocalHost,0),"Overview fixture server did not listen");
        QObject::connect(&server,&QTcpServer::newConnection,&server,[this]{
            while(server.hasPendingConnections()) {
                auto* socket=server.nextPendingConnection();auto buffer=std::make_shared<QByteArray>();
                QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
                QObject::connect(socket,&QTcpSocket::readyRead,&server,[this,socket,buffer]{
                    buffer->append(socket->readAll());if(!buffer->contains("\r\n\r\n"))return;
                    buffer->clear();++requests;
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nCache-Control: max-age=3600\r\n"
                        "Content-Length: "+QByteArray::number(png.size())+"\r\nConnection: close\r\n\r\n"+png);
                    socket->disconnectFromHost();
                });
            }
        });
    }
    MapProvider provider() const {
        return {"fixture","Overview fixture",QString("http://127.0.0.1:%1/{z}/{x}/{y}.png").arg(server.serverPort()),
            "Fixture credit",QUrl("https://example.org/credit"),0,19,256};
    }
};
void overviewTiles() {
    Server server;QTemporaryDir cache;require(cache.isValid(),"Overview cache directory failed");
    MapLayer layer(cache.path());layer.setProviders({server.provider()});MapSettings settings;settings.providerId="fixture";
    layer.setSettings(settings);const QPolygonF view{{-800000,600000},{800000,600000},{800000,-600000},{-800000,-600000}};
    layer.setView(view,.0005);
    require(!layer.wantedTiles().empty(),"Regional overview produced no visible tiles");
    require(layer.wantedTiles().size()<=128,"Regional overview exceeded the visible tile budget");
    require(layer.status()!=MapStatus::outsideStudy && layer.status()!=MapStatus::renderLimit,"Regional overview was refused");
    QElapsedTimer timer;timer.start();
    while(layer.status()!=MapStatus::ready && timer.elapsed()<5000)QTest::qWait(10);
    require(layer.status()==MapStatus::ready && server.requests>0,"Regional overview tiles did not load");
    require(server.requests==static_cast<int>(layer.wantedTiles().size()),"Regional overview requested extra tiles");
    GeoProjection projection(settings.reference);const MetrePoint far{500000,0};
    require(!projection.insideStudy(far) && projection.insideOverview(far),"Overview and engineering study limits were conflated");
    const auto geographic=projection.worldToGeographic(far);const auto world=layer.geographicToWorld(geographic);
    require(world && std::hypot(world->x-far.x,world->y-far.y)<1e-5,"Overview geographic navigation still uses the study limit");
    const auto back=layer.worldToGeographic(far);
    require(back && std::hypot(back->longitude-geographic.longitude,back->latitude-geographic.latitude)<1e-9,
        "Overview inverse coordinates still use the study limit");
    const MetrePoint outside{2000001,0};require(!projection.insideOverview(outside),"Overview extent was unbounded");
    require(!layer.worldToGeographic(outside),"Overview inverse accepted a point beyond its bound");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        panAndClick();widerZoom();overviewTiles();std::cout<<"map navigation checks passed\n";return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
