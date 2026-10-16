#include "../src/editor/canvas.hpp"
#include "../src/editor/map_provider.hpp"
#include "../src/editor/map_layer.hpp"
#include "../src/project/document.hpp"
#include <QApplication>
#include <QLabel>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTest>
#include <cmath>
#include <iostream>
using namespace trafficsim;
// UI slice e1 (D197): the canvas HUD shows a 1/2/5 scale bar, px/m and, only when the map
// georeference supports it, a north arrow. It never takes a gesture and keeps clear of the map corners.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(120);QApplication::processEvents();}
QWidget* hud(EditorCanvas& canvas){auto* h=canvas.viewport()->findChild<QWidget*>("editorCanvasHud");require(h,"Canvas HUD missing");return h;}
bool niceStep(double metres) {
    const double decade=std::pow(10.0,std::floor(std::log10(metres)+1e-9));
    for(double m:{1.0,2.0,5.0})if(std::abs(metres-m*decade)<1e-6*decade)return true;
    return false;
}
// A provider at a closed local port: attribution text without any network fetch.
MapProvider offline(){return {"fixture","Fixture","http://127.0.0.1:9/{z}/{x}/{y}.png","Fixture credit",QUrl("https://example.org/credit"),0,19,256};}
double north(EditorCanvas& canvas){settle();const auto v=hud(canvas)->property("northDegrees");require(v.isValid(),"North arrow missing");return v.toDouble();}
double wrap(double degrees){while(degrees>180)degrees-=360;while(degrees<=-180)degrees+=360;return degrees;}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        EditorCanvas canvas;canvas.resize(900,600);canvas.setMapProviders({offline()});
        canvas.setMapStatusText([](MapStatus s){return s==MapStatus::ready||s==MapStatus::disabled?QString():QStringLiteral("Map tiles unavailable");});
        canvas.setHudUnits(QStringLiteral("%1 m"),QStringLiteral("%1 px/m"));
        ProjectDocument document;document.network.links.push_back({"road",{{-50,0},{50,0}},{{"lane",3.5}}});
        canvas.setDocument(&document);canvas.show();settle();
        auto* h=hud(canvas);
        // Scale: a 1/2/5 step, 64-160 px, whatever the rotation or mirroring.
        for(double s:{0.3,4.0,37.0,90.0})for(const auto& extra:{QTransform(),QTransform().rotate(30),QTransform::fromScale(-1,1)}) {
            canvas.setTransform(extra*QTransform::fromScale(s,-s));settle();
            const double metres=h->property("scaleMetres").toDouble(),pixels=h->property("scalePixels").toDouble();
            require(niceStep(metres),"Scale bar is not a 1/2/5 step");
            require(std::abs(pixels-metres*s)<1e-6*pixels+1e-9&&pixels>=64&&pixels<=160,"Scale bar length wrong for the zoom");
        }
        canvas.setTransform(QTransform::fromScale(4,-4));settle();
        require(!h->property("northDegrees").isValid(),"North shown without a georeference");
        // North from the existing georeference: rotation, convergence and the view's own rotation.
        document.map=MapSettings{};document.map->providerId="fixture";
        document.map->reference=GeoReference{"EPSG:32647",99,13,0,0,0};canvas.setDocument(&document);
        const double straight=north(canvas);
        require(std::abs(wrap(straight))<0.2,"North on the central meridian is not up");
        document.map->reference.rotationDegrees=30;canvas.setDocument(&document);const double plus=wrap(north(canvas));
        document.map->reference.rotationDegrees=-30;canvas.setDocument(&document);const double minus=wrap(north(canvas));
        require(std::abs(std::abs(plus)-30)<0.2&&std::abs(plus+minus)<0.2,"Georeference rotation does not turn north");
        document.map->reference=GeoReference{"EPSG:32647",101.5,13,0,0,0};canvas.setDocument(&document);
        const double gamma=2.5*std::sin(13*M_PI/180); // ≈ Δλ·sinφ, the grid convergence
        require(std::abs(std::abs(wrap(north(canvas)))-gamma)<0.15,"North ignores grid convergence");
        document.map->reference=GeoReference{"EPSG:32647",99,13,0,0,0};canvas.setDocument(&document);
        canvas.setTransform(QTransform().rotate(40)*QTransform::fromScale(4,-4));
        require(std::abs(std::abs(wrap(north(canvas)))-40)<0.2,"A rotated view did not turn north");
        canvas.setTransform(QTransform::fromScale(4,-4));settle();
        // E19 (D198): any georeference gives north, with no tile provider and with the map hidden.
        document.map->providerId="missing";canvas.setDocument(&document);
        require(std::abs(wrap(north(canvas)))<0.2,"North needs a tile provider");
        document.map->providerId="fixture";canvas.setDocument(&document);canvas.setMapVisible(false);
        require(std::abs(wrap(north(canvas)))<0.2,"North hidden with the map");
        canvas.setMapVisible(true);settle();
        // The HUD takes no gesture: the viewport sees the click.
        const auto centre=h->geometry().center();
        require(canvas.viewport()->childAt(centre)!=h,"HUD intercepts the pointer");
        // Clear of the map status (top-left) and the credit (bottom-left), inside the viewport.
        auto* credit=canvas.viewport()->findChild<QLabel*>("editorMapCredit");auto* status=canvas.viewport()->findChild<QLabel*>("editorMapStatus");
        for(const QSize size:{QSize(900,600),QSize(560,400),QSize(420,300),QSize(260,300)}) {
            canvas.resize(size);settle();
            require(credit->isVisible(),"Map credit hidden");
            require(canvas.viewport()->rect().contains(h->geometry()),"HUD outside the viewport");
            require(!h->geometry().intersects(credit->geometry()),"HUD covers the map credit");
            require(!status->isVisible()||!h->geometry().intersects(status->geometry()),"HUD covers the map status");
        }
        // The viewport alone can change size (a scroll bar comes or goes): the HUD follows it.
        canvas.resize(900,600);settle();
        canvas.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);settle();
        require(h->geometry().right()==canvas.viewport()->width()-1-8,"HUD kept a stale viewport width");
        canvas.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);settle();
        // Scrolling moves a viewport's child widgets with the content; the HUD and map corners stay put
        // at once, not after the map timer.
        {
            const auto before=h->pos(),creditAt=credit->pos();
            canvas.horizontalScrollBar()->setValue(canvas.horizontalScrollBar()->value()+150);QApplication::processEvents();
            require(h->pos()==before&&credit->pos()==creditAt,"Scrolling dragged the corner overlays");
        }
        if(argc>1) {
            canvas.resize(900,600);document.map->reference.rotationDegrees=30;canvas.setDocument(&document);settle();
            require(canvas.grab().save(QString::fromUtf8(argv[1])+"-rotated.png"),"Screenshot failed");
        }
        std::cout<<"Canvas HUD: scale, north, no hits and corners passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
