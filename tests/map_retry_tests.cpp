#include "../src/editor/map_layer.hpp"
#include <QApplication>
#include <QBuffer>
#include <QElapsedTimer>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace multra;
namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
void waitFor(const std::function<bool()>& predicate,int timeout=5000) {
    QElapsedTimer timer;timer.start();
    while(!predicate() && timer.elapsed()<timeout)QTest::qWait(10);
    require(predicate(),"Timed out waiting for map retry fixture");
}
QByteArray png() {
    QImage image(256,256,QImage::Format_RGB32);image.fill(Qt::green);
    QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);
    require(image.save(&buffer,"PNG"),"Retry fixture PNG failed");return bytes;
}
class Server {
public:
    enum class Response { image, invalid, limited, held };
    QTcpServer server;
    QElapsedTimer timer;
    Response fallback{Response::image};
    std::map<QByteArray,Response> routes;
    std::map<QByteArray,int> requests;
    std::map<QByteArray,std::vector<qint64>> received;
    std::vector<QPointer<QTcpSocket>> held;
    const QByteArray image{png()};
    Server() {
        timer.start();require(server.listen(QHostAddress::LocalHost,0),"Retry fixture server did not listen");
        QObject::connect(&server,&QTcpServer::newConnection,&server,[this]{
            while(server.hasPendingConnections()) {
                auto* socket=server.nextPendingConnection();auto buffer=std::make_shared<QByteArray>();
                QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
                QObject::connect(socket,&QTcpSocket::readyRead,&server,[this,socket,buffer]{
                    buffer->append(socket->readAll());if(!buffer->contains("\r\n\r\n"))return;
                    const auto path=buffer->split(' ').value(1);buffer->clear();
                    ++requests[path];received[path].push_back(timer.elapsed());
                    const auto route=routes.find(path);const auto response=route==routes.end()?fallback:route->second;
                    if(response==Response::held)held.emplace_back(socket);else reply(socket,response);
                });
            }
        });
    }
    void reply(QTcpSocket* socket,Response response) const {
        if(response==Response::limited)socket->write(
            "HTTP/1.1 429 Too Many Requests\r\nRetry-After: 2\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        else {
            const auto body=response==Response::invalid?QByteArray("invalid PNG"):image;
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nCache-Control: max-age=3600\r\n"
                "Content-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);
        }
        socket->disconnectFromHost();
    }
    void releaseHeld() {
        for(const auto& socket:held)if(socket && socket->state()==QAbstractSocket::ConnectedState)reply(socket,Response::image);
        held.clear();
    }
    int count() const {int total=0;for(const auto& item:requests)total+=item.second;return total;}
    MapProvider provider() const {
        return {"fixture","Retry fixture",QString("http://127.0.0.1:%1/{z}/{x}/{y}.png").arg(server.serverPort()),
            "Fixture credit",QUrl("https://example.org/credit"),0,19,256};
    }
};
const TileKey cachedKey{"fixture",15,25000,15000},failedKey{"fixture",15,25001,15000};
const QByteArray cachedPath{"/15/25000/15000.png"},failedPath{"/15/25001/15000.png"};

