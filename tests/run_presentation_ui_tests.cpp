#include "../src/shell/editor_window.hpp"
#include "../src/editor/run_vehicle_item.hpp"
#include "../src/editor/road_crossbar.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QBuffer>
#include <QTemporaryDir>
#include <fstream>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineEdit>
#include <QPainter>
#include <QStandardPaths>
#include <QTest>
#include <algorithm>
#include <filesystem>
#include <iostream>
using namespace trafficsim;
// UI slice e1b (D199): run vehicles carry a neutral casing and run signal bars a shape per state,
// both drawn only: hit regions, poses and crossbars are what they were.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
double luminance(QRgb c){return (0.2126*qRed(c)+0.7152*qGreen(c)+0.0722*qBlue(c))/255;}
QImage render(QGraphicsItem* item,QColor background,double scale) {
    QGraphicsScene scene;scene.addItem(item);
    const auto bounds=item->sceneBoundingRect().adjusted(-2/scale,-2/scale,2/scale,2/scale);
    QImage image(QSize(int(bounds.width()*scale),int(bounds.height()*scale)),QImage::Format_RGB32);image.fill(background);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);scene.render(&p,QRectF(image.rect()),bounds);p.end();
    scene.removeItem(item);
    return image;
}
QSet<int> ink(const QImage& image,QRgb background) {
    QSet<int> out;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)
        if(std::abs(luminance(image.pixel(x,y))-luminance(background))>0.05)out.insert(y*image.width()+x);
    return out;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        // A vehicle the colour of its background still shows its edge; its hit shape is the bare path.
        QPainterPath body;body.addRect(-4.5,-0.9,4.5,1.8);
        for(const QColor fill:{QColor("#202020"),QColor("#f0f0f0")}) {
            auto* vehicle=new RunVehicleItem(body,fill,20);
            QGraphicsPathItem plain(body);plain.setPen(Qt::NoPen);
            require(vehicle->shape()==plain.shape(),"Casing changed the vehicle's hit shape");
            const auto image=render(vehicle,fill,20);
            double edge=0;for(int x=0;x<image.width();++x)edge=std::max(edge,std::abs(luminance(image.pixel(x,image.height()/2))-luminance(fill.rgb())));
            require(edge>=0.3,"Vehicle vanishes on a background of its own colour");
            delete vehicle;
        }
        // Red, amber and green bars differ in shape, not only hue; their hit shape is one.
        const RoadCrossbar bar{{0,-3.5},{0,3.5},0};
        const QColor background("#808080");
        std::vector<QSet<int>> masks;std::vector<QPainterPath> shapes;
        for(const auto cue:{SignalCue::stop,SignalCue::caution,SignalCue::go}) {
            auto* item=new RoadCrossbarItem(bar,Qt::black,20,false,cue);
            shapes.push_back(item->shape());masks.push_back(ink(render(item,background,20),background.rgb()));delete item;
        }
        require(shapes[0]==shapes[1]&&shapes[1]==shapes[2],"A signal cue changed the crossbar's hit shape");
        require(masks[0]!=masks[1]&&masks[1]!=masks[2]&&masks[0]!=masks[2],"Signal states share one shape");
        require(masks[0].contains(masks[1])&&masks[0].contains(masks[2]),"Red is not the full bar");
        // A real run: every vehicle is cased and a click on one still reaches the run.
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        for(int i=0;i<300;++i)window.findChild<QAction*>("editorStep")->trigger();
        settle();
        auto* canvas=window.canvas();int vehicles=0;QGraphicsItem* first=nullptr;
        for(auto* item:canvas->scene()->items())if(item->data(0).toString()==QStringLiteral("run-vehicle")) {
            require(dynamic_cast<RunVehicleItem*>(item),"A run vehicle is not cased");
            ++vehicles;if(!first)first=item;
        }
        require(vehicles>0,"Run fixture has no vehicles");
        std::optional<std::uint64_t> clicked;canvas->runVehicleClicked=[&](std::uint64_t id){clicked=id;};
        canvas->centerOn(first);settle();
        const auto at=canvas->mapFromScene(first->mapToScene(first->shape().boundingRect().center()));
        QTest::mouseClick(canvas->viewport(),Qt::LeftButton,{},at);settle();
        require(clicked&&*clicked==first->data(2).toULongLong(),"A click on a cased vehicle missed it");
        // D281: during a run the static network below the vehicles is painted from the cached
        // background, pixel for pixel what live items paint, and a selection still repaints it.
        const auto hidden=[&]{int n=0;for(auto* item:canvas->scene()->items())if(!item->isVisible())++n;return n;};
        require(hidden()>0,"The forcing: a run hides the static layer it bakes");
        const auto grab=[&]{settle();return canvas->viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);};
        // The cache is a premultiplied pixmap, so antialiased edges may round a level or two apart
        // and a live casing over them a little more (≤24 levels on ≈40 px measured); a stale or
        // missing layer differs far more than this.
        const auto same=[](const QImage& x,const QImage& y){
            if(x.size()!=y.size())return false;
            long differing=0;
            for(int r=0;r<x.height();++r)for(int c=0;c<x.width();++c) {
                const auto p=x.pixel(c,r),q=y.pixel(c,r);
                const int d=std::max({std::abs(qRed(p)-qRed(q)),std::abs(qGreen(p)-qGreen(q)),std::abs(qBlue(p)-qBlue(q))});
                if(d>32)return false;
                if(d>0)++differing;
            }
            return differing*100<long(x.width())*x.height();
        };
        const auto baked=grab();
        canvas->setRunLayerCache(false);require(hidden()==0,"Without the cache every item is live");
        require(same(grab(),baked),"The cached static layer paints differently from live items");
        canvas->setRunLayerCache(true);require(same(grab(),baked),"Turning the cache back on changed the picture");
        const auto link=window.history().document().network.links.front().id;
        canvas->select(link);const auto selected=grab();
        require(!same(selected,baked),"The forcing: selecting a Link changes the picture");
        canvas->setRunLayerCache(false);require(same(grab(),selected),"A selection during a run left the cached layer stale");
        canvas->setRunLayerCache(true);
        window.findChild<QLineEdit*>("editorSeed")->setText("43");settle(); // ends the run (clearRun)
        require(hidden()==0,"Leaving the run leaves static items hidden");
        // A half-transparent background image is baked with its opacity, as the scene paints it.
        {
            QTemporaryDir directory;require(directory.isValid(),"No temporary directory");
            auto document=window.history().document();QImage photo(400,400,QImage::Format_RGB32);photo.fill(QColor("#203040"));
            QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);require(photo.save(&buffer,"PNG"),"Image fixture");
            document.background.pngBase64=std::make_shared<const std::string>(png.toBase64().toStdString());
            document.background.x=-200;document.background.y=200;document.background.opacity=.5;
            const auto file=directory.filePath("photo.traffic.json");
            std::ofstream(file.toStdString())<<documentJson(document).dump(1);
            window.openFile(file);settle();
            for(int i=0;i<50;++i)window.findChild<QAction*>("editorStep")->trigger();
            const auto photoBaked=grab();
            canvas->setRunLayerCache(false);
            require(same(grab(),photoBaked),"A background image's opacity is lost in the cached layer");
            canvas->setRunLayerCache(true);
        }
        if(argc>2) {
            canvas->setTransform(QTransform::fromScale(14,-14));canvas->centerOn(0,0);settle();
            auto image=canvas->viewport()->grab().toImage().convertToFormat(QImage::Format_Grayscale8);
            require(image.save(QString::fromUtf8(argv[2])+"-grayscale.png"),"Screenshot failed");
        }
        std::cout<<"Run presentation: casings, signal cues and unchanged hits passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
