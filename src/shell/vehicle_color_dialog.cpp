#include "vehicle_color_dialog.hpp"
#include "dialog_fit.hpp"
#include "editor_interaction.hpp"
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QIcon>
#include <QPixmap>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <algorithm>
#include <set>
namespace multra {
namespace {
bool validColor(const QString& color) {
    static const QRegularExpression hex(QStringLiteral("^#[0-9a-fA-F]{6}$"));
    return hex.match(color).hasMatch();
}
bool save(const DisplayCatalog& catalog,const QString& path) {
    QJsonObject palettes;
    for(const auto& [id,colors]:catalog.vehicleColorPalettes) {
        QJsonArray values;for(const auto& color:colors)values.append(QString::fromStdString(color));
        palettes.insert(QString::fromStdString(id),values);
    }
    const QByteArray bytes=QJsonDocument(QJsonObject{{"version",1},{"palettes",palettes}}).toJson();
    if(!QDir().mkpath(QFileInfo(path).absolutePath()))return false;
    QSaveFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
}
}
QString vehicleColorSettingsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/vehicle-colors.json";
}
bool loadVehicleColorPreferences(DisplayCatalog& catalog,const QString& path) {
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)return false;
    QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError || !document.isObject())return false;
    const auto root=document.object();
    if(root.value("version").toDouble()!=1 || !root.value("palettes").isObject())return false;
    std::map<std::string,std::vector<std::string>> palettes;
    const auto object=root.value("palettes").toObject();
    for(auto entry=object.begin();entry!=object.end();++entry) {
        if(entry.key().isEmpty()||!entry.value().isArray())return false;
        const auto values=entry.value().toArray();if(values.isEmpty()||values.size()>16)return false;
        std::set<QString> seen;std::vector<std::string> colors;
        for(const auto& value:values) {
            if(!value.isString()||!validColor(value.toString())||!seen.insert(value.toString().toLower()).second)return false;
            colors.push_back(value.toString().toLower().toStdString());
        }
        palettes.emplace(entry.key().toStdString(),std::move(colors));
    }
    catalog.vehicleColorPalettes=std::move(palettes);return true;
}
bool editVehicleColors(QWidget* parent,DisplayCatalog& catalog,const std::vector<std::string>& typeIds,
        const std::function<QString(const char*)>& tr,const QString& path) {
    if(typeIds.empty())return false;
    auto staged=catalog;
    QDialog dialog(parent);dialog.setObjectName("editorVehicleColorDialog");dialog.setWindowTitle(tr("editorVehicleColors"));
    auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);
    auto* help=new ReflowLabel(tr("editorVehicleColorsHelp"),body);help->setWordWrap(true);
    setReflowPolicy(help,QSizePolicy::Ignored);layout->addWidget(help);
    auto* types=new QComboBox(body);types->setObjectName("editorVehicleColorType");
    for(const auto& id:typeIds)types->addItem(QString::fromStdString(id));
    layout->addWidget(types);
    auto* mixed=new QCheckBox(tr("editorVehicleColorsMixed"),body);mixed->setObjectName("editorVehicleColorMixed");layout->addWidget(mixed);
    auto* list=new QListWidget(body);list->setObjectName("editorVehicleColorList");
    list->setViewMode(QListView::IconMode);list->setMovement(QListView::Static);list->setResizeMode(QListView::Adjust);
    list->setIconSize(QSize(36,28));list->setMinimumHeight(130);layout->addWidget(list);
    auto* row=new QHBoxLayout;layout->addLayout(row);
    auto button=[&](const char* key,const char* name){auto* b=new QPushButton(tr(key),body);b->setObjectName(name);row->addWidget(b);return b;};
    auto* add=button("editorVehicleColorAdd","editorVehicleColorAdd");
    auto* edit=button("editorVehicleColorEdit","editorVehicleColorEdit");
    auto* remove=button("editorVehicleColorRemove","editorVehicleColorRemove");
    auto* defaults=new QPushButton(tr("editorVehicleColorsPalette"),body);defaults->setObjectName("editorVehicleColorPalette");layout->addWidget(defaults);
    auto* reset=new QPushButton(tr("editorVehicleColorsReset"),body);reset->setObjectName("editorVehicleColorReset");layout->addWidget(reset);
    auto* error=new ReflowLabel(body);error->setObjectName("editorVehicleColorError");error->setWordWrap(true);
    setReflowPolicy(error,QSizePolicy::Ignored);layout->addWidget(error);
    auto* footer=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    footer->button(QDialogButtonBox::Ok)->setText(tr("editorConfirm"));footer->button(QDialogButtonBox::Cancel)->setText(tr("editorCancel"));addFooter(dialog,footer);
    bool loading=false;
    const auto id=[&]{return types->currentText().toStdString();};
    const auto baseColor=[&]{
        const auto normalise=[](const std::string& c){return QColor(QString::fromStdString(c)).name().toStdString();};
        const auto c=catalog.vehicleColors.find(id());if(c!=catalog.vehicleColors.end())return normalise(c->second);
        for(const auto& type:catalog.types)if(type.id=="default")return normalise(type.vehicleColor);
        return std::string{};
    };
    std::vector<std::string> colors;
    const auto redraw=[&]{
        list->clear();for(const auto& color:colors) {
            const QString label=QString::fromStdString(color);QPixmap swatch(36,28);swatch.fill(QColor(label));
            auto* item=new QListWidgetItem(QIcon(swatch),label,list);item->setData(Qt::UserRole,label);
        }
        list->setCurrentRow(0);loading=true;mixed->setChecked(colors.size()>1);loading=false;
        add->setEnabled(colors.size()<16);remove->setEnabled(colors.size()>2);
    };
    const auto load=[&]{
        const auto p=staged.vehicleColorPalettes.find(id());colors=p==staged.vehicleColorPalettes.end()?std::vector<std::string>{baseColor()}:p->second;
        error->clear();redraw();
    };
    const auto store=[&]{staged.vehicleColorPalettes[id()]=colors;error->clear();redraw();};
    const auto usePalette=[&]{
        if(catalog.availableVehicleColors.size()<2){redraw();error->setText(tr("editorVehicleColorsNoPalette"));return;}
        colors=catalog.availableVehicleColors;
        store();
    };
    QObject::connect(types,&QComboBox::currentIndexChanged,&dialog,[&]{load();});
    QObject::connect(mixed,&QCheckBox::toggled,&dialog,[&](bool enabled){
        if(loading)return;
        if(enabled)usePalette();else{
            const int at=list->currentRow();const auto selected=colors.at(at<0?0:at);colors={selected};store();
        }
    });
    QObject::connect(defaults,&QPushButton::clicked,&dialog,usePalette);
    QObject::connect(reset,&QPushButton::clicked,&dialog,[&]{staged.vehicleColorPalettes.erase(id());load();});
    const auto choose=[&](bool append){
        const int at=list->currentRow();if(!append&&at<0)return;
        QColorDialog picker(QColor(QString::fromStdString(colors.at(append?0:at))),&dialog);
        picker.setObjectName("editorVehicleColorPicker");picker.setWindowTitle(tr("editorVehicleColorEdit"));
        picker.setOption(QColorDialog::DontUseNativeDialog);
        if(auto* buttons=picker.findChild<QDialogButtonBox*>()) {
            buttons->button(QDialogButtonBox::Ok)->setText(tr("editorConfirm"));
            buttons->button(QDialogButtonBox::Cancel)->setText(tr("editorCancel"));
        }
        if(picker.exec()!=QDialog::Accepted)return;
        const auto color=picker.currentColor();
        const auto value=color.name().toStdString();
        for(std::size_t i=0;i<colors.size();++i)if(colors[i]==value&&(append||int(i)!=at)){
            error->setText(tr("editorVehicleColorsDuplicate"));return;
        }
        if(append){if(colors.size()>=16)return;colors.push_back(value);}else colors.at(at)=value;
        store();list->setCurrentRow(append?int(colors.size())-1:at);
    };
    QObject::connect(add,&QPushButton::clicked,&dialog,[&]{choose(true);});
    QObject::connect(edit,&QPushButton::clicked,&dialog,[&]{choose(false);});
    QObject::connect(list,&QListWidget::itemDoubleClicked,&dialog,[&]{choose(false);});
    QObject::connect(remove,&QPushButton::clicked,&dialog,[&]{
        if(colors.size()<=2||list->currentRow()<0)return;
        const int at=list->currentRow();colors.erase(colors.begin()+at);store();
        list->setCurrentRow(std::min(at,int(colors.size())-1));
    });
    QObject::connect(footer,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QObject::connect(footer,&QDialogButtonBox::accepted,&dialog,[&]{
        if(!save(staged,path)){error->setText(tr("editorVehicleColorSaveError"));return;}
        dialog.accept();
    });
    load();dialog.resize(640,470);
    if(dialog.exec()!=QDialog::Accepted)return false;
    catalog.vehicleColorPalettes=std::move(staged.vehicleColorPalettes);return true;
}
}
