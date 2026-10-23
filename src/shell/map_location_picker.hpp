#pragma once
#include <QString>
#include <filesystem>
#include <functional>
class QWidget;
class QFormLayout;
class QDoubleSpinBox;
namespace multra {
using MapText=std::function<QString(const char*)>;
// Optional, searchable location shortcuts. Selection fills the editable coordinate
// fields; no document or camera is changed until the owning dialog is confirmed.
// A missing catalog leaves manual coordinates available.
// The returned check rejects unfinished search text, so confirmation cannot apply
// a previously selected location under a newly typed name.
std::function<bool()> addMapLocationPicker(QFormLayout*,QWidget*,const std::filesystem::path& catalog,
    const QString& language,const MapText&,QDoubleSpinBox* longitude,QDoubleSpinBox* latitude);
}
