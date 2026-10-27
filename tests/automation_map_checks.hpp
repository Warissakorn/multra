#pragma once
#include "../src/shell/editor_window.hpp"
#include "../src/editor/map_layer.hpp"
#include <QBuffer>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
// Local tile responses exercise production map state without contacting external providers.
inline void mapAutomationChecks(const std::filesystem::path& data) {
    using namespace multra;
    const auto demand=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    QTcpServer server;demand(server.listen(QHostAddress::LocalHost,0),"Map automation fixture failed to listen");
    QImage image(256,256,QImage::Format_RGB32);image.fill(Qt::green);
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);demand(image.save(&buffer,"PNG"),"Map fixture PNG failed");
    bool hold=true,fail=false;int requests=0;
    std::vector<QPointer<QTcpSocket>> held;
    const auto reply=[&](QTcpSocket* socket) {
        const auto body=fail?QByteArray("invalid PNG"):png;
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nCache-Control: max-age=3600\r\nContent-Length: "+
            QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
    };
    QObject::connect(&server,&QTcpServer::newConnection,&server,[&]{
        while(auto* socket=server.nextPendingConnection()) {
            auto pending=std::make_shared<QByteArray>();
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            QObject::connect(socket,&QTcpSocket::readyRead,&server,[&,socket,pending]{
                pending->append(socket->readAll());if(!pending->contains("\r\n\r\n"))return;
                pending->clear();++requests;if(hold)held.push_back(socket);else reply(socket);
            });
        }
    });
    EditorWindow w(data);w.resize(900,650);w.show();QTest::qWait(15);
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    MapProvider provider{"osm-standard","Local map fixture",QString("http://127.0.0.1:%1/%2/{z}/{x}/{y}.png").arg(server.serverPort()).arg(QUuid::createUuid().toString(QUuid::WithoutBraces)),
        "Local fixture",QUrl("https://example.org/credit"),0,19,256};
    w.canvas()->setMapProviders({provider});
    const auto call=[&](const QString& command,const QJsonObject& params=QJsonObject{}) {
        const auto state=w.automationRequest({{"command","status"}}).value("state").toObject();
        return w.automationRequest({{"command",command},{"params",params},{"documentId",state.value("documentId")},
            {"expectedRevision",state.value("revision")}});
    };
    const auto tiles=[&]{
        const auto r=w.automationRequest({{"command","map.status"}});demand(r.value("ok").toBool(),"Unguarded map status failed");
        return r.value("result").toObject().value("tiles").toObject();
    };
    const auto wait=[&](const std::function<bool()>& ready){
        QElapsedTimer timer;timer.start();while(!ready()&&timer.elapsed()<5000)QTest::qWait(5);
        demand(ready(),"Map automation condition timed out");
    };
    const auto get=w.automationRequest({{"command","map.get"}});
    demand(get.value("ok").toBool()&&get.value("result").toObject().value("settings").isNull()&&
        !get.value("result").toObject().value("providers").toArray().isEmpty(),"Map discovery failed");
    demand(tiles().value("status")==QJsonValue("disabled")&&!tiles().value("ready").toBool(),"Absent map reported ready");
    const auto original=w.history().document();
    demand(call("simulation.step",{{"steps",3}}).value("ok").toBool(),"Map run fixture failed");
    const QJsonObject settings{{"longitude",99.0},{"latitude",13.0}};
    auto changed=call("map.set",settings);
    demand(changed.value("ok").toBool()&&changed.value("result").toObject().value("changed").toBool(),"Map set failed without schema patch");
    demand(w.history().document().map&&w.history().document().map->reference.projectedCrs=="EPSG:32647"&&
        w.history().document().network==original.network&&!w.runState().scenario,"Map set changed network or kept stale run");
    wait([&]{return requests>0;});
    demand(!tiles().value("ready").toBool()&&tiles().value("pending").toInt()>0,"Held tiles reported ready");
    const auto revision=w.history().revision();const auto centre=w.canvas()->mapToScene(w.canvas()->viewport()->rect().center());
    demand(!call("map.set",settings).value("result").toObject().value("changed").toBool()&&w.history().revision()==revision&&
        w.canvas()->mapToScene(w.canvas()->viewport()->rect().center())==centre,"No-op map set changed revision or camera");
    for(const auto& invalid:std::vector<std::pair<QJsonObject,QString>>{
        {{{"longitude",181},{"latitude",13}},"params.longitude"},
        {{{"longitude","99"},{"latitude",13}},"params.longitude"},
        {{{"longitude",99},{"latitude",13},{"opacity",2}},"params.opacity"},
        {{{"longitude",99},{"latitude",13},{"providerId","missing"}},"params.providerId"},
        {{{"longitude",99},{"latitude",13},{"unexpected",1}},"params.unexpected"}}) {
        const auto r=call("map.set",invalid.first);const auto error=r.value("error").toObject();
        demand(!r.value("ok").toBool()&&error.value("path")==QJsonValue(invalid.second)&&error.contains("expected")&&
            w.history().revision()==revision,"Map validation lacked field detail or mutated work");
    }
    hold=false;for(const auto& socket:held)if(socket)reply(socket);held.clear();
    wait([&]{return tiles().value("ready").toBool();});
    auto t=tiles();demand(t.value("loaded")==t.value("wanted")&&t.value("failed").toInt()==0,"Ready tile counts do not reconcile");
    demand(w.canvas()->centerOnGeographic({99.001,13}),"Map pan fixture failed");
    demand(tiles().value("viewportPending").toBool()&&!tiles().value("ready").toBool(),"Pan reported stale viewport ready");
    const int beforeReads=requests;for(int i=0;i<20;++i)tiles();
    demand(requests==beforeReads&&w.history().revision()==revision,"Map status fetched tiles or changed history");
    wait([&]{return tiles().value("ready").toBool();});
    fail=true;
    demand(call("map.set",{{"longitude",100.0},{"latitude",13.0}}).value("ok").toBool(),"Map failure fixture set failed");
    wait([&]{return tiles().value("failed").toInt()>0;});
    demand(tiles().value("status")==QJsonValue("networkError")&&!tiles().value("ready").toBool(),"Tile error was hidden");
    const auto failedRevision=w.history().revision();
    fail=false;demand(call("map.retry").value("ok").toBool(),"Map retry failed");
    wait([&]{return tiles().value("ready").toBool();});
    demand(w.history().revision()==failedRevision,"Retry created an Undo entry");
    auto* visibility=w.findChild<QAction*>("editorToggleMap");demand(visibility,"Map visibility action missing");visibility->trigger();
    demand(tiles().value("status")==QJsonValue("disabled")&&!tiles().value("ready").toBool(),"Hidden map reported ready");
    const int beforeRetry=requests;call("map.retry");QTest::qWait(100);
    demand(requests==beforeRetry,"Hidden retry fetched tiles");visibility->trigger();
    w.canvas()->setMapProviders({});demand(tiles().value("status")==QJsonValue("unavailable"),"Missing provider did not report unavailable");
    w.canvas()->setMapProviders({provider});
    demand(call("map.remove").value("result").toObject().value("changed").toBool()&&!w.history().document().map,"Map remove failed");
    const auto removedRevision=w.history().revision();
    demand(!call("map.remove").value("result").toObject().value("changed").toBool()&&w.history().revision()==removedRevision,"Map removal was not idempotent");
    demand(call("history.undo").value("ok").toBool()&&w.history().document().map.has_value(),"Undo did not restore map");
    demand(call("history.redo").value("ok").toBool()&&!w.history().document().map,"Redo did not remove map");
    demand(call("file.new",{{"discardChanges",true}}).value("ok").toBool(),"Map fixture cleanup failed");
    std::cout<<"Map discovery, schema-free set/remove, field errors, tile loading/failure/retry and viewport readiness passed\n";
}
