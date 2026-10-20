#include "../src/editor/canvas.hpp"
#include <QFontMetricsF>
#include "../src/editor/ui_design_tokens.hpp"
#include "../src/shell/editor_style.hpp"
#include "../src/shell/editor_window.hpp"
#include <QAbstractAnimation>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QStyle>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <QImage>
#include <QPainter>
#include <QTreeWidget>
#include <algorithm>
#include <utility>
#include <vector>
#include <iostream>
#include <limits>
#include <string>
// The presentation contract of the precision-tool restyle: tokens, palette roles, QSS rules,
// box model, number formatting, hairlines and the two-tier grid. Every assertion names the
// measured value so a failure on a platform is a measurement, not a guess.
using namespace trafficsim;
namespace {
void require(bool ok,const std::string& message) { if(!ok)throw std::runtime_error(message); }
double luminance(const QColor& colour) {
    const auto linear=[](double channel){return channel<=.04045?channel/12.92:std::pow((channel+.055)/1.055,2.4);};
    return .2126*linear(colour.redF())+.7152*linear(colour.greenF())+.0722*linear(colour.blueF());
}
double contrast(const QColor& a,const QColor& b) {
    const double x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);
}
// RGB spread, not HSL saturation: near-white greys such as #F9FAFB have a large HSL saturation
// from a 2/255 spread and are still neutral. The slate greys lean blue by up to 24/255 (0.094);
// the accent and semantic colours spread by 0.4 or more, so 0.15 separates them cleanly.
bool chromatic(const QColor& c) {
    return std::max({c.redF(),c.greenF(),c.blueF()})-std::min({c.redF(),c.greenF(),c.blueF()})>.15;
}

