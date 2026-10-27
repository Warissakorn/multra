#include "canvas.hpp"
#include "canvas_image.hpp"
#include "map_layer.hpp"
#include <QPainter>
#include <QFontMetrics>
#include <stdexcept>
#include <algorithm>

namespace multra {
void validateCanvasImageSize(QSize size) {
    if(size.width()<320 || size.height()<240 || size.width()>8192 || size.height()>8192 ||
       static_cast<qint64>(size.width())*size.height()>32000000)throw std::runtime_error("EDIT_IMAGE_SIZE");
}
QImage EditorCanvas::renderImage(const CanvasImageOptions& options) const {
    validateCanvasImageSize(options.pixels);
    EditorCanvas drawing;
    drawing.setMapVisible(false); // No tile requests, event pumping or camera edits during export.
    drawing.setDisplayCatalog(display_);drawing.setVisibleLevel(visibleLevel_);
    drawing.setWireframe(wireframe_);drawing.setBackgroundVisible(false);drawing.setDocument(document_);
    auto bounds=drawing.scene()->itemsBoundingRect();
    if(options.scope==CanvasImageScope::view)bounds=mapToScene(viewport()->rect()).boundingRect();
    const bool empty=bounds.isEmpty();
    if(empty && options.background){drawing.setBackgroundVisible(true);bounds=drawing.scene()->itemsBoundingRect();} // Image-only draft.
    if(bounds.isEmpty())bounds=QRectF(-50,-50,100,100);
    if(options.scope==CanvasImageScope::network) {
        const auto margin=std::max(1.0,0.03*std::max(bounds.width(),bounds.height()));
        bounds.adjust(-margin,-margin,margin,margin);
    }
    QImage image(options.pixels,QImage::Format_ARGB32_Premultiplied);
    if(image.isNull())throw std::runtime_error("EDIT_IMAGE_SIZE");
    image.fill(Qt::white);QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);painter.setRenderHint(QPainter::TextAntialiasing);
    QString credit;
    if(options.background && mapLayer_ && mapLayer_->loadState().visible && mapLayer_->loadState().loaded) {
        credit=mapLayer_->attribution()+" "+mapLayer_->attributionUrl().toString();
    }
    QFont creditFont=font();creditFont.setPixelSize(12);painter.setFont(creditFont);
    const auto footer=credit.isEmpty()?0:QFontMetrics(creditFont).boundingRect(
        QRect(0,0,image.width()-12,image.height()),Qt::TextWordWrap,credit).height()+12;
    if(footer>=image.height())throw std::runtime_error("EDIT_IMAGE_SIZE");
    const QRectF available(0,footer,image.width(),image.height()-footer);
    double scale{};QRectF target;
    const auto frame=[&]{
        scale=std::min(available.width()/bounds.width(),available.height()/bounds.height());
        const QSizeF size(bounds.width()*scale,bounds.height()*scale);
        target=QRectF(available.center()-QPointF(size.width()/2,size.height()/2),size);
        drawing.setTransform(QTransform::fromScale(scale,-scale));drawing.redraw();
    };
    frame(); // Lay out cosmetic markers/text at the export's actual pixel scale.
    if(options.scope==CanvasImageScope::network && !empty) {
        bounds=drawing.scene()->itemsBoundingRect();
        const auto margin=std::max(1.0,0.03*std::max(bounds.width(),bounds.height()));
        bounds.adjust(-margin,-margin,margin,margin);frame();
    }
    if(options.background && !empty)drawing.setBackgroundVisible(true);
    painter.save();painter.translate(0,image.height());painter.scale(1,-1);
    if(!credit.isEmpty()) {
        painter.save();painter.setClipRect(target);
        painter.translate(target.left(),target.top());painter.scale(scale,scale);painter.translate(-bounds.left(),-bounds.top());
        mapLayer_->paint(painter,bounds);painter.restore();
    }
    drawing.scene()->render(&painter,target,bounds,Qt::IgnoreAspectRatio);
    painter.restore();
    if(!credit.isEmpty()) {
        painter.setPen(Qt::black);painter.drawText(QRect(6,image.height()-footer+6,image.width()-12,footer-6),Qt::TextWordWrap,credit);
    }
    if(!credit.isEmpty())image.setText("Map attribution",credit);
    return image;
}
}
