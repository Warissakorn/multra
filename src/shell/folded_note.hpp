#pragma once
#include <QLabel>
namespace multra {
// A Results note folded to one line (D490): the line is elided with "…", the whole text is its
// tooltip and accessible description, and a click, Space or Enter unfolds it to the wrapped
// paragraph (and folds it again). Every note opens with its "not yet validated" statement, so the
// folded line still carries the marker. A note that fits on one line draws as a plain label.
class FoldedNote : public QLabel {
public:
    explicit FoldedNote(QWidget* parent=nullptr);
    bool folded() const { return folded_; }
    void setFolded(bool folded);
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
private:
    QString line() const;  // the text on one line: paragraph breaks become spaces
    bool elided() const;   // the one line does not fit the current width
    int lineHeight() const;
    bool folded_=true;
};
}
