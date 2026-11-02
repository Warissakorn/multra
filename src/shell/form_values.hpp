#pragma once
#include <QString>
#include <QDialog>
#include <functional>
class QDialogButtonBox;
class QDoubleSpinBox;
namespace multra {
class FormDialog : public QDialog {
public:
    using QDialog::QDialog;
    void setValidation(std::function<bool()> validate){validate_=std::move(validate);}
    void accept() override {if(!validate_||validate_())QDialog::accept();}
private:
    std::function<bool()> validate_;
};
// Display rounded numbers without rewriting untouched source precision or out-of-display-range values.
std::function<void()> preserveFormDraft(QWidget*);
void setFormNumber(QDoubleSpinBox*,double);
double formNumber(const QDoubleSpinBox*);
// Commit while the dialog still owns its inputs. A failed validation leaves the form open and editable.
void validateDialog(FormDialog&,QDialogButtonBox*,const std::function<void()>& commit,
                    const std::function<QString(const char*)>& text);
}
