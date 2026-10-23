#include "../src/editor/map_layer.hpp"
#include "ui_test_storage.hpp"
#include "../src/editor/map_http_cache.hpp"
#include "../src/geo/projection_data.hpp"
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
#include <cmath>
#include <memory>
#include <fstream>
#include <limits>
using namespace multra;
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
    QByteArray responseTag{"fixture"};
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
                        socket->write("HTTP/1.1 304 Not Modified\r\nCache-Control: max-age=3600\r\nETag: \""+responseTag+"\"\r\nConnection: close\r\n\r\n");
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
        require(server.headers.front().contains("MULTRA/")&&server.headers.front().contains("github.com/Warissakorn/multra"),"Missing application User-Agent");
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
    layer.setView(QPolygonF{{2100000,0},{2101000,0},{2101000,1000},{2100000,1000}},2);
    require(layer.status()==MapStatus::outsideStudy && layer.wantedTiles().empty(),"Out-of-study view fetched tiles");
    layer.setSettings({});require(layer.status()==MapStatus::disabled,"Removal retained map");
}
void corruptCache() {
    Server server;server.revalidate=true;QTemporaryDir cache;const TileKey key{"fixture",15,25000,15000};
    {
        QNetworkDiskCache disk;disk.setCacheDirectory(cache.path());QNetworkCacheMetaData metadata;
        metadata.setUrl(server.provider().tileUrl(key));metadata.setExpirationDate(QDateTime::currentDateTimeUtc().addSecs(3600));
        metadata.setRawHeaders({{"ETag","\"fixture\""}});metadata.setSaveToDisk(true);
        auto* writer=disk.prepare(metadata);require(writer,"Corrupt-cache setup failed");
        writer->write("broken PNG");disk.insert(writer);
        std::unique_ptr<QIODevice> read(disk.data(metadata.url()));require(read && read->readAll()=="broken PNG","Corrupt-cache forcing failed");
    }
    MapTileClient client(cache.path());client.setProvider(server.provider());client.setWantedTiles({key});
    waitFor([&]{return !client.image(key).isNull();});
    require(server.requests==1 && server.conditional==0,"Corrupt cache sent a validator and required an avoidable retry");
}
void widePng() {
    Server server;QImage source(512,512,QImage::Format_RGBA64);source.fill(QColor(200,100,40));
    QBuffer buffer(&server.payload);buffer.open(QIODevice::WriteOnly);require(source.save(&buffer,"PNG"),"16-bit fixture save failed");
    QImage check;require(check.loadFromData(server.payload,"PNG") && check.depth()==64,"16-bit forcing failed");
    auto provider=server.provider();provider.tilePixels=512;QTemporaryDir cache;MapTileClient client(cache.path());client.setProvider(provider);
    const TileKey key{"fixture",15,25000,15000};client.setWantedTiles({key});waitFor([&]{return !client.image(key).isNull();});
    require(client.image(key).depth()==32,"PNG depth exceeds the decoded-cache pixel budget");
}
void fitRefresh() {
    Server server;auto provider=server.provider();provider.maximumZoom=22;EditorCanvas canvas;
    canvas.resize(800,600);canvas.setMapProviders({provider});
    canvas.setMapStatusText([](MapStatus status){return status==MapStatus::ready || status==MapStatus::disabled?QString():QString("busy");});
    ProjectDocument document;document.map=MapSettings{};document.map->providerId="fixture";
    document.network.links.push_back({"road",{{-10,0},{10,0}},{{"lane",3.5}}});
    canvas.setDocument(&document);canvas.show();
    waitFor([&]{return server.requests>0 && !canvas.findChild<QLabel*>("editorMapStatus")->isVisible();});
    const int before=server.requests;const auto scale=canvas.transform().m11();canvas.fitNetwork();
    require(canvas.transform().m11()>scale*2,"Fit zoom forcing failed");
    QTest::qWait(250);require(server.requests>before,"Fit zoom kept old tile level when the centre did not move");
}
void dateline() {
    Server server;QTemporaryDir cache;MapLayer layer(cache.path());layer.setProviders({server.provider()});
    MapSettings settings;settings.providerId="fixture";
    for(const auto& reference:std::vector<GeoReference>{{"EPSG:32660",179.9999,13,0,0,0},
            {"EPSG:32701",-179.9999,-13,0,0,37},{"EPSG:32660",179.9999,83,0,0,23}}) {
        settings.reference=reference;layer.setSettings(settings);
        const QPolygonF view{{-150,150},{150,150},{150,-150},{-150,-150}};layer.setView(view,2);
        require(layer.status()!=MapStatus::renderLimit,"Local viewport crossing the date line became a world-wide request");
        require(!layer.wantedTiles().empty(),"Date-line view had no visible tiles");
        waitFor([&]{return layer.status()==MapStatus::ready;});require(layer.wantedTiles().size()<=128,"Date-line tile count unbounded");
    }
}
void largeAnchor() {
    Server server;EditorCanvas canvas;canvas.resize(800,600);canvas.setMapProviders({server.provider()});
    ProjectDocument document;document.map=MapSettings{};document.map->providerId="fixture";
    document.map->reference.anchorWorldX=1e9;
    bool rejected=false;try {validateDocument(document);}catch(const std::exception&){rejected=true;}
    require(rejected,"Unreachable anchor was accepted");
    document.map->reference.anchorWorldX=mapWorldAnchorLimit;canvas.setDocument(&document);canvas.show();QApplication::processEvents();
    require(canvas.centerOnGeographic({99,13}),"Large-anchor navigation rejected");
    const auto centre=canvas.mapToScene(canvas.viewport()->rect().center());
    require(std::abs(centre.x()-mapWorldAnchorLimit)<1,"Accepted map anchor cannot be reached by the Canvas scroll range");
}
void quotedFreshness() {
    Server server;server.revalidate=true;server.extraHeaders="Cache-Control: max-age=\"0\"\r\nETag: \"fixture\"\r\n";
    QTemporaryDir cache;const TileKey key{"fixture",15,25000,15000};
    for(int repeat=0;repeat<2;++repeat) {
        MapTileClient client(cache.path());client.setProvider(server.provider());client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});
    }
    require(server.requests==2 && server.conditional==1,"Quoted max-age used seven-day fallback");
}
void validatorMismatch() {
    Server server;server.extraHeaders="Cache-Control: max-age=0\r\nETag: \"fixture\"\r\n";
    QTemporaryDir cache;const TileKey key{"fixture",15,25000,15000};
    {
        MapTileClient client(cache.path());client.setProvider(server.provider());client.setWantedTiles({key});
        waitFor([&]{return !client.image(key).isNull();});require(server.requests==1,"Validator setup failed");
    }
    server.revalidate=true;server.responseTag="changed";
    server.extraHeaders="Cache-Control: max-age=3600\r\nETag: \"changed\"\r\n";
    MapTileClient client(cache.path());client.setProvider(server.provider());client.setWantedTiles({key});
    waitFor([&]{return !client.image(key).isNull();});
    require(server.requests==3,"Mismatched 304 validator reused an unvalidated representation");
}
void cachePolicy() {
    using namespace mapHttp;
    const auto now=QDateTime::fromSecsSinceEpoch(1700000000,Qt::UTC);
    require(expiration({{"Cache-Control","public, max-age=\"60\""},{"Age","20"}},now)==now.addSecs(40),"Age was ignored");
    require(expiration({{"Cache-Control","max-age=60"},{"Cache-Control","max-age=120"}},now)==now,"Duplicate freshness accepted");
    require(expiration({{"Cache-Control","max-age=invalid"}},now)==now,"Invalid freshness used fallback");
    require(expiration({{"Cache-Control","no-cache, max-age=60"}},now)==now,"no-cache was reused without validation");
    require(expiration({{"Expires","invalid"}},now)==now,"Invalid Expires used fallback");
    require(expiration({{"Date","Tue, 14 Nov 2023 22:13:20 GMT"},{"Expires","Tue, 14 Nov 2023 22:14:20 GMT"},{"Age","20"}},now)==now.addSecs(40),"Expires freshness ignored Age");
    const Headers cached{{"Cache-Control","max-age=60"},{"Date","Tue, 14 Nov 2023 21:00:00 GMT"},{"Age","3600"},{"ETag","\"one\""}};
    const auto validated=validatedHeaders(cached,{},now);
    require(header(validated,"age").isEmpty() && expiration(validated,now)==now.addSecs(60),"304 retained old response age");
    const auto repeated=validatedHeaders(cached,{{"Cache-Control","max-age=60"},{"Cache-Control","max-age=120"}},now);
    require(expiration(repeated,now)==now,"304 discarded duplicate cache directives");
    QNetworkRequest request;request.setRawHeader("If-None-Match","\"one\"");
    require(matchesValidation(cached,request,{{"ETag","W/\"one\""}}),"Matching weak validator was rejected");
    require(!matchesValidation(cached,request,{{"ETag","\"two\""}}),"Mismatched validator was accepted");
    require(retryDeadline("172800",1000)==172801000,"Retry-After was shortened to one day");
    require(retryDeadline("9999999999999999999999",1000)==std::numeric_limits<qint64>::max(),"Retry-After overflowed");
    require(retryDeadline("Wed, 15 Nov 2023 22:13:20 GMT",now.toMSecsSinceEpoch())==now.addDays(1).toMSecsSinceEpoch(),"HTTP Retry-After date was ignored");
    const auto old=QDateTime(QDate(1994,11,6),QTime(8,49,37),Qt::UTC);
    require(httpDate("Sunday, 06-Nov-94 08:49:37 GMT",now)==old,"RFC850 HTTP date rejected");
    require(httpDate("Sun Nov  6 08:49:37 1994",now)==old,"asctime HTTP date rejected");
}
void projectionBootstrap() {
    QTemporaryDir root;require(root.isValid(),"Projection fixture directory failed");
    const auto base=std::filesystem::path(root.path().toStdString());
    const auto content=base/"custom",executable=base/"installed";
    require(!projectionDataDirectory(content,executable),"Absent projection database selected");
    std::filesystem::create_directories(executable/"data/proj");
    {std::ofstream db(executable/"data/proj/proj.db");db<<"fixture";}
    require(projectionDataDirectory(content,executable)==executable/"data/proj","Custom data directory hid the installed database");
    std::filesystem::create_directories(content/"proj");
    {std::ofstream db(content/"proj/proj.db");db<<"override";}
    require(projectionDataDirectory(content,executable)==content/"proj","Explicit projection data override was ignored");
}
void decodedBudget() {
    Server server;QImage source(512,512,QImage::Format_RGB32);source.fill(Qt::blue);
    QBuffer buffer(&server.payload);buffer.open(QIODevice::WriteOnly);require(source.save(&buffer,"PNG"),"Budget PNG failed");
    auto provider=server.provider();provider.tilePixels=512;QTemporaryDir cache;MapTileClient client(cache.path());client.setProvider(provider);
    std::set<TileKey> wanted;for(int i=0;i<80;++i)wanted.insert({"fixture",15,25000+i,15000});
    client.setWantedTiles(wanted);waitFor([&]{return !client.pending();},10000);
    require(server.requests==64,"512-pixel viewport exceeded decoded memory budget");
    QTest::qWait(300);require(server.requests==64,"Decoded-cache eviction caused request churn");
}
void editor(const char* data,const QString& screenshot) {
    EditorWindow window(data);window.resize(1000,700);window.show();QApplication::processEvents();
    auto* retry=window.findChild<QAction*>("editorMapRetry");
    require(retry && !retry->isEnabled(),"Map retry enabled without a map");
    Server fixture;fixture.invalid=true;window.canvas()->setMapProviders({fixture.provider("osm-standard")});
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
    // Exhaust automatic attempts before recovering through the menu. A cancelled
    // startup viewport may leave earlier paths with fewer attempts.
    waitFor([&]{
        std::map<QByteArray,int> attempts;
        for(const auto& request:fixture.headers)++attempts[request.left(request.indexOf('\r'))];
        return std::any_of(attempts.begin(),attempts.end(),[](const auto& item){return item.second>=3;});
    },10000);
    QTest::qWait(100);
    require(window.findChild<QLabel*>("editorMapStatus")->isVisible(),"Failed-tile forcing did not show an error");
    const auto retryRevision=window.history().revision();
    const int failedRequests=fixture.requests;fixture.invalid=false;
    require(retry->isEnabled() && retry->text()=="Retry failed map tiles","English map retry action missing");
    retry->trigger();
    require(window.history().revision()==retryRevision,"Retry dirtied the project");
    waitFor([&]{return fixture.requests>0 && !window.findChild<QLabel*>("editorMapStatus")->isVisible();});
    require(fixture.requests>failedRequests,"Menu retry did not request failed tiles");
    const auto bounds=window.canvas()->scene()->itemsBoundingRect();
    require(window.findChild<QLabel*>("editorMapCredit")->isVisible(),"Map credit is hidden");
    if (!screenshot.isEmpty())require(window.grab().save(screenshot),"Map preview failed");
    window.canvas()->fitNetwork();QApplication::processEvents();
    for(const auto* item:window.canvas()->scene()->items())
        require(item->type()!=QGraphicsPixmapItem::Type,"Map tiles became scene items");
    const auto fitted=window.canvas()->mapToScene(window.canvas()->viewport()->rect()).boundingRect();
    require(fitted.width()<bounds.width()*1.6,"Map tiles enlarged Fit network bounds");
    window.findChild<QAction*>("editorToggleMap")->trigger();
    require(!retry->isEnabled(),"Retry enabled for a hidden map");
    const auto revision=window.history().revision();window.canvas()->centerOnGeographic({100.5,13.7});
    require(window.history().revision()==revision,"Navigation dirtied project");
    QTemporaryDir files;const auto path=files.filePath("map.traffic.json");window.saveFile(path);window.openFile(path);
    require(window.history().document().map->reference.anchorLongitude==100.5,"Open lost georeference");
    auto* language=window.findChild<QComboBox*>("editorLanguage");language->setCurrentIndex(1);
    require(!window.findChild<QAction*>("editorMapSettings")->text().isEmpty(),"Thai map action missing");
    require(retry->text()==QString::fromUtf8("ลองโหลดแผนที่อีกครั้ง"),"Thai map retry action missing");
    window.findChild<QAction*>("editorRemoveMap")->trigger();require(!window.history().document().map,"Remove failed");
    require(!retry->isEnabled(),"Retry enabled after map removal");
    window.findChild<QAction*>("editorUndo")->trigger();require(window.history().document().map.has_value(),"Undo lost map");
    window.findChild<QAction*>("editorRedo")->trigger();require(!window.history().document().map,"Redo failed");
    window.saveFile(path);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    useTestStorage(); // D320
    try {
        const auto mode=argc>1?QString::fromLocal8Bit(argv[1]):QString();
        if(mode.startsWith("--review-")) {
            if(mode=="--review-corrupt")corruptCache();
            else if(mode=="--review-png")widePng();
            else if(mode=="--review-fit")fitRefresh();
            else if(mode=="--review-wrap")dateline();
            else if(mode=="--review-anchor")largeAnchor();
            else if(mode=="--review-freshness")quotedFreshness();
            else if(mode=="--review-validator")validatorMismatch();
            else if(mode=="--review-policy")cachePolicy();
            else if(mode=="--review-bootstrap")projectionBootstrap();
            else if(mode=="--review-budget")decodedBudget();
            else throw std::runtime_error("Unknown review case");
            std::cout<<"review case passed\n";return 0;
        }
        tilesAndCache();cachePolicy();projectionBootstrap();decodedBudget();quotedFreshness();validatorMismatch();corruptCache();widePng();fitRefresh();dateline();largeAnchor();render();layerLifecycle();if(argc>1)editor(argv[1],argc>2?QString::fromLocal8Bit(argv[2]):QString());std::cout<<"map UI checks passed\n";return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
