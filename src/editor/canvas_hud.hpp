#pragma once
#include <QWidget>
#include <optional>

namespace trafficsim {
// Screen HUD over the canvas viewport (D197): a 1/2/5 m scale bar, the zoom in px/m and, only
// when the map georeference supplies one, a north arrow. Transparent to the mouse, so it never
// takes a gesture; the canvas places it clear of the map credit.
class CanvasHud : public QWidget {
public:
    explicit CanvasHud(QWidget* viewport);
    // Unit formats with one %1 each, from the shell's locale ("%1 m", "%1 px/m").
    void setUnits(const QString& metres, const QString& zoom);
    // pixelsPerMetre is the view's rotation-safe scale; north is the screen direction of true
    // north in degrees clockwise from up, or nullopt to omit the arrow.
    void setView(double pixelsPerMetre, std::optional<double> northDegrees);
    // Bottom-right with an 8 px inset; above `avoid` when it would overlap; stacked when narrow.
    void place(const QRect& viewport, const QRect& avoid);
    // The largest 1/2/5×10^n metres whose bar is at most 160 px: 64-160 px, as the steps are ≤ 2.5×.
    static double scaleStep(double pixelsPerMetre);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QSize contentSize(bool stacked) const;
    QString metresText() const;
    QString zoomText() const;
    QString metresFormat_{QStringLiteral("%1 m")}, zoomFormat_{QStringLiteral("%1 px/m")};
    double metres_{1}, pixels_{0}, perMetre_{0};
    std::optional<double> north_;
    bool stacked_{};
};
}
