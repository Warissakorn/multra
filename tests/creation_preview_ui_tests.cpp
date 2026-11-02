#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QSpinBox>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
// D433: Link, Connector and StreetSegment creation gestures preview the object they would create
// (a filled road surface), not a bare line, and leave nothing behind when cancelled.
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok){std::cerr<<message<<"\n";throw std::runtime_error(message);}}
void settle(){for(int n=0;n<3;++n){QApplication::processEvents();QTest::qWait(10);}}
QPoint pixel(EditorCanvas& c,Point p){const auto q=c.mapFromScene(p.x,p.y);require(c.viewport()->rect().contains(q),"Point outside the viewport");return q;}
// The preview surface's area in square metres, 0 when none is drawn.
double previewArea(EditorCanvas& c){
    double area=0;
    for(auto* item:c.scene()->items())if(item->data(0).toString()=="creation-preview")
        if(auto* path=dynamic_cast<QGraphicsPathItem*>(item);path&&path->brush().style()!=Qt::NoBrush) {
            const auto poly=path->path().toFillPolygon();
            for(int i=0;i<poly.size();++i){const auto a=poly[i],b=poly[(i+1)%poly.size()];area+=(a.x()*b.y()-b.x()*a.y())/2;}
        }
    return std::abs(area);
}
void shot(EditorWindow& w,const char* name){if(const auto dir=qEnvironmentVariable("MULTRA_EVIDENCE");!dir.isEmpty())w.grab().save(dir+"/"+name+".png");}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);require(argc>1,"Usage: <data>");useTestStorage();
    const std::filesystem::path data=argv[1];
    EditorWindow w(data,"en",nullptr,true);w.resize(1366,860);w.show();
    QTemporaryDir temp;require(temp.isValid(),"No temporary directory");const auto file=temp.filePath("preview.traffic.json");
    require(QFile::copy(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()),file),"Fixture copy failed");
    QFile::setPermissions(file,QFileDevice::ReadOwner|QFileDevice::WriteOwner);
    w.openFile(file);settle();
    auto& c=*w.canvas();c.fitNetwork();settle();
    const auto& network=[&]()->const Network&{return w.history().document().network;};
    const auto revision=w.history().revision();
    // Link: 3 lanes of 3.5 m along a 40 m draft is about 420 m^2 of road.
    c.setNewLinkLanes(3,3.5);c.setTool(EditorCanvas::Tool::draw);
    const auto centre=c.mapToScene(c.viewport()->rect().center());
    const Point a{centre.x()-60,centre.y()+70},b{centre.x()-20,centre.y()+70};
    QTest::mousePress(c.viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,a));QTest::mouseMove(c.viewport(),pixel(c,b));settle();
    const double link=previewArea(c);std::cout<<"Link preview "<<link<<" m2"<<std::endl;
    require(link>300&&link<550,"Link preview is not the new Link's road surface");shot(w,"preview-link");
    QTest::keyClick(&c,Qt::Key_Escape);settle();
    require(previewArea(c)==0&&w.history().revision()==revision,"Cancelling left a preview or changed the document");
    // Connector: drag from the end of one Link onto the start of another.
    const Link* in=nullptr;const Link* out=nullptr;
    for(const auto& l:network().links){if(!in&&l.geometry.size()>=2)in=&l;}
    for(const auto& l:network().links)if(&l!=in&&l.geometry.size()>=2&&!out) {
        const auto d=std::hypot(l.geometry.front().x-in->geometry.back().x,l.geometry.front().y-in->geometry.back().y);
        if(d>5&&d<80)out=&l;
    }
    require(in&&out,"No Link pair for the Connector fixture");
    const auto along=[](const std::vector<Point>& g,bool end){const auto p=end?g.back():g.front(),q=end?g[g.size()-2]:g[1];
        const double d=std::hypot(q.x-p.x,q.y-p.y);return Point{p.x+(q.x-p.x)*2/d,p.y+(q.y-p.y)*2/d};};
    QTest::mousePress(c.viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,along(in->geometry,true)));
    QTest::mouseMove(c.viewport(),pixel(c,along(out->geometry,false)));settle();
    const double connector=previewArea(c);std::cout<<"Connector preview "<<connector<<" m2"<<std::endl;
    require(connector>5,"Connector preview has no road surface");shot(w,"preview-connector");
    QTest::keyClick(&c,Qt::Key_Escape);settle();require(previewArea(c)==0,"Connector preview outlived cancel");
    // StreetSegment: with a 2+2 template, the draft centreline previews both directions' Links.
    QTimer::singleShot(0,[]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());require(d,"No template dialog");
        d->findChild<QLineEdit*>("editorCrossSectionName")->setText("Two by two");
        d->findChild<QComboBox*>("editorCrossSectionDirections")->setCurrentIndex(1);
        d->findChild<QSpinBox*>("editorCrossSectionLanesForward")->setValue(2);
        d->findChild<QSpinBox*>("editorCrossSectionLanesBackward")->setValue(2);
        d->findChild<QDoubleSpinBox*>("editorCrossSectionLaneWidth")->setValue(3.25);
        if(auto* source=d->findChild<QLineEdit*>("editorCrossSectionSource"))source->setText("Test fixture");d->accept();});
    w.findChild<QAction*>("editorAddCrossSection")->trigger();settle();
    require(w.history().document().definition&&w.history().document().definition->crossSections.size()==1,"Adding the template failed");
    c.setFocus();QTest::keyClick(&c,Qt::Key_O);settle(); // the StreetSegment tool lists the templates
    auto* chosen=w.findChild<QComboBox*>("editorStreetSegmentTemplate");require(chosen&&chosen->count()>1,"No template to choose");
    QApplication::setActiveWindow(&w);if(auto* tool=w.findChild<QComboBox*>("editorTool"))tool->setCurrentIndex(14);settle();
    chosen->setCurrentIndex(1);settle();
    QTest::qWait(600);QTest::mouseClick(c.viewport(),Qt::RightButton,Qt::ControlModifier,pixel(c,a));settle();
    require(c.zoneDraft().size()==1,"The StreetSegment draft did not start");
    QTest::mouseMove(c.viewport(),pixel(c,b));settle();
    const double road=previewArea(c);std::cout<<"StreetSegment preview "<<road<<" m2"<<std::endl;
    require(road>350,"StreetSegment preview is not its template's Links");shot(w,"preview-street-segment");
    chosen->setCurrentIndex(0);settle();require(previewArea(c)==0,"A StreetSegment without a template previewed Links");
    std::cout<<"Creation previews passed"<<std::endl;
}
