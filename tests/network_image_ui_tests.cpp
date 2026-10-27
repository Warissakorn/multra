#include "../src/shell/editor_window.hpp"
#include "../src/shell/image_export.hpp"
#include "../src/shell/editor_storage.hpp"
#include "../src/editor/canvas_image.hpp"
#include "../src/editor/map_layer.hpp"
#include "ui_test_storage.hpp"
#include <nlohmann/json.hpp>
#include <QApplication>
#include <QAction>
#include <QBuffer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QFile>
#include <QScrollBar>
#include <QPainter>
#include <QTest>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <iostream>
using namespace multra;
namespace {
void require(bool b,const char* text){if(!b)throw std::runtime_error(text);}
QByteArray png(QImage image) {QByteArray out;QBuffer b(&out);b.open(QIODevice::WriteOnly);require(image.save(&b,"PNG"),"Fixture encoding failed");return out;}
bool marked(const QImage& image) {
    for(int y=0;y<image.height();y+=3)for(int x=0;x<image.width();x+=3)
        if(image.pixelColor(x,y)!=QColor(Qt::white))return true;
    return false;
}
void renderCases(const std::filesystem::path& data,const QString& directory) {
    EditorWindow w(data);w.resize(1200,800);w.show();QApplication::processEvents();
    w.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));
    auto* c=w.canvas();c->select(w.history().document().network.links.front().id);c->fitNetwork();
    const auto document=documentJson(w.history().document());const auto camera=c->transform();const auto selection=c->selection();
    const auto h=c->horizontalScrollBar()->value(),v=c->verticalScrollBar()->value();
    CanvasImageOptions options;options.pixels={640,480};options.background=false;
    const auto all=c->renderImage(options);require(all.size()==options.pixels && marked(all),"Network render is blank/wrong size");
    options.scope=CanvasImageScope::view;require(marked(c->renderImage(options)),"View-area render is blank");
    writeNetworkPng(directory+"/ภาพเครือข่าย.png",*c,options,{directory+"/not-created-recovery.json"});
    const QImage saved(directory+"/ภาพเครือข่าย.png");require(saved.size()==options.pixels,"PNG cannot be reopened");
    require(documentJson(w.history().document())==document && c->transform()==camera && c->selection()==selection &&
        c->horizontalScrollBar()->value()==h && c->verticalScrollBar()->value()==v,"Export changed document/camera/selection");
    options.scope=CanvasImageScope::network;c->scale(2,2);require(c->transform()!=camera,"Zoom forcing failed");
    require(c->renderImage(options)==all,"Whole-network output depends on live camera zoom");
    options.scope=CanvasImageScope::view;
    const auto file=directory+"/keep.png";{QFile f(file);require(f.open(QIODevice::WriteOnly)&&f.write("keep")==4,"Sentinel write failed");}
    options.pixels={8192,8192};require(static_cast<qint64>(options.pixels.width())*options.pixels.height()>32000000,"Size forcing failed");
    bool failed=false;try{writeNetworkPng(file,*c,options);}catch(const std::exception&){failed=true;}
    QFile preserved(file);require(preserved.open(QIODevice::ReadOnly),"Sentinel missing");require(failed && preserved.readAll()=="keep","Oversize export replaced destination");
    options.pixels={640,480};failed=false;try{writeNetworkPng(file,*c,options,{file});}catch(const std::exception&){failed=true;}
    require(failed,"Protected destination accepted");
    ProjectDocument blank;EditorCanvas preview;preview.setDocument(&blank);
    require(!marked(preview.renderImage(options)),"Empty draft is not a blank image");
    QImage background(16,16,QImage::Format_RGB32);background.fill(Qt::blue);
    {QPainter p(&background);p.fillRect(0,0,16,8,Qt::red);}
    blank.background.pngBase64=std::make_shared<const std::string>(png(background).toBase64().toStdString());
    blank.background.x=-8;blank.background.y=8;blank.background.metresPerPixel=1;blank.background.opacity=1;
    preview.setDocument(&blank);options.scope=CanvasImageScope::network;options.background=true;
    const auto withBackground=preview.renderImage(options);
    require(withBackground.pixelColor(320,180).red()>200 && withBackground.pixelColor(320,300).blue()>200,"Image orientation/background incorrect");
    options.background=false;require(!marked(preview.renderImage(options)),"Background exclusion failed");
}
void cachedMapCase() {
    QTcpServer server;require(server.listen(QHostAddress::LocalHost,0),"Tile server did not listen");int requests=0;
    QImage tile(256,256,QImage::Format_RGB32);tile.fill(Qt::green);const auto payload=png(tile);
    QObject::connect(&server,&QTcpServer::newConnection,[&]{
        while(auto* socket=server.nextPendingConnection()) {
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{
                auto request=socket->property("request").toByteArray()+socket->readAll();socket->setProperty("request",request);
                if(!request.contains("\r\n\r\n"))return;
                ++requests;socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+QByteArray::number(payload.size())+"\r\n\r\n"+payload);
                socket->disconnectFromHost();
            });
        }
    });
    EditorCanvas canvas;canvas.resize(640,480);
    const auto id=std::string("png-export-")+std::to_string(server.serverPort());
    canvas.setMapProviders({{id,"Fixture",QString("http://127.0.0.1:%1/{z}/{x}/{y}.png").arg(server.serverPort()),
        "PNG fixture credit",QUrl("https://example.org/png-credit"),0,19,256}});
    ProjectDocument doc;doc.map=MapSettings{};doc.map->providerId=id;doc.map->opacity=1;
    doc.network.links.push_back({"road",{{-10,0},{10,0}},{{"lane",3.5}}});
    canvas.setDocument(&doc);canvas.show();QElapsedTimer clock;clock.start();
    while(clock.elapsed()<5000 && (canvas.mapLoadState().status!=MapStatus::ready || canvas.mapLoadState().viewportPending || !requests)) {
        QApplication::processEvents();QTest::qWait(10);
    }
    require(requests>0 && canvas.mapLoadState().loaded>0 && !canvas.mapLoadState().pending,"Cached map forcing failed");
    const auto before=requests;const auto camera=canvas.transform();
    CanvasImageOptions options;options.pixels={640,480};options.scope=CanvasImageScope::view;
    const auto image=canvas.renderImage(options);
    require(image.text("Map attribution").contains("PNG fixture credit"),"Map attribution missing");
    require(image.pixelColor(200,100).green()>200 && image.pixelColor(200,100).red()<50,"Loaded map was not painted");
    QTest::qWait(120);require(requests==before && canvas.transform()==camera,"Export requested tiles or moved camera");
    options.background=false;require(canvas.renderImage(options).text("Map attribution").isEmpty(),"Excluded map retains credit");
}
void dialogCases(const std::filesystem::path& data,const QString& directory) {
    EditorWindow w(data);auto* action=w.findChild<QAction*>("editorExportImage");require(action && action->isEnabled(),"PNG menu needs no Run");
    const auto before=documentJson(w.history().document());bool cancelled=false;
    QTimer::singleShot(25,[&]{if(auto* d=w.findChild<QDialog*>("editorImageExportDialog")){cancelled=true;d->reject();}});
    action->trigger();require(cancelled,"Options cancel fixture failed");
    bool optionsSeen=false,fileSeen=false;const auto path=directory+"/dialog.png";
    QTimer timer;timer.setInterval(25);QObject::connect(&timer,&QTimer::timeout,[&]{
        if(auto* d=w.findChild<QDialog*>("editorImageExportDialog");d && d->isVisible()) {
            optionsSeen=true;d->findChild<QSpinBox*>("editorImageWidth")->setValue(640);
            d->findChild<QSpinBox*>("editorImageHeight")->setValue(480);
            d->findChild<QCheckBox*>("editorImageBackground")->setChecked(false);
            d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
        } else if(auto* f=w.findChild<QFileDialog*>("editorImageFileDialog");f && f->isVisible()) {
            fileSeen=true;f->findChild<QLineEdit*>("fileNameEdit")->setText(path);QMetaObject::invokeMethod(f,"accept");
        }
    });timer.start();action->trigger();timer.stop();
    require(optionsSeen && fileSeen && QImage(path).size()==QSize(640,480),"PNG menu did not save chosen dimensions");
    require(documentJson(w.history().document())==before && !w.history().dirty(),"Dialog export changed draft");
    w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(1);
    require(action->text().contains(QString::fromUtf8("ส่งออกภาพ")),"Thai action missing");
}
}
int main(int argc,char** argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);useTestStorage();
    try {
        require(argc==2,"Expected data path");QTemporaryDir dir;require(dir.isValid(),"Temporary directory failed");
        renderCases(std::filesystem::path(argv[1]),dir.path());dialogCases(std::filesystem::path(argv[1]),dir.path());cachedMapCase();
        std::cout<<"PNG dimensions, orientation, scopes, background, state and dialogs passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
