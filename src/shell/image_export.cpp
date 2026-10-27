#include "image_export.hpp"
#include "../editor/canvas.hpp"
#include "editor_storage.hpp"
#include "dialog_fit.hpp"
#include <QBuffer>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

namespace multra {
void writeNetworkPng(const QString& file,const EditorCanvas& canvas,const CanvasImageOptions& options,const QStringList& protectedFiles) {
    checkExportDestination(file,protectedFiles,"EDIT_IMAGE_PROJECT");
    const auto image=canvas.renderImage(options);QBuffer buffer;buffer.open(QIODevice::WriteOnly);
    if(!image.save(&buffer,"PNG"))throw std::runtime_error("EDIT_IMAGE_WRITE");
    writeEditorBytes(file,buffer.data().toStdString(),"EDIT_IMAGE_WRITE");
}
bool exportNetworkImageDialog(QWidget* parent,const EditorCanvas& canvas,const std::function<QString(const char*)>& text,
    const QStringList& protectedFiles) {
    QDialog dialog(parent);dialog.setObjectName("editorImageExportDialog");dialog.setWindowTitle(text("editorExportImage"));
    auto* body=scrollingBody(dialog);auto* form=new QFormLayout(body);
    auto* scope=new QComboBox(body);scope->setObjectName("editorImageScope");
    scope->addItem(text("editorImageNetwork"));scope->addItem(text("editorImageView"));form->addRow(text("editorImageScope"),scope);
    const auto dimension=[&](const char* key,int value,int minimum) {
        auto* box=new QSpinBox(body);box->setObjectName(key);box->setRange(minimum,8192);box->setValue(value);
        form->addRow(text(key),box);return box;
    };
    auto* width=dimension("editorImageWidth",2048,320);auto* height=dimension("editorImageHeight",1536,240);
    auto* background=new QCheckBox(text("editorImageBackground"),body);background->setObjectName("editorImageBackground");
    background->setChecked(true);form->addRow(background);
    auto* note=new QLabel(text("editorImageNote"),body);note->setWordWrap(true);form->addRow(note);
    auto* error=new QLabel(body);error->setObjectName("editorImageError");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);
    buttons->button(QDialogButtonBox::Save)->setText(text("editorSave"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    addFooter(dialog,buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        try{validateCanvasImageSize({width->value(),height->value()});dialog.accept();}
        catch(const std::exception&){error->setText(text("EDIT_IMAGE_SIZE"));}
    });
    if(dialog.exec()!=QDialog::Accepted)return false;
    const CanvasImageOptions options{scope->currentIndex()==0?CanvasImageScope::network:CanvasImageScope::view,
        {width->value(),height->value()},background->isChecked()};
    QFileDialog destination(parent,text("editorExportImage"),"network.png",text("editorPngFilter"));
    destination.setObjectName("editorImageFileDialog");destination.setAcceptMode(QFileDialog::AcceptSave);destination.setDefaultSuffix("png");
    if(destination.exec()!=QDialog::Accepted || destination.selectedFiles().isEmpty())return false;
    writeNetworkPng(destination.selectedFiles().first(),canvas,options,protectedFiles);return true;
}
}