void palette() {
    const auto p=editorDesign::editorPalette();
    require(p.color(QPalette::Highlight)==QColor("#2F6FED"),"Accent must be #2F6FED");
    // Only the accent and the four semantic roles may carry hue; every other role is neutral.
    QSet<int> hued{QPalette::Highlight,QPalette::BrightText,QPalette::LinkVisited,QPalette::Link,QPalette::Shadow};
#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    hued.insert(QPalette::Accent);
#endif
    for(int role=0;role<QPalette::NColorRoles;++role) {
        if(role==QPalette::NoRole)continue;
        require(hued.contains(role)||!chromatic(p.color(QPalette::Active,static_cast<QPalette::ColorRole>(role))),
                "Neutral role "+std::to_string(role)+" carries hue");
    }
    // Text-bearing roles meet WCAG AA against both surfaces they sit on. The accent is only ever a
    // fill or border (4.55:1 on white, 4.1:1 on the hover grey), so it is checked as a fill.
    const auto base=p.color(QPalette::Base),hover=p.color(QPalette::Midlight);
    for(const auto role:{QPalette::Text,QPalette::WindowText,QPalette::BrightText,QPalette::LinkVisited,QPalette::Link,QPalette::Shadow}) {
        require(contrast(p.color(role),base)>=4.5,"Role "+std::to_string(role)+" below 4.5:1 on Base: "+std::to_string(contrast(p.color(role),base)));
        require(contrast(p.color(role),hover)>=4.5,"Role "+std::to_string(role)+" below 4.5:1 on Midlight: "+std::to_string(contrast(p.color(role),hover)));
    }
    require(contrast(p.color(QPalette::HighlightedText),p.color(QPalette::Highlight))>=4.5,"Text on the accent below 4.5:1");
    require(contrast(p.color(QPalette::Dark),base)>=3,"Control outline below 3:1 (WCAG 1.4.11)");
    for(const auto s:{editorDesign::Semantic::error,editorDesign::Semantic::warning,editorDesign::Semantic::advisory,editorDesign::Semantic::ok})
        require(editorDesign::semantic(s).isValid(),"Semantic colour missing");
}
void stylesheet() {
    const auto qss=editorStyleSheet();
    require(!qss.contains(QRegularExpression("#[0-9a-fA-F]{3,8}(?![0-9a-zA-Z_])")),"QSS holds a hex colour literal");
    for(const char* banned:{"gradient","box-shadow","glow","transition","animation","letter-spacing","rgb("})
        require(!qss.contains(banned),std::string("QSS uses ")+banned);
    const QSet<int> sizes{11,12,13,14,18};
    auto sizeMatches=QRegularExpression("font-size:\\s*(\\d+)px").globalMatch(qss);
    int found=0;
    while(sizeMatches.hasNext()){const int px=sizeMatches.next().captured(1).toInt();++found;
        require(sizes.contains(px),"Off-scale font size "+std::to_string(px));}
    require(found>0,"No font sizes found");
    auto radii=QRegularExpression("border-radius:\\s*(\\d+)px").globalMatch(qss);
    while(radii.hasNext()){const int px=radii.next().captured(1).toInt();
        require(px<=editorDesign::maxRadius,"Radius above 3 px: "+std::to_string(px));}
    require(editorDesign::maxMotionMs<=150,"Motion budget above 150 ms");
    for(const int space:{editorDesign::space1,editorDesign::space2,editorDesign::space3,editorDesign::space4,editorDesign::space5,editorDesign::space6})
        require(space%4==0,"Spacing token off the 4 px scale");
}
void boxModel() {
    QWidget host;applyEditorStyle(&host);auto* layout=new QVBoxLayout(&host);
    auto* edit=new QLineEdit(&host);auto* spin=new QDoubleSpinBox(&host);auto* combo=new QComboBox(&host);
    auto* button=new QPushButton("Apply ตกลง",&host);combo->addItem("ค่าเริ่มต้น x");edit->setText("ถนน 12");
    for(QWidget* control:{static_cast<QWidget*>(edit),static_cast<QWidget*>(spin),static_cast<QWidget*>(combo),static_cast<QWidget*>(button)})layout->addWidget(control);
    host.show();QApplication::processEvents();
    for(QWidget* control:{static_cast<QWidget*>(edit),static_cast<QWidget*>(spin),static_cast<QWidget*>(combo),static_cast<QWidget*>(button)}) {
        // The laid-out height, with the bundled Thai face: that is what the user sees. A size hint
        // alone varies with the font's line height and with Qt's per-version spin-box extra.
        const auto height=[&]{host.adjustSize();QApplication::processEvents();return control->height();};
        require(height()==editorDesign::controlHeight,
                std::string(control->metaObject()->className())+" is "+std::to_string(control->height())+" px, not "+std::to_string(editorDesign::controlHeight));
        // The invalid state swaps a 1 px border for 2 px; the content gives 2 px back so height holds.
        control->setProperty("validationState","invalid");control->style()->unpolish(control);control->style()->polish(control);
        require(height()==editorDesign::controlHeight,
                std::string(control->metaObject()->className())+" changes height when invalid: "+std::to_string(control->height()));
        control->setProperty("validationState",QVariant());control->style()->unpolish(control);control->style()->polish(control);
    }
    // One vertical pattern: the same digits sit at the same rows in every kind of control, and
    // centred to the pixel (a 9 px cap in 24 px leaves 15 px, so 7 above and 8 below is centred).
    // Above 100 % the limit is measured, not chosen: Qt centres the font's line box, and the 13 px
    // face's 6 px descent (Thai below-vowels) leaves its digits up to 1.5 logical px high (13/16
    // device px at 2x); the 12 px numeric face sits about 0.5 px high. A 1 px shift centred the
    // digits but put body and numeric digits on different rows at 100 %, which the owner declined
    // (2026-10-03). So: centred within 1.5 logical px, and rows within one logical px of each other.
    const double ratio=edit->devicePixelRatioF();
    const int centring=ratio==1?1:int(std::ceil(1.5*ratio)),rows=ratio==1?0:int(std::ceil(ratio));
    button->setFocus();QApplication::processEvents(); // no text cursor in the line edit
    edit->setText("0000");spin->setValue(0);combo->setItemText(0,"0000");button->setText("0000");
    std::vector<std::pair<int,int>> gaps;
    for(QWidget* control:{static_cast<QWidget*>(edit),static_cast<QWidget*>(spin),static_cast<QWidget*>(combo),static_cast<QWidget*>(button)}) {
        QApplication::processEvents();
        const QImage image=control->grab().toImage();int top=-1,bottom=-1;
        for(int y=2;y<image.height()-2;++y)for(int x=4;x<image.width()-24;++x)
            if(image.pixelColor(x,y).lightness()<110){if(top<0)top=y;bottom=y;break;}
        require(top>=0,std::string(control->metaObject()->className())+" drew no text");
        gaps.push_back({top,image.height()-1-bottom});
        require(std::abs(gaps.back().first-gaps.back().second)<=centring,
                std::string(control->metaObject()->className())+" text is not centred: "+std::to_string(gaps.back().first)+" above, "+std::to_string(gaps.back().second)+" below");
        require(std::abs(gaps.back().first-gaps.front().first)<=rows&&std::abs(gaps.back().second-gaps.front().second)<=rows,
                std::string(control->metaObject()->className())+" places text differently from a line edit");
    }
    // Same configuration as the editor's toolbars (editor_workspace.cpp): icon-only 16 px buttons.
    // Without it the button shows its text beside the icon and is 27 px, which is a test artefact.
    QToolBar bar;bar.setStyleSheet(editorStyleSheet());bar.setIconSize(QSize(editorDesign::iconSize,editorDesign::iconSize));
    bar.setToolButtonStyle(Qt::ToolButtonIconOnly);
    auto* selectAction=bar.addAction(editorIcon(EditorIcon::select),"Select");bar.show();QApplication::processEvents();
    // widgetForAction, not findChild: the first QToolButton child is the 12 px overflow extension.
    const auto* toolButton=qobject_cast<QToolButton*>(bar.widgetForAction(selectAction));
    require(toolButton!=nullptr,"Toolbar has no button for its action");
    require(toolButton->sizeHint().height()==editorDesign::iconSize+8,
            "Toolbar button is "+std::to_string(toolButton->sizeHint().height())+" px, not 24 (iconSize "+
            std::to_string(toolButton->iconSize().height())+", style "+std::to_string(static_cast<int>(toolButton->toolButtonStyle()))+
            ", font height "+std::to_string(toolButton->fontMetrics().height())+", text '"+toolButton->text().toStdString()+
            "', minimum "+std::to_string(toolButton->minimumSizeHint().height())+", padding-rule "+
            std::to_string(toolButton->style()->pixelMetric(QStyle::PM_ButtonMargin,nullptr,toolButton))+")");
    require(bar.sizeHint().height()==editorDesign::toolbarHeight,
            "Toolbar is "+std::to_string(bar.sizeHint().height())+" px, not 32 (button "+std::to_string(toolButton->sizeHint().height())+
            ", contents margins "+std::to_string(bar.contentsMargins().top())+"/"+std::to_string(bar.contentsMargins().bottom())+
            ", layout margins "+std::to_string(bar.layout()?bar.layout()->contentsMargins().top():-1)+"/"+
            std::to_string(bar.layout()?bar.layout()->contentsMargins().bottom():-1)+")");
    require(editorIcon(EditorIcon::select).actualSize(QSize(64,64),QIcon::Normal).width()>=editorDesign::iconSize,"Icon raster smaller than 16 px");
    // The active tool: Mid fill and an accent underline, no frame, and no change of height.
    selectAction->setCheckable(true);selectAction->setChecked(true);QApplication::processEvents();
    require(toolButton->sizeHint().height()==editorDesign::iconSize+8,"Checked toolbar button changes height");
    const QImage checked=const_cast<QToolButton*>(toolButton)->grab().toImage();
    // The bar carries the stylesheet but not editorPalette(), so roles resolve on its own palette.
    const auto accent=toolButton->palette().color(QPalette::Highlight);
    const auto isAccent=[&](QColor c){return std::abs(c.red()-accent.red())<24&&std::abs(c.green()-accent.green())<24&&std::abs(c.blue()-accent.blue())<24;};
    const int middle=checked.width()/2;
    require(isAccent(checked.pixelColor(middle,checked.height()-1)),"Checked toolbar button has no accent underline");
    require(!isAccent(checked.pixelColor(middle,0))&&!isAccent(checked.pixelColor(0,checked.height()/2)),"Checked toolbar button draws an accent frame");
    selectAction->setChecked(false);QApplication::processEvents();
    const QImage resting=const_cast<QToolButton*>(toolButton)->grab().toImage();
    require(checked.pixelColor(2,checked.height()/2)!=resting.pixelColor(2,resting.height()/2),"Checked toolbar button is not filled");
    require(!isAccent(resting.pixelColor(middle,resting.height()-1)),"Resting toolbar button draws the checked underline");
}
void typography() {
    const auto numeric=editorDesign::numericFont();
    // Tabular digits, not a monospace face: every digit advances the same, so columns align.
    const QFontMetricsF digits(numeric);
    for(QChar d='1';d<='9';d=QChar(d.unicode()+1))
        require(digits.horizontalAdvance(d)==digits.horizontalAdvance(QChar('0')),"Numeric digits are not tabular");
    require(numeric.pixelSize()==12,"Numeric face is not 12 px");
    QFont english;editorDesign::styleGroupLabel(english,true);
    require(english.pixelSize()==11&&english.capitalization()==QFont::AllUppercase&&english.letterSpacingType()==QFont::AbsoluteSpacing&&
            english.letterSpacing()==editorDesign::labelTracking,"English group label is not 11 px tracked uppercase");
    QFont thai;editorDesign::styleGroupLabel(thai,false);
    require(thai.capitalization()==QFont::MixedCase&&thai.letterSpacingType()==QFont::PercentageSpacing&&thai.letterSpacing()==100.,
            "Thai group label must stay untracked");
    const QLocale german(QLocale::German,QLocale::Germany),english_(QLocale::English,QLocale::UnitedStates);
    const auto nbsp=QString(QChar(0x00A0));
    require(editorDesign::formatValue(1234.5,2,"m",german)=="1.234,50"+nbsp+"m","German format: "+editorDesign::formatValue(1234.5,2,"m",german).toStdString());
    require(editorDesign::formatValue(1234.5,2,"m",english_)=="1,234.50"+nbsp+"m","English format wrong");
    require(editorDesign::formatValue(3,3,QString(),english_)=="3.000","Fixed decimals lost");
}
void hairlines() {
    // A cosmetic width is in device pixels, so one device pixel is width 1 at every scale; the
    // scaled ctest entries' gridIsCrisp() is what proves it on the running Qt.
    const auto pen=editorDesign::hairlinePen(QColor("#E5E7EB"));
    require(pen.isCosmetic()&&std::abs(pen.widthF()-1)<1e-12&&pen.capStyle()==Qt::FlatCap,"Hairline is not a one-device-pixel cosmetic pen");
    // Every snapped coordinate lands on a device-pixel centre, for either axis sign and any offset.
    for(const double scale:{4.,-4.,1.5,-.25,8.})for(const double offset:{0.,3.3,-120.7,401.})for(double scene=-50.3;scene<50;scene+=7.31) {
        const double device=editorDesign::snapHairline(scene,scale,offset)*scale+offset;
        require(std::abs(device-std::floor(device)-.5)<1e-9,"Hairline not on a device pixel centre (scale "+std::to_string(scale)+")");
    }
    require(editorDesign::snapHairline(5,0,0)==5,"Zero scale must pass the coordinate through");
}
void gridTiers() {
    require(std::abs(editorDesign::levelOfDetail(QTransform::fromScale(4,-4))-4)<1e-12,"LOD of a 4x flipped view is not 4");
    auto tiers=editorDesign::gridTiers(1,4);
    require(tiers.minor==10&&tiers.major==100,"4 px/m: minor must jump to 10 m");
    tiers=editorDesign::gridTiers(1,10);require(tiers.minor==1&&tiers.major==10,"10 px/m keeps the 1 m minor");
    tiers=editorDesign::gridTiers(1,.05);require(tiers.minor*.05>=editorDesign::minorMinPixels&&tiers.minor==1000,"Zoomed-out minor spacing below 8 px");
    tiers=editorDesign::gridTiers(.1,100);require(tiers.minor==.1,"Fine grid dropped at high zoom");
    for(const double bad:{0.,-1.,std::nan(""),std::numeric_limits<double>::infinity()}) {
        require(editorDesign::gridTiers(bad,4).minor==0,"Bad step accepted");require(editorDesign::gridTiers(1,bad).minor==0,"Bad zoom accepted");
    }
}
// An empty canvas paints only the background, so every pixel must be exactly one of the three
// neutral roles: a blended pixel means a line straddled two device pixels.
void gridIsCrisp() {
    EditorCanvas canvas;canvas.resize(420,300);canvas.show();QApplication::processEvents();
    const auto image=canvas.viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);
    const QSet<QRgb> allowed{editorDesign::role(QPalette::Base).rgb(),editorDesign::role(QPalette::Midlight).rgb(),editorDesign::role(QPalette::Mid).rgb()};
    // The HUD (D197) paints its own text over the grid; the grid is judged everywhere else.
    auto* hud=canvas.viewport()->findChild<QWidget*>("editorCanvasHud");require(hud&&hud->isVisible(),"Canvas HUD missing");
    const double ratio=image.devicePixelRatio(); // the image is in device pixels
    const QRectF logical(hud->geometry());
    const auto hudRect=QRectF(logical.topLeft()*ratio,logical.size()*ratio).toAlignedRect().adjusted(-1,-1,1,1);
    int lines=0;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
        if(hudRect.contains(x,y))continue;
        const auto pixel=image.pixel(x,y)|0xFF000000u;
        require(allowed.contains(pixel),"Blended grid pixel "+QColor(pixel).name().toStdString()+" at "+std::to_string(x)+","+std::to_string(y));
        lines+=pixel!=editorDesign::role(QPalette::Base).rgb();
    }
    require(lines>0,"Grid drew nothing");
}
// The 30 glyphs with the names the contact sheet prints; the enum order (editor_style.hpp).
const std::pair<EditorIcon,const char*> glyphs[]={
    {EditorIcon::document,"document"},{EditorIcon::open,"open"},{EditorIcon::save,"save"},{EditorIcon::undo,"undo"},
    {EditorIcon::redo,"redo"},{EditorIcon::fit,"fit"},{EditorIcon::finish,"finish"},{EditorIcon::rotate,"rotate"},
    {EditorIcon::remove,"remove"},{EditorIcon::select,"select"},{EditorIcon::link,"link"},{EditorIcon::connector,"connector"},
    {EditorIcon::route,"route"},{EditorIcon::input,"input"},{EditorIcon::signal,"signal"},{EditorIcon::split,"split"},
    {EditorIcon::measure,"measure"},{EditorIcon::image,"image"},{EditorIcon::run,"run"},{EditorIcon::pause,"pause"},
    {EditorIcon::step,"step"},{EditorIcon::reset,"reset"},{EditorIcon::inspector,"inspector"},{EditorIcon::objects,"objects"},
    {EditorIcon::focus,"focus"},{EditorIcon::grid,"grid"},{EditorIcon::conflict,"conflict"},{EditorIcon::counter,"counter"},
    {EditorIcon::section,"section"},{EditorIcon::zone,"zone"}};
