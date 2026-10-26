#pragma once
#include <QJsonObject>
#include <QString>
#include <stdexcept>
#include <utility>
namespace multra {
struct CommandError : std::runtime_error {
    QString code;
    QJsonObject details;
    CommandError(const char* code,const char* message,QJsonObject details = {})
        :std::runtime_error(message),code(code),details(std::move(details)) {}
};
}
