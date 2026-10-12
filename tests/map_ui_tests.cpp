#include "../src/editor/map_layer.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/commands/map_commands.hpp"
#include <QApplication>
#include <QAction>
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <algorithm>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QDateTime>
#include <QLabel>
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <map>
using namespace trafficsim;
namespace {
void require(bool value,const char* message) {if (!value) throw std::runtime_error(message);}
void waitFor(const std::function<bool()>& predicate,int timeout=3000) {
    QElapsedTimer timer;timer.start();
    while (!predicate() && timer.elapsed()<timeout) {QApplication::processEvents();QTest::qWait(10);}
    require(predicate(),"Timed out waiting for local map fixture");
}
QByteArray png() {
    QImage image(256,256,QImage::Format_RGB32);image.fill(Qt::blue);
    QPainter p(&image);p.fillRect(0,0,128,128,Qt::red);p.fillRect(128,0,128,128,Qt::green);p.fillRect(0,128,128,128,Qt::yellow);p.end();
    QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);require(image.save(&buffer,"PNG"),"Fixture PNG failed");return bytes;
}
class Server {
public:
    QTcpServer server;
    int requests{},conditional{},peak{};
    bool invalid{},limited{},hold{},revalidate{};
    QByteArray payload{png()},extraHeaders{"Cache-Control: max-age=3600\r\nETag: \"fixture\"\r\n"};
    std::vector<QByteArray> headers;
    std::vector<QTcpSocket*> held;
    Server() {
        require(server.listen(QHostAddress::LocalHost,0),"Local tile server did not listen");
        QObject::connect(&server,&QTcpServer::newConnection,&server,[this]{
            while (server.hasPendingConnections()) {
                auto* socket=server.nextPendingConnection();auto buffer=std::make_shared<QByteArray>();
                QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
                QObject::connect(socket,&QTcpSocket::readyRead,&server,[this,socket,buffer]{
                    buffer->append(socket->readAll());if (!buffer->contains("\r\n\r\n")) return;
                    const auto request=*buffer;buffer->clear();++requests;headers.push_back(request);
                    if (request.toLower().contains("if-none-match:")) ++conditional;
                    if (hold) {held.push_back(socket);peak=std::max(peak,static_cast<int>(held.size()));return;}
                    if (revalidate && request.toLower().contains("if-none-match:")) {
                        socket->write("HTTP/1.1 304 Not Modified\r\nCache-Control: max-age=3600\r\nETag: \"fixture\"\r\nConnection: close\r\n\r\n");
                    }
                    else if (limited) {socket->write("HTTP/1.1 429 Too Many Requests\r\nRetry-After: 2\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
                    else {
                        const auto body=invalid?QByteArray("broken image"):payload;
                        socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n"+extraHeaders+
                            "Content-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);
                    }
                    socket->disconnectFromHost();
                });
            }
        });
    }
    MapProvider provider(std::string id="fixture") const {
        return {id,"Fixture",QString("http://127.0.0.1:%1/{z}/{x}/{y}.png").arg(server.serverPort()),
            "Fixture credit",QUrl("https://example.org/credit"),0,19,256};
    }
};
void tilesAndCache() {
    Server server;QTemporaryDir cache;require(cache.isValid(),"Cache directory failed");const auto provider=server.provider();
    const TileKey key{provider.id,15,25000,15000};
    {
        MapTileClient client(cache.path());client.setProvider(provider);
        client.setWantedTiles({key});client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});require(server.requests==1,"Duplicate network request");
        require(server.headers.front().contains("TrafficSim/"),"Missing application User-Agent");
        require(!server.headers.front().contains("Cache-Control: no-cache"),"Request bypassed cache");
    }
    QTest::qWait(50);
    {
        MapTileClient client(cache.path());client.setProvider(provider);client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});require(server.requests==1,"Fresh disk cache missed");
    }
    Server expiring;expiring.revalidate=true;
    expiring.extraHeaders="Cache-Control: max-age=1\r\nETag: \"fixture\"\r\n";
    {
        MapTileClient client(cache.path()+"/expired");client.setProvider(expiring.provider());client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});require(expiring.requests==1,"Expiry fixture did not load");
    }
    QTest::qWait(1200);
    {
        MapTileClient client(cache.path()+"/expired");client.setProvider(expiring.provider());client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});
        if(expiring.requests!=2 || expiring.conditional!=1) {
            std::cerr<<"requests="<<expiring.requests<<" conditional="<<expiring.conditional<<"\n";
            for(const auto& h:expiring.headers)std::cerr<<h.toStdString();
            throw std::runtime_error("Expired cache was not revalidated");
        }
    }
    Server fallback;fallback.extraHeaders="ETag: \"fixture\"\r\n";
    for (int repeat=0;repeat<2;++repeat) {
        MapTileClient client(cache.path()+"/fallback");client.setProvider(fallback.provider());client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});
    }
    require(fallback.requests==1,"Missing freshness did not receive seven-day cache fallback");
    Server uncached;uncached.extraHeaders="Cache-Control: no-store\r\n";
    for(int repeat=0;repeat<2;++repeat) {
        MapTileClient client(cache.path()+"/no-store");client.setProvider(uncached.provider());client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});
    }
    require(uncached.requests==2,"no-store response was persisted");
    Server bad;bad.invalid=true;MapTileClient client(cache.path()+"/invalid");client.setProvider(bad.provider());
    client.setWantedTiles({key});waitFor([&]{return client.hasFailures();});require(bad.requests==1,"Bad-image forcing did not work");
    require(client.image(key).isNull(),"Bad PNG accepted");client.stop();
    Server throttled;throttled.limited=true;client.setProvider(throttled.provider());client.setWantedTiles({key});
    waitFor([&]{return client.hasFailures();});require(throttled.requests==1,"429 forcing did not work");
    QTest::qWait(350);require(throttled.requests==1,"Retry-After ignored");client.stop();
    Server slow;slow.hold=true;client.setProvider(slow.provider());client.setTimeout(150);
    client.setWantedTiles({key});waitFor([&]{return slow.requests==1;});waitFor([&]{return client.hasFailures();});
    require(client.image(key).isNull(),"Timed-out reply accepted");client.stop();
    Server bounded;bounded.hold=true;client.setProvider(bounded.provider());client.setTimeout(15000);
    std::set<TileKey> wanted;for (int i=0;i<20;++i)wanted.insert({"fixture",15,25000+i,15000});
    client.setWantedTiles(wanted);waitFor([&]{return bounded.requests==4;});QTest::qWait(100);
    require(bounded.requests==4,"More than four active requests");client.stop();QTest::qWait(30);
    require(client.pending()==0,"Stop retained pending work");
}
void render() {
    GeoProjection projection(GeoReference{});const auto centre=projection.worldToMercator({0,0});
    const auto key=tileAt("fixture",16,centre);const auto mesh=buildMapMesh(projection,key,2);
    require(mesh.sampledError*2<=.5,"Mesh missed error budget");
    QImage source;source.loadFromData(png(),"PNG");QImage result(512,512,QImage::Format_RGB32);result.fill(Qt::black);
    QPolygonF polygon;for(const auto p:mesh.vertices)polygon<<p;
    const auto bounds=polygon.boundingRect();
    QPainter painter(&result);painter.translate(256,256);painter.scale(480/bounds.width(),-480/bounds.height());
    painter.translate(-bounds.center().x(),-bounds.center().y());paintMapMesh(painter,source,mesh);painter.end();
    require(result.pixelColor(150,150).red()>200,"North-west image orientation wrong");
    require(result.pixelColor(350,150).green()>80,"North-east image orientation wrong");
    require(result.pixelColor(150,350).red()>200 && result.pixelColor(150,350).green()>200,"South-west orientation wrong");
    // The diagonal shared by the two triangles must not expose background holes.
    require(result.pixelColor(256,256)!=Qt::black,"Mesh seam left a hole");
}
void layerLifecycle() {
    Server server;QTemporaryDir cache;MapLayer layer(cache.path());layer.setProviders({server.provider()});
    MapSettings settings;settings.providerId="fixture";layer.setSettings(settings);
    const QPolygonF view{{-150,150},{150,150},{150,-150},{-150,-150}};layer.setView(view,2);
    waitFor([&]{return layer.status()==MapStatus::ready;});const int requests=server.requests;
    require(requests>0 && layer.wantedTiles().size()<=128,"Layer requested no tiles or too many tiles");
    layer.setVisible(false);layer.setView(view,2);QTest::qWait(100);require(server.requests==requests,"Hidden layer made requests");
    settings.reference.anchorLongitude=100;layer.setSettings(settings);layer.setVisible(true);layer.setView(view,2);
    waitFor([&]{return layer.status()==MapStatus::ready;});
    require(layer.geographicToWorld({100,13}).has_value(),"New reference was not installed");
    layer.setView(QPolygonF{{200000,0},{201000,0},{201000,1000},{200000,1000}},2);
    require(layer.status()==MapStatus::outsideStudy && layer.wantedTiles().empty(),"Out-of-study view fetched tiles");
    layer.setSettings({});require(layer.status()==MapStatus::disabled,"Removal retained map");
}
void editor(const char* data,const QString& screenshot) {
    EditorWindow window(data);window.resize(1000,700);window.show();QApplication::processEvents();
    Server fixture;window.canvas()->setMapProviders({fixture.provider("osm-standard")});
    window.canvas()->createLink({{-60,0},{60,0}});
    require(!window.history().document().network.links.empty(),"Authored-network fixture failed");
    const auto before=window.history().document();bool filled=false;
    QTimer::singleShot(0,[&]{
        auto* dialog=window.findChild<QDialog*>("editorMapSettingsDialog");require(dialog,"Map settings dialog missing");
        dialog->findChild<QDoubleSpinBox*>("editorMapLongitude")->setValue(100.5);
        dialog->findChild<QDoubleSpinBox*>("editorMapLatitude")->setValue(13.7);
        auto* buttons=dialog->findChild<QDialogButtonBox*>();filled=true;buttons->button(QDialogButtonBox::Ok)->click();
    });
    window.findChild<QAction*>("editorMapSettings")->trigger();require(filled,"Dialog fixture did not run");
    require(window.history().document().map.has_value(),"Dialog did not commit settings");
    require(window.history().document().network==before.network,"Map moved authored network");
    waitFor([&]{return fixture.requests>0 && !window.findChild<QLabel*>("editorMapStatus")->isVisible();});
    const auto bounds=window.canvas()->scene()->itemsBoundingRect();
    require(window.findChild<QLabel*>("editorMapCredit")->isVisible(),"Map credit is hidden");
    if (!screenshot.isEmpty())require(window.grab().save(screenshot),"Map preview failed");
    window.canvas()->fitNetwork();QApplication::processEvents();
    for(const auto* item:window.canvas()->scene()->items())
        require(item->type()!=QGraphicsPixmapItem::Type,"Map tiles became scene items");
    const auto fitted=window.canvas()->mapToScene(window.canvas()->viewport()->rect()).boundingRect();
    require(fitted.width()<bounds.width()*1.6,"Map tiles enlarged Fit network bounds");
    window.findChild<QAction*>("editorToggleMap")->trigger();
    const auto revision=window.history().revision();window.canvas()->centerOnGeographic({100.5,13.7});
    require(window.history().revision()==revision,"Navigation dirtied project");
    QTemporaryDir files;const auto path=files.filePath("map.traffic.json");window.saveFile(path);window.openFile(path);
    require(window.history().document().map->reference.anchorLongitude==100.5,"Open lost georeference");
    auto* language=window.findChild<QComboBox*>("editorLanguage");language->setCurrentIndex(1);
    require(!window.findChild<QAction*>("editorMapSettings")->text().isEmpty(),"Thai map action missing");
    window.findChild<QAction*>("editorRemoveMap")->trigger();require(!window.history().document().map,"Remove failed");
    window.findChild<QAction*>("editorUndo")->trigger();require(window.history().document().map.has_value(),"Undo lost map");
    window.findChild<QAction*>("editorRedo")->trigger();require(!window.history().document().map,"Redo failed");
    window.saveFile(path);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {tilesAndCache();render();layerLifecycle();if(argc>1)editor(argv[1],argc>2?QString::fromLocal8Bit(argv[2]):QString());std::cout<<"map UI checks passed\n";return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
