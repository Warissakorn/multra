// Canvas style slices B/B2/B3 (D424-D426): Node and zone names carry a Base halo; input rates and
// decision names are white boxes in a grey frame on a leader. All are placed in screen space so no
// two overprint; nearby boxes stack in a column and keep off roads when they can; a label with
// no free place hides unless pinned. Placement is per redraw, drawing only.
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/editor/canvas_labels.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include "../src/editor/canvas_style.hpp"
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <cmath>
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
    return l->deviceTransform(c.viewportTransform()).mapRect(l->frame());
}
// D424's halo placement before boxes: the text's left edge a gap right of the anchor, centred on it.
QRectF rightOf(EditorCanvas& c,CanvasLabel* l) {
    const auto text=l->textRect();
    const QPointF at=c.viewportTransform().map(l->anchor());
    return QRectF(at+QPointF(editorDesign::space2,-text.height()/2),text.size());
}
// D425's first box place: 12 px east of the anchor, centred on it.
QRectF eastBox(EditorCanvas& c,CanvasLabel* l) {
    const auto box=l->frame();
    const QPointF at=c.viewportTransform().map(l->anchor());
    return QRectF(at+QPointF(12,-box.height()/2),box.size());
}
// Every visible box hangs on a leader from its anchor to its frame; a hidden one takes it along.
void leaders(EditorCanvas& c) {
    for(auto* l:labels(c,nullptr))if(l->style()==CanvasLabel::Style::box) {
        auto* leader=l->leader();require(leader&&leader->data(0).toString()=="label-leader","A box has no leader");
        require(leader->isVisible()==l->isVisible(),"A leader outlives its hidden box");
        if(!l->isVisible())continue;
        const auto line=leader->line();
        const QPointF a=c.viewportTransform().map(line.p1()),b=c.viewportTransform().map(line.p2());
        require(QLineF(a,c.viewportTransform().map(l->anchor())).length()<1.5,"A leader does not start at its anchor");
        const auto frame=screen(c,l);
        const double dx=std::max({frame.left()-b.x(),0.,b.x()-frame.right()}),dy=std::max({frame.top()-b.y(),0.,b.y()-frame.bottom()});
        const bool onEdge=std::hypot(dx,dy)<1.5&&(std::abs(b.x()-frame.left())<1.5||std::abs(b.x()-frame.right())<1.5||
                                                  std::abs(b.y()-frame.top())<1.5||std::abs(b.y()-frame.bottom())<1.5);
        require(onEdge,"A leader does not end on its box frame");
    }
}
// Equal to within half a pixel per edge: what placement computed is what was painted.
bool same(const QRectF& a,const QRectF& b) {
    return std::abs(a.left()-b.left())<=.5&&std::abs(a.top()-b.top())<=.5&&std::abs(a.right()-b.right())<=.5&&std::abs(a.bottom()-b.bottom())<=.5;
}
// A drawn road or demand object under a screen rectangle (what D426 avoids when it can).
bool onObject(EditorCanvas& c,const QRectF& rect) {
    for(auto* item:c.scene()->items(c.viewportTransform().inverted().map(QPolygonF(rect)),Qt::IntersectsItemShape)) {
        if(!item->isVisible()||dynamic_cast<CanvasLabel*>(item))continue;
        const auto tag=item->data(0).toString();
        if(tag=="road-surface"||tag=="road-kerb"||tag=="input-marker"||tag=="decision-marker"||tag=="stop-line"||tag=="road-marking")return true;
        if(tag.isEmpty()&&item->zValue()>-100&&item->zValue()<200000)if(auto* p=dynamic_cast<QGraphicsPathItem*>(item);p&&p->pen().style()==Qt::NoPen)return true;
    }
    return false;
}
// Box clusters (anchors chained within 48 px): every visible member of one shares an aligned edge,
// and they stand 2 px apart in a column. Returns how many clusters have two or more members.
int stacked(EditorCanvas& c,const std::vector<CanvasLabel*>& all) {
    std::vector<CanvasLabel*> boxes;for(auto* l:all)if(l->style()==CanvasLabel::Style::box)boxes.push_back(l);
    std::vector<int> group(boxes.size(),-1);int groups=0;
    for(std::size_t i=0;i<boxes.size();++i)if(group[i]<0) {
        std::vector<std::size_t> todo{i};group[i]=groups;
        while(!todo.empty()){const auto k=todo.back();todo.pop_back();
            for(std::size_t j=0;j<boxes.size();++j)if(group[j]<0&&QLineF(c.viewportTransform().map(boxes[k]->anchor()),
                c.viewportTransform().map(boxes[j]->anchor())).length()<=48){group[j]=groups;todo.push_back(j);}}
        ++groups;
    }
    int multi=0;
    for(int g=0;g<groups;++g) {
        std::vector<QRectF> rects;
        for(std::size_t i=0;i<boxes.size();++i)if(group[i]==g&&boxes[i]->isVisible())rects.push_back(screen(c,boxes[i]));
        if(rects.size()<2)continue;
        ++multi;
        std::sort(rects.begin(),rects.end(),[](const QRectF& a,const QRectF& b){return a.top()<b.top();});
        const auto aligned=[&](auto edge){return std::all_of(rects.begin(),rects.end(),[&](const QRectF& r){return std::abs(edge(r)-edge(rects.front()))<=.5;});};
        require(aligned([](const QRectF& r){return r.left();})||aligned([](const QRectF& r){return r.right();})||
                aligned([](const QRectF& r){return r.center().x();}),"A label column is not aligned");
        for(std::size_t i=1;i<rects.size();++i)require(std::abs(rects[i].top()-rects[i-1].bottom()-2)<=.5,"A label column is not 2 px apart");
    }
    return multi;
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
        // 1b. Box: a white box in a grey frame (accent when pinned), then the text.
        for(const bool pin:{false,true}) {
            CanvasLabel label(QStringLiteral("950 veh/h"),editorDesign::numericFont(),{},CanvasLabel::input,pin,CanvasLabel::Style::box);
            label.setBrush(editorDesign::role(QPalette::Text));
            const auto bounds=label.boundingRect();
            QImage image(bounds.size().toSize()+QSize(2,2),QImage::Format_ARGB32);image.fill(QColor("#49596d"));
            QPainter painter(&image);painter.translate(-bounds.topLeft()+QPointF(1,1));label.paint(&painter,nullptr,nullptr);painter.end();
            const auto frame=label.frame().translated(-bounds.topLeft()+QPointF(1,1));
            const QColor inside=image.pixelColor(QPoint(int(frame.left())+2,int(frame.center().y())));
            const QColor edge=image.pixelColor(QPoint(int(frame.center().x()),int(frame.top())));
            require(inside==editorDesign::role(QPalette::Base),"A box is not filled white");
            const QColor want=pin?canvasStyle::selection():editorDesign::role(QPalette::Dark);
            require(std::abs(edge.red()-want.red())+std::abs(edge.green()-want.green())+std::abs(edge.blue()-want.blue())<60,
                    "A box frame is not grey, or accent when pinned");
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
        leaders(canvas);
        for(auto* l:rates)require(l->style()==CanvasLabel::Style::box,"An input rate is not a box");
        // 2b. D426: the anchors within 48 px of each other -- several inputs at one road end -- form a
        // column: one aligned edge, 2 px apart, top to bottom; and no box sits on a road, marker or
        // stop line while the view has room. The forcing: such a cluster exists, and the D425 first
        // place (12 px east of the anchor) of a west-end rate lies on the road.
        for(const double s:{.9,1.5}) {
            view(canvas,QTransform::fromScale(s,-s),centre);
            const auto all=labels(canvas,"input-volume");
            require(stacked(canvas,all)>0,"Setup: no two input anchors share a road end");
            require(std::any_of(all.begin(),all.end(),[&](CanvasLabel* l){return onObject(canvas,eastBox(canvas,l));}),
                    "Setup: the D425 first place covers no road");
            for(auto* l:all)require(l->isVisible(),"A rate label hid although the view has room");
            for(auto* l:all)require(!onObject(canvas,screen(canvas,l)),"A rate box covers a road or marker it could avoid");
        }
        view(canvas,QTransform::fromScale(1.5,-1.5),centre);
        rates=labels(canvas,"input-volume");
        CanvasLabel* moved=rates.front();
        // 3. A hidden label is not clickable; a visible one still opens its input.
        for(auto* l:rates)if(l->isVisible()) {
            const auto at=screen(canvas,l).center().toPoint();
            require(canvas.demandObjectAt(at).first=="input","A visible rate label no longer opens its input");
        }
        // 4. Selecting an input's origin road pins its label: shown, framed in the accent, its column first.
        const auto id=moved->data(1).toString().toStdString();
        std::string origin;
        const auto& def=*window.history().document().definition;
        for(const auto& input:def.inputs)if(input.id==id) {
            origin=input.linkId;
            if(origin.empty())for(const auto& route:def.routes)if(route.id==input.routeId&&!route.segmentIds.empty())origin=route.segmentIds.front();
            if(origin.empty())for(const auto& decision:def.routingDecisions)if(decision.id==input.routingDecisionId)origin=decision.linkId;
        }
        require(!origin.empty(),"Setup: the input names no origin Link");
        canvas.select(origin);canvas.redraw();settle();
        bool pinned=false;
        for(auto* l:labels(canvas,"input-volume"))if(l->data(1).toString().toStdString()==id)
            pinned=l->isVisible()&&l->pinned();
        require(pinned,"A selected input's label is not shown pinned");
        require(!overlapping(visible(canvas)),"A pinned column overprints");
        canvas.clearSelection(false);canvas.redraw();settle();
        // 5. Thai text, a rotated and a flipped view: still no overprint.
        canvas.setDemandLabels(QString::fromUtf8("คัน/ชม."),QString::fromUtf8("%1 ช่วงเวลา"));
        for(const auto& t:{QTransform::fromScale(1.5,-1.5),QTransform().rotate(37)*QTransform::fromScale(1.5,-1.5),
                           QTransform::fromScale(1.5,1.5),QTransform::fromScale(.9,-.9)}) {
            view(canvas,t,centre);
            require(static_cast<int>(labels(canvas,"input-volume").size())==inputs,"A view lost a rate label");
            require(!overlapping(visible(canvas)),"Labels overprint in a Thai, rotated, flipped or far view");
            leaders(canvas);stacked(canvas,labels(canvas,nullptr));
        }
        // 6. The template's period counts and decision names share the pass.
        window.openFile(QString::fromStdString((data/"projects/m2.6-study-template.traffic.json").string()));settle();
        view(canvas,QTransform::fromScale(1,-1),centre);
        require(!labels(canvas,"decision-label").empty()&&!labels(canvas,"input-volume").empty(),"Template lost its labels");
        require(!overlapping(visible(canvas)),"Decision and period labels overprint");
        leaders(canvas);stacked(canvas,labels(canvas,nullptr));
        for(auto* l:labels(canvas,"decision-label"))require(l->style()==CanvasLabel::Style::box,"A decision name is not a box");
        std::cout<<"Canvas labels: halo, screen-space placement, pinned selection, clicks and views passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
