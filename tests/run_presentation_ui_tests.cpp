#include "../src/shell/editor_window.hpp"
#include "../src/editor/run_vehicle_item.hpp"
#include "../src/editor/road_crossbar.hpp"
#include <QAction>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QStandardPaths>
#include <QTest>
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
        if(argc>2) {
            canvas->setTransform(QTransform::fromScale(14,-14));canvas->centerOn(0,0);settle();
            auto image=canvas->viewport()->grab().toImage().convertToFormat(QImage::Format_Grayscale8);
            require(image.save(QString::fromUtf8(argv[2])+"-grayscale.png"),"Screenshot failed");
        }
        std::cout<<"Run presentation: casings, signal cues and unchanged hits passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
