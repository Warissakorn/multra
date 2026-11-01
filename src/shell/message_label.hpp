#pragma once
#include <QLabel>
#include <QString>

class QToolButton;
namespace multra {
// D429: the editor's message line under the canvas. It keeps the full message as its text (tests,
// automation and copy read text()), but shows one row: the first line, elided, then "and N more"
// when the message has more lines. The full text is the tooltip, and the close
// button hides the line until a different message is set. setText and clear hide QLabel's
// non-virtual functions, so every call through a MessageLabel* gets this behaviour.
class MessageLabel : public QLabel {
public:
    explicit MessageLabel(QWidget* parent = nullptr);
    void setText(const QString& text);
    void clear();
    void setMoreText(const QString& pattern) { more_ = pattern; update(); } // "%1" is the count
    void setCloseText(const QString& tip);
    QStringList lines() const;
    QString shownText() const; // the row painted, for tests
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool hasHeightForWidth() const override { return false; }
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    QToolButton* close_{};
    QString more_{QStringLiteral("and %1 more")};
    int textWidth() const;
};
}
