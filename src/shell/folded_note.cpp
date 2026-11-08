#include "folded_note.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
namespace multra {
FoldedNote::FoldedNote(QWidget* parent):QLabel(parent) {
    setWordWrap(true);setFocusPolicy(Qt::TabFocus);
    setTextFormat(Qt::PlainText);
}
void FoldedNote::setFolded(bool folded) {
    if(folded_==folded)return;
    folded_=folded;updateGeometry();update();
}
QString FoldedNote::line() const { return text().simplified(); }
int FoldedNote::lineHeight() const {
    const auto margins=contentsMargins();
    return fontMetrics().height()+margins.top()+margins.bottom()+2*margin();
}
bool FoldedNote::elided() const {
    const int width=contentsRect().width()-2*margin();
    return width>0&&(text().contains(QLatin1Char('\n'))||fontMetrics().horizontalAdvance(line())>width);
}
bool FoldedNote::hasHeightForWidth() const { return !folded_&&QLabel::hasHeightForWidth(); }
int FoldedNote::heightForWidth(int width) const { return folded_?lineHeight():QLabel::heightForWidth(width); }
QSize FoldedNote::sizeHint() const {
    if(!folded_)return QLabel::sizeHint();
    return {QLabel::sizeHint().width(),lineHeight()};
}
QSize FoldedNote::minimumSizeHint() const {
    if(!folded_)return QLabel::minimumSizeHint();
    return {fontMetrics().averageCharWidth()*8,lineHeight()};
}
bool FoldedNote::event(QEvent* event) {
    // The tooltip and the pointer follow whether there is anything more to see.
    if(event->type()==QEvent::Paint||event->type()==QEvent::Resize) {
        const bool more=folded_&&elided();
        const auto tip=more?text():QString();
        if(toolTip()!=tip)setToolTip(tip);
        setAccessibleDescription(text());
        setCursor((more||!folded_)&&!text().isEmpty()?Qt::PointingHandCursor:Qt::ArrowCursor);
    }
    return QLabel::event(event);
}
void FoldedNote::paintEvent(QPaintEvent* event) {
    if(!folded_||!elided()){QLabel::paintEvent(event);return;}
    QPainter painter(this);
    const auto area=contentsRect().adjusted(margin(),margin(),-margin(),-margin());
    const auto shown=fontMetrics().elidedText(line(),Qt::ElideRight,area.width());
    style()->drawItemText(&painter,area,Qt::AlignLeft|Qt::AlignVCenter,palette(),isEnabled(),shown,foregroundRole());
}
void FoldedNote::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button()==Qt::LeftButton&&(elided()||!folded_)){setFolded(!folded_);event->accept();return;}
    QLabel::mouseReleaseEvent(event);
}
void FoldedNote::keyPressEvent(QKeyEvent* event) {
    if(event->key()==Qt::Key_Space||event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter){setFolded(!folded_);event->accept();return;}
    QLabel::keyPressEvent(event);
}
}