void terminalRecovery() {
    Server server;server.routes[failedPath]=Server::Response::invalid;QTemporaryDir cache;
    require(cache.isValid(),"Retry cache directory failed");MapTileClient client(cache.path());client.setProvider(server.provider());
    client.setWantedTiles({cachedKey,failedKey});
    waitFor([&]{return server.requests[failedPath]==3 && client.pending()==0;},8000);
    require(client.hasFailures() && client.image(failedKey).isNull(),"Terminal failure forcing did not work");
    require(!client.image(cachedKey).isNull() && server.requests[cachedPath]==1,"Successful tile fixture did not load once");
    server.routes[failedPath]=Server::Response::image;
    QTest::qWait(350);require(server.requests[failedPath]==3,"Terminal failure retried without an explicit request");
    client.retryFailedTiles();client.retryFailedTiles();
    waitFor([&]{return !client.image(failedKey).isNull();});
    require(!client.hasFailures() && client.pending()==0,"Explicit retry retained failure state");
    require(server.requests[failedPath]==4,"Explicit retry did not recover exactly once");
    require(server.requests[cachedPath]==1,"Retry downloaded an already cached tile");
    client.retryFailedTiles();QTest::qWait(100);require(server.count()==5,"Retry without failures fetched tiles");
}
void providerCooldown() {
    Server server;server.routes[failedPath]=Server::Response::limited;QTemporaryDir cache;
    require(cache.isValid(),"Cooldown cache directory failed");MapTileClient client(cache.path());client.setProvider(server.provider());
    client.setWantedTiles({failedKey});waitFor([&]{return client.hasFailures();});
    require(server.requests[failedPath]==1 && client.image(failedKey).isNull(),"429 forcing did not work");
    server.routes[failedPath]=Server::Response::image;client.retryFailedTiles();client.retryFailedTiles();
    QTest::qWait(300);require(server.requests[failedPath]==1,"Explicit retry bypassed Retry-After");
    waitFor([&]{return !client.image(failedKey).isNull();});
    require(server.requests[failedPath]==2 && !client.hasFailures(),"Cooldown retry did not recover once");
    require(server.received[failedPath][1]-server.received[failedPath][0]>=1900,"Provider cooldown was shortened");
}
void activeRequests() {
    Server server;server.routes[cachedPath]=Server::Response::held;server.routes[failedPath]=Server::Response::invalid;
    QTemporaryDir cache;require(cache.isValid(),"Active request cache directory failed");
    MapTileClient client(cache.path());client.setProvider(server.provider());client.setWantedTiles({cachedKey,failedKey});
    waitFor([&]{return server.requests[cachedPath]==1 && client.hasFailures();});
    require(client.image(cachedKey).isNull() && server.requests[failedPath]==1,"Active/failure forcing did not work");
    server.routes[failedPath]=Server::Response::held;client.retryFailedTiles();client.retryFailedTiles();
    waitFor([&]{return server.requests[failedPath]==2;});QTest::qWait(100);
    require(server.requests[cachedPath]==1 && server.requests[failedPath]==2,"Retry duplicated an active request");
    server.releaseHeld();waitFor([&]{return !client.image(cachedKey).isNull() && !client.image(failedKey).isNull();});
    require(client.pending()==0 && !client.hasFailures(),"Active request recovery retained pending work");
}
void layerLifecycle() {
    Server server;server.fallback=Server::Response::invalid;QTemporaryDir cache;
    require(cache.isValid(),"Layer retry cache directory failed");MapLayer layer(cache.path());layer.setProviders({server.provider()});
    MapSettings settings;settings.providerId="fixture";layer.setSettings(settings);
    const QPolygonF view{{-1,1},{1,1},{1,-1},{-1,-1}};layer.setView(view,2);
    const auto wanted=layer.wantedTiles().size();require(wanted>0,"Layer fixture wanted no tiles");
    waitFor([&]{return server.count()>=static_cast<int>(wanted) && layer.status()==MapStatus::networkError;});
    require(layer.status()==MapStatus::networkError,"Layer failure forcing did not work");
    server.fallback=Server::Response::image;layer.retryFailedTiles();
    waitFor([&]{return layer.status()==MapStatus::ready;});
    require(layer.wantedTiles().size()==wanted,"Layer retry changed the visible tile set");
    layer.setVisible(false);QTest::qWait(100);const int before=server.count();
    layer.retryFailedTiles();layer.setView(view,2);QTest::qWait(300);
    require(server.count()==before && layer.wantedTiles().empty(),"Hidden layer retry made requests");
    require(layer.status()==MapStatus::disabled,"Hidden layer retry changed visibility");
    layer.setSettings({});layer.setVisible(true);layer.retryFailedTiles();QTest::qWait(100);
    require(server.count()==before && layer.status()==MapStatus::disabled,"Removed layer retry made requests");
    layer.setProviders({});layer.setSettings(settings);
    require(layer.status()==MapStatus::unavailable,"Unavailable-provider forcing did not work");
    layer.retryFailedTiles();QTest::qWait(100);
    require(server.count()==before && layer.status()==MapStatus::unavailable,"Unavailable layer retry made requests");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        terminalRecovery();providerCooldown();activeRequests();layerLifecycle();
        std::cout<<"map retry checks passed\n";return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
