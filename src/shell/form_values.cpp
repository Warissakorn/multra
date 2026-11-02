#include "form_values.hpp"
#include "editor_interaction.hpp"
#include "../core/validate.hpp"
#include <QDialog>
#include <QComboBox>
#include <QPointer>
#include <vector>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
namespace multra {
std::function<void()> preserveFormDraft(QWidget* root) {
    struct Edit {QPointer<QLineEdit> field;QString text;int cursor;};
    struct Choice {QPointer<QComboBox> field;QVariant data;};
    struct Number {QPointer<QAbstractSpinBox> field;QVariant value;};
    std::vector<Edit> edits;std::vector<Choice> choices;std::vector<Number> numbers;
    if(root) {
        for(auto* field:root->findChildren<QLineEdit*>())
            if(!field->isReadOnly()&&!qobject_cast<QComboBox*>(field->parentWidget()))edits.push_back({field,field->text(),field->cursorPosition()});
        for(auto* field:root->findChildren<QComboBox*>())choices.push_back({field,field->currentData()});
        for(auto* field:root->findChildren<QAbstractSpinBox*>()) {
            if(auto* real=qobject_cast<QDoubleSpinBox*>(field))numbers.push_back({field,real->value()});
            else if(auto* whole=qobject_cast<QSpinBox*>(field))numbers.push_back({field,whole->value()});
        }
    }
    return [edits,choices,numbers]{
        for(const auto& number:numbers)if(number.field) {
            const QSignalBlocker block(number.field);
            if(auto* real=qobject_cast<QDoubleSpinBox*>(number.field.data()))real->setValue(number.value.toDouble());
            else if(auto* whole=qobject_cast<QSpinBox*>(number.field.data()))whole->setValue(number.value.toInt());
        }
        for(const auto& choice:choices)if(choice.field) {
            const QSignalBlocker block(choice.field);const int at=choice.field->findData(choice.data);if(at>=0)choice.field->setCurrentIndex(at);
        }
        for(const auto& edit:edits)if(edit.field) {
            const QSignalBlocker fieldBlock(edit.field),parentBlock(edit.field->parent());
            edit.field->setText(edit.text);edit.field->setCursorPosition(edit.cursor);
        }
    };
}
void setFormNumber(QDoubleSpinBox* field,double number) {
    const QSignalBlocker guard(field);field->setValue(number);
    field->setProperty("formOriginal",number);field->setProperty("formEdited",false);
    if(field->property("formTracked").toBool())return;
    field->setProperty("formTracked",true);
    QObject::connect(field,&QDoubleSpinBox::valueChanged,field,[field]{field->setProperty("formEdited",true);});
}
double formNumber(const QDoubleSpinBox* field) {
    return field->property("formOriginal").isValid()&&!field->property("formEdited").toBool()
        ?field->property("formOriginal").toDouble():field->value();
}
void validateDialog(FormDialog& dialog,QDialogButtonBox* buttons,const std::function<void()>& commit,
                    const std::function<QString(const char*)>& text) {
    auto* error=new ReflowLabel(&dialog);error->setObjectName("editorFormError");error->setWordWrap(true);
    error->setTextFormat(Qt::PlainText);error->setProperty("validationState","invalid");error->hide();
    if(auto* box=qobject_cast<QVBoxLayout*>(dialog.layout()))box->insertWidget(std::max(0,box->count()-1),error);
    else if(auto* form=qobject_cast<QFormLayout*>(dialog.layout()))form->insertRow(std::max(0,form->rowCount()-1),error);
    dialog.setValidation([&dialog,error,commit,text]{
        try {
            for(auto* spin:dialog.findChildren<QAbstractSpinBox*>())if(spin->isEnabled()&&!spin->hasAcceptableInput())
                throw std::invalid_argument("EDIT_INVALID_NUMBER");
            commit();return true;
        } catch(const std::exception& failure) {
            QString message;
            if(const auto* invalid=dynamic_cast<const ValidationError*>(&failure);invalid&&!invalid->issues.empty()) {
                const auto& first=invalid->issues.front();message=text(first.code.c_str());
                if(message.isEmpty())message=QString::fromStdString(first.code);
                message+=" — "+QString::fromStdString(first.path);
            } else {message=text(failure.what());if(message.isEmpty())message=QString::fromUtf8(failure.what());}
            error->setText(message);error->show();error->setAccessibleName(message);return false;
        }
    });
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
}
}
