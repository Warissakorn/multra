#include "message_label.hpp"
#include <QPainter>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>

namespace multra {
MessageLabel::MessageLabel(QWidget* parent) : QLabel(parent) {
    setWordWrap(false);setTextFormat(Qt::PlainText);
    setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
    close_=new QToolButton(this);close_->setObjectName("editorMessageClose");close_->setAutoRaise(true);
    close_->setText(QStringLiteral("×"));close_->setCursor(Qt::ArrowCursor);
    const int side=QFontMetrics(font()).height();close_->setFixedSize(side+4,side); // no taller than the text
    connect(close_,&QToolButton::clicked,this,[this]{hide();});
    hide();
}
void MessageLabel::setText(const QString& text) {
    const bool changed=text!=QLabel::text();
    QLabel::setText(text);setToolTip(text);
    // A new message shows again after a close; repeating the same one leaves the choice alone.
    if(text.isEmpty())hide();else if(changed)show();
    updateGeometry();update();
}
void MessageLabel::clear() { setText({}); }
void MessageLabel::setCloseText(const QString& tip) { close_->setToolTip(tip);close_->setAccessibleName(tip); }
QStringList MessageLabel::lines() const {
    auto all=text().split(QLatin1Char('\n'),Qt::SkipEmptyParts);
    for(auto& line:all)line=line.trimmed();
    all.removeAll(QString());
    return all;
}
int MessageLabel::textWidth() const {
    return std::max(0,contentsRect().width()-close_->width()-4);
}
QString MessageLabel::shownText() const {
    const auto all=lines();
    if(all.isEmpty())return {};
    const QFontMetrics metrics(font());
    if(all.size()==1)return metrics.elidedText(all.front(),Qt::ElideRight,textWidth());
    // One row: the first line, elided to leave room for the count of the rest.
    const QString more=QStringLiteral("  ")+more_.arg(all.size()-1);
    const int room=std::max(0,textWidth()-metrics.horizontalAdvance(more));
    return metrics.elidedText(all.front(),Qt::ElideRight,room)+more;
}
QSize MessageLabel::sizeHint() const {
    const QFontMetrics metrics(font());
    const auto margins=contentsMargins();
    const int height=std::max(metrics.lineSpacing(),close_->height())+margins.top()+margins.bottom();
    return {QLabel::minimumSizeHint().width(),height};
}
QSize MessageLabel::minimumSizeHint() const { return {0,sizeHint().height()}; }
void MessageLabel::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QStyleOption option;option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget,&option,&painter,this); // stylesheet background/border
    painter.setPen(palette().color(foregroundRole()));painter.setFont(font());
    QRect area=contentsRect();area.setWidth(textWidth());
    painter.drawText(area,Qt::AlignLeft|Qt::AlignVCenter,shownText());
}
void MessageLabel::resizeEvent(QResizeEvent* event) {
    QLabel::resizeEvent(event);
    const auto size=close_->size();const auto area=contentsRect();
    close_->setGeometry(area.right()-size.width()+1,area.top()+(area.height()-size.height())/2,size.width(),size.height());
}
}
