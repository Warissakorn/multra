#pragma once
#include <QDateTime>
#include <QNetworkCacheMetaData>
#include <QNetworkRequest>
namespace trafficsim::mapHttp {
using Headers=QNetworkCacheMetaData::RawHeaderList;
QByteArray header(const Headers&,const QByteArray& lowerCaseName);
QDateTime httpDate(const QByteArray&,QDateTime received=QDateTime::currentDateTimeUtc());
QDateTime expiration(const Headers&,QDateTime received=QDateTime::currentDateTimeUtc());
Headers validatedHeaders(Headers cached,const Headers& response,QDateTime received=QDateTime::currentDateTimeUtc());
bool matchesValidation(const Headers& cached,const QNetworkRequest&,const Headers& response);
qint64 retryDeadline(const QByteArray& retryAfter,qint64 now);
}