// The raster QIcon holds for one ratio, unscaled: at a device pixel ratio of 1, asking for 16*r px
// returns the 16*r px pixmap that editorIcon() drew.
QImage raster(EditorIcon icon,int ratio) {
    const int side=editorDesign::iconSize*ratio;
    return editorIcon(icon).pixmap(QSize(side,side)).toImage().convertToFormat(QImage::Format_ARGB32);
}
// Inked = alpha above a third; antialiasing fringes below that are not part of the shape.
QVector<bool> ink(const QImage& image) {
    QVector<bool> mask(image.width()*image.height());
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)mask[y*image.width()+x]=qAlpha(image.pixel(x,y))>85;
    return mask;
}
// Share of the pair's inked pixels that only one of them inks: 0 for identical shapes, 1 for disjoint.
double shapeDifference(EditorIcon a,EditorIcon b) {
    const auto x=ink(raster(a,1)),y=ink(raster(b,1));
    int either=0,one=0;
    for(int i=0;i<x.size();++i){either+=x[i]||y[i];one+=x[i]!=y[i];}
    return either?static_cast<double>(one)/either:0;
}
// Slice a's contract (UI_PRESENTATION_SLICES): metrics, roles and distinct shapes at 16 px.
// Distinct pixels do not prove a user tells the shapes apart; the contact sheet and the owner's
// review (OWNER_SITTING E14) do that. These rows only stop two glyphs collapsing into one.
void iconVocabulary() {
    const auto text=editorDesign::role(QPalette::WindowText),accent=editorDesign::accent();
    const auto near=[](QColor c,const QColor& want){return std::abs(c.red()-want.red())<12&&std::abs(c.green()-want.green())<12&&std::abs(c.blue()-want.blue())<12;};
    for(const auto& [icon,name]:glyphs) {
        QList<QSize> sizes=editorIcon(icon).availableSizes();
        std::sort(sizes.begin(),sizes.end(),[](QSize a,QSize b){return a.width()<b.width();});
        require(sizes==QList<QSize>({QSize(16,16),QSize(32,32),QSize(48,48)}),std::string(name)+" is not rastered at exactly 16/32/48 px");
        for(int ratio:{1,2,3}) {
            // Antialiased 1.5 px strokes leave no fully opaque pixel at 1x, so the colour is read from
            // every pixel at half coverage or more (Format_ARGB32 is unpremultiplied).
            const auto image=raster(icon,ratio);int inked=0;
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
                const QColor c=image.pixelColor(x,y);if(c.alpha()<128)continue;++inked;
                const bool filled=icon==EditorIcon::run||icon==EditorIcon::step;
                require(near(c,text)||(filled&&near(c,accent)),std::string(name)+" paints "+c.name().toStdString()+", not WindowText"+(filled?" or the accent":""));
            }
            require(inked>0,std::string(name)+" draws nothing at "+std::to_string(ratio)+"x");
        }
    }
    for(std::size_t i=0;i<std::size(glyphs);++i)for(std::size_t j=i+1;j<std::size(glyphs);++j)
        require(shapeDifference(glyphs[i].first,glyphs[j].first)>0,std::string(glyphs[i].second)+" and "+glyphs[j].second+" are the same 16 px shape");
    // The audit's near pairs must differ in outline, not by one detail: at least 40% of their
    // combined ink is one glyph's only. Measured before slice a
    // (D192): fit/focus 0.23, counter/section 0.90, link/split 0.73, connector/route and rotate/reset 0.
    for(const auto& [a,b]:{std::pair{EditorIcon::fit,EditorIcon::focus},{EditorIcon::counter,EditorIcon::section},{EditorIcon::link,EditorIcon::split},
                           {EditorIcon::connector,EditorIcon::route},{EditorIcon::rotate,EditorIcon::reset}}) {
        const double d=shapeDifference(a,b);
        const auto name=[](EditorIcon icon){return std::string(glyphs[static_cast<int>(icon)].second);};
        require(d>=.4,name(a)+"/"+name(b)+" differ in only "+std::to_string(d)+" of their 16 px ink");
    }
}
// trafficsim-design-ui-tests <data> --contact-sheet <png>: every glyph on white, at actual 16 px,
// at 16 px magnified 4x with no smoothing, and at the 32/48 px rasters, then reduced to grey.
void contactSheet(const QString& path) {
    constexpr int columns=3,cellWidth=300,cellHeight=76;
    const int rows=(static_cast<int>(std::size(glyphs))+columns-1)/columns;
    QImage sheet(columns*cellWidth,rows*cellHeight,QImage::Format_ARGB32);sheet.fill(Qt::white);
    QPainter p(&sheet);p.setPen(Qt::black);
    for(std::size_t i=0;i<std::size(glyphs);++i) {
        const int x=static_cast<int>(i%columns)*cellWidth+8,y=static_cast<int>(i/columns)*cellHeight+6;
        p.drawText(QRect(x,y,84,64),Qt::AlignLeft|Qt::AlignVCenter,QString::fromLatin1(glyphs[i].second));
        const auto one=raster(glyphs[i].first,1);
        p.drawImage(QPoint(x+84,y+24),one);
        p.drawImage(QRect(x+108,y,64,64),one);
        p.drawImage(QPoint(x+180,y+16),raster(glyphs[i].first,2));
        p.drawImage(QPoint(x+220,y+8),raster(glyphs[i].first,3));
    }
    p.end();
    require(sheet.convertToFormat(QImage::Format_Grayscale8).save(path),"Could not write the contact sheet to "+path.toStdString());
}
void window(const std::filesystem::path& data) {
    EditorWindow w{data};w.show();QApplication::processEvents();
    int buddies=0;
    for(auto* label:w.findChildren<QLabel*>())if(auto* field=label->buddy()) {
        ++buddies;require(field->focusPolicy()!=Qt::NoFocus,"Buddy field cannot take focus: "+field->objectName().toStdString());
    }
    require(buddies>5,"Inspector labels lost their buddies");
    // QMainWindow's layout still builds a zero-length geometry animation per re-laid-out widget and
    // deletes it later; flush those so only an animation the editor keeps can fail this.
    require(!w.isAnimated(),"The main window animates dock and toolbar moves");
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    require(w.findChildren<QAbstractAnimation*>().isEmpty(),"The editor owns an animation");
    require(w.palette().color(QPalette::Highlight)==QColor("#2F6FED"),"Window did not take the editor palette");
    // One glyph per object on every surface: the palette rows and the actions share editorIcon()'s
    // cached QIcon (same cacheKey), so no surface carries a copied painter path.
    const EditorIcon tools[]={EditorIcon::select,EditorIcon::link,EditorIcon::split,EditorIcon::measure,EditorIcon::image,
        EditorIcon::connector,EditorIcon::route,EditorIcon::input,EditorIcon::signal,EditorIcon::conflict,
        EditorIcon::counter,EditorIcon::section,EditorIcon::zone,EditorIcon::zone}; // S0.1: the Node tool reuses the zone glyph
    auto* palette=w.findChild<QTreeWidget*>("editorObjectPalette");require(palette!=nullptr,"No object palette");
    auto* iconsOnly=w.findChild<QAction*>("editorPaletteIconsOnly");require(iconsOnly&&iconsOnly->isCheckable(),"No palette view toggle");
    auto* toggle=w.findChild<QToolButton*>("editorPaletteViewButton");require(toggle!=nullptr,"No palette view button");
    for(const bool compact:{false,true}) {
        if(iconsOnly->isChecked()!=compact)iconsOnly->trigger();
        QApplication::processEvents();require(iconsOnly->isChecked()==compact,"Palette view toggle did not take effect");
        const std::string view=compact?" (icons only)":" (names)";
        int rows=0;
        for(int g=0;g<palette->topLevelItemCount();++g)for(int r=0;r<palette->topLevelItem(g)->childCount();++r) {
            const auto* item=palette->topLevelItem(g)->child(r);const int mode=item->data(0,Qt::UserRole).toInt();++rows;
            require(mode>=0&&mode<14&&item->icon(0).cacheKey()==editorIcon(tools[mode]).cacheKey(),"Palette tool "+std::to_string(mode)+" does not reuse its glyph"+view);
        }
        require(rows==14,"Palette lists "+std::to_string(rows)+" tools, not 14"+view);
        require(toggle->height()==editorDesign::controlHeight,"Palette view button height changed"+view);
    }
    // D190 state language in the icons-only view: the active tool is Mid with a left accent bar; a
    // keyboard-focused row that is not the tool draws only the accent frame. Forcing first: focus.
    QApplication::setActiveWindow(&w);palette->setFocus();QApplication::processEvents();
    require(palette->hasFocus(),"Palette focus fixture did not take effect");
    const auto accent=editorDesign::accent();
    const auto near=[](QColor a,QColor b){return std::abs(a.red()-b.red())<24&&std::abs(a.green()-b.green())<24&&std::abs(a.blue()-b.blue())<24;};
    auto* active=palette->topLevelItem(0)->child(0);palette->setCurrentItem(active);QApplication::processEvents();
    require(active->isSelected(),"Active tool fixture not selected");
    auto shot=palette->viewport()->grab().toImage();auto row=palette->visualItemRect(active);
    require(near(shot.pixelColor(row.left(),row.center().y()),accent),"Icons-only active tool has no left accent bar");
    require(near(shot.pixelColor(row.right()-3,row.center().y()),editorDesign::role(QPalette::Mid)),"Icons-only active tool is not Mid-filled");
    auto* header=palette->topLevelItem(1);palette->setCurrentItem(header);QApplication::processEvents();
    require(!header->isSelected()&&palette->currentItem()==header,"Focus-only fixture selected the row");
    shot=palette->viewport()->grab().toImage();row=palette->visualItemRect(header);
    require(near(shot.pixelColor(row.center().x(),row.top()),accent),"Icons-only keyboard focus draws no accent frame");
    require(!near(shot.pixelColor(row.center().x(),row.center().y()),editorDesign::role(QPalette::Mid)),"Focus-only row is filled as selected");
    iconsOnly->trigger();QApplication::processEvents();
    auto* objects=w.findChild<QAction*>("editorNetworkObjects");
    require(objects&&objects->icon().cacheKey()==editorIcon(EditorIcon::link).cacheKey(),"Network objects does not reuse the Link glyph");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>=2,"Expected data directory");
        // The editor's own face, as EditorWindow loads it: heights are measured with the font
        // the user sees, not the test machine's default.
        loadEditorFont(argv[1]);
        if(argc>=3&&std::string(argv[2])=="--at-scale") {
            // The scaled ctest entries: the box model and the grid, which promise the same at every scale.
            // The forcing first: the window really is at the scale the entry asked for.
            const double asked=qEnvironmentVariable("QT_SCALE_FACTOR").toDouble();
            QWidget probe;probe.show();QApplication::processEvents();
            require(asked>1&&std::abs(probe.devicePixelRatioF()-asked)<1e-6,
                    "Scale factor not applied: asked "+std::to_string(asked)+", got "+std::to_string(probe.devicePixelRatioF()));
            boxModel();gridIsCrisp();
            std::cout<<"At "<<asked<<"x: box model and grid passed\n";return 0;
        }
        if(argc>=4&&std::string(argv[2])=="--contact-sheet"){contactSheet(QString::fromLocal8Bit(argv[3]));return 0;}
        palette();stylesheet();boxModel();typography();hairlines();gridTiers();gridIsCrisp();iconVocabulary();window(argv[1]);
        std::cout<<"Palette roles, QSS rules, 24/32 px box model, formatting, hairlines and grid tiers passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
