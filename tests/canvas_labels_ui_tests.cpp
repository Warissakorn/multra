// Canvas style slice B (D424): canvas labels carry a Base halo and are placed in screen space --
// right, left, above, below their anchor -- so no two overprint; a label with no free side hides
// unless its object is selected. Placement is per redraw and drawing only.
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/editor/canvas_labels.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QTest>
#include <algorithm>
#include <set>
#include <iostream>
#include <string>
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
std::vector<CanvasLabel*> labels(EditorCanvas& c,const char* tag) {
    std::vector<CanvasLabel*> out;
    for(auto* item:c.scene()->items())if(auto* l=dynamic_cast<CanvasLabel*>(item))
        if(!tag || l->data(0).toString()==tag)out.push_back(l);
    return out;
}
QRectF screen(EditorCanvas& c,CanvasLabel* l) {
    return l->deviceTransform(c.viewportTransform()).mapRect(l->QGraphicsSimpleTextItem::boundingRect());
}
// Today's single placement: the text's left edge a gap right of the anchor, centred on it.
QRectF rightOf(EditorCanvas& c,CanvasLabel* l) {
    const auto text=l->QGraphicsSimpleTextItem::boundingRect();
    const QPointF at=c.mapFromScene(l->anchor());
    return QRectF(at+QPointF(editorDesign::space2,-text.height()/2),text.size());
}
bool overlapping(const std::vector<QRectF>& rects) {
    for(std::size_t i=0;i<rects.size();++i)for(std::size_t j=i+1;j<rects.size();++j)if(rects[i].intersects(rects[j]))return true;
    return false;
}
std::vector<QRectF> visible(EditorCanvas& c) {
    std::vector<QRectF> out;
    for(auto* l:labels(c,nullptr))if(l->isVisible())out.push_back(screen(c,l));
    return out;
}
void view(EditorCanvas& c,const QTransform& t,QPointF centre){c.setTransform(t);c.centerOn(centre);c.redraw();settle();}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory required");const std::filesystem::path data(argv[1]);
        // 1. Halo: on asphalt the glyphs are ringed in Base and filled in the text colour.
        {
            CanvasLabel label(QStringLiteral("950 veh/h"),editorDesign::numericFont(),{},CanvasLabel::input,false);
            label.setBrush(editorDesign::role(QPalette::Text));
            const auto bounds=label.boundingRect();
            QImage image(bounds.size().toSize()+QSize(2,2),QImage::Format_ARGB32);image.fill(QColor("#49596d"));
            QPainter painter(&image);painter.translate(-bounds.topLeft()+QPointF(1,1));label.paint(&painter,nullptr,nullptr);painter.end();
            int base=0,text=0;
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
                const QColor p=image.pixelColor(x,y);
                base+=p==editorDesign::role(QPalette::Base);text+=p.lightness()<60; // glyph strokes are antialiased; asphalt is ~95
            }
            require(base>40&&text>10,"A label is not text on a Base halo");
        }
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));settle();
        auto& canvas=*window.canvas();
        QPointF centre;int n=0;
        for(auto* item:canvas.scene()->items())if(item->data(0).toString()=="road-surface"){centre+=item->sceneBoundingRect().center();++n;}
        centre/=std::max(n,1);
        // 2. The forcing: at 1.5 px/m today's right-hand placement overprints two input rates.
        view(canvas,QTransform::fromScale(1.5,-1.5),centre);
        std::set<QString> drawn;
        for(auto* item:canvas.scene()->items())if(item->data(0).toString()=="input-marker")drawn.insert(item->data(1).toString());
        const int inputs=static_cast<int>(drawn.size());
        auto rates=labels(canvas,"input-volume");
        require(static_cast<int>(rates.size())==inputs&&inputs>1,"Every input does not carry one rate label");
        std::vector<QRectF> before;for(auto* l:rates)before.push_back(rightOf(canvas,l));
        require(overlapping(before),"Setup: the shipped labels no longer collide at 1.5 px/m");
        // ...and placed, no two visible labels meet; every input is shown or hidden, none lost.
        require(!overlapping(visible(canvas)),"Placed labels still overprint");
        CanvasLabel* moved=nullptr;
        for(auto* l:rates)if(!l->isVisible()||l->transform().dx()<0||l->transform().dy()!=-l->QGraphicsSimpleTextItem::boundingRect().height()/2)moved=l;
        require(moved,"No label left its right-hand place");
        // 3. A hidden label is not clickable; a visible one still opens its input.
        for(auto* l:rates)if(l->isVisible()) {
            const auto at=screen(canvas,l).center().toPoint();
            require(canvas.demandObjectAt(at).first=="input","A visible rate label no longer opens its input");
        }
        // 4. Selecting the moved input's origin road pins its label at the right, visible whatever it meets.
        const auto id=moved->data(1).toString().toStdString();
        std::string origin;
        const auto& def=*window.history().document().definition;
        for(const auto& input:def.inputs)if(input.id==id) {
            origin=input.linkId;
            if(origin.empty())for(const auto& route:def.routes)if(route.id==input.routeId&&!route.segmentIds.empty())origin=route.segmentIds.front();
            if(origin.empty())for(const auto& decision:def.routingDecisions)if(decision.id==input.routingDecisionId)origin=decision.linkId;
        }
        require(!origin.empty(),"Setup: the moved input names no origin Link");
        canvas.select(origin);canvas.redraw();settle();
        bool pinned=false;
        for(auto* l:labels(canvas,"input-volume"))if(l->data(1).toString().toStdString()==id)
            pinned=l->isVisible()&&l->pinned()&&screen(canvas,l).toAlignedRect()==rightOf(canvas,l).toAlignedRect();
        require(pinned,"A selected input's label is not shown at its right");
        canvas.clearSelection(false);canvas.redraw();settle();
        // 5. Thai text, a rotated and a flipped view: still no overprint.
        canvas.setDemandLabels(QString::fromUtf8("คัน/ชม."),QString::fromUtf8("%1 ช่วงเวลา"));
        for(const auto& t:{QTransform::fromScale(1.5,-1.5),QTransform().rotate(37)*QTransform::fromScale(1.5,-1.5),
                           QTransform::fromScale(1.5,1.5),QTransform::fromScale(.9,-.9)}) {
            view(canvas,t,centre);
            require(static_cast<int>(labels(canvas,"input-volume").size())==inputs,"A view lost a rate label");
            require(!overlapping(visible(canvas)),"Labels overprint in a Thai, rotated, flipped or far view");
        }
        // 6. The template's period counts and decision names share the pass.
        window.openFile(QString::fromStdString((data/"projects/m2.6-study-template.traffic.json").string()));settle();
        view(canvas,QTransform::fromScale(1,-1),centre);
        require(!labels(canvas,"decision-label").empty()&&!labels(canvas,"input-volume").empty(),"Template lost its labels");
        require(!overlapping(visible(canvas)),"Decision and period labels overprint");
        std::cout<<"Canvas labels: halo, screen-space placement, pinned selection, clicks and views passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
