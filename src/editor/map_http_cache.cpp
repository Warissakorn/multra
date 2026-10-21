#include "map_http_cache.hpp"
#include <algorithm>
#include <limits>
namespace multra::mapHttp {
QByteArray header(const Headers& headers,const QByteArray& name) {
    QByteArray result;
    for(const auto& h:headers)if(h.first.toLower()==name) {
        if(name!="cache-control")return h.second;
        if(!result.isEmpty())result+=", ";
        result+=h.second;
    }
    return result;
}
QDateTime httpDate(const QByteArray& raw,QDateTime received) {
    const auto text=QString::fromLatin1(raw).simplified();auto normalized=text;
    // Qt 6.4's RFC2822 parser accepts numeric offsets but rejects HTTP's GMT.
    if(normalized.endsWith(" GMT")){normalized.chop(4);normalized+=" +0000";}
    auto parsed=QDateTime::fromString(normalized,Qt::RFC2822Date);
    if(parsed.isValid())return parsed.toUTC();
    const auto comma=text.indexOf(',');
    if(comma>=0) {
        const auto parts=text.mid(comma+1).trimmed().split(' ');
        if(parts.size()==3 && parts[2]=="GMT") {
            const auto date=parts[0].split('-');
            if(date.size()==3 && date[2].size()==2) {
                bool ok=false;const int yy=date[2].toInt(&ok);
                if(ok && yy>=0 && yy<=99) {
                    int year=received.date().year()/100*100+yy;
                    if(year>received.date().year()+50)year-=100;
                    parsed=QDateTime::fromString(date[0]+" "+date[1]+" "+QString::number(year)+" "+parts[1]+" +0000",Qt::RFC2822Date);
                }
            }
        }
    } else {
        const auto parts=text.split(' ');
        if(parts.size()==5)parsed=QDateTime::fromString(parts[2]+" "+parts[1]+" "+parts[4]+" "+parts[3]+" +0000",Qt::RFC2822Date);
    }
    return parsed.toUTC();
}
namespace {
QList<QByteArray> directives(const QByteArray& value) {
    QList<QByteArray> result;QByteArray part;bool quoted=false,escaped=false;
    for(char c:value) {
        if(c==',' && !quoted){result.push_back(part.trimmed());part.clear();continue;}
        part+=c;
        if(escaped){escaped=false;continue;}
        if(c=='\\' && quoted)escaped=true;
        else if(c=='"')quoted=!quoted;
    }
    result.push_back(part.trimmed());return result;
}
bool digits(const QByteArray& value) {
    return !value.isEmpty() && std::all_of(value.begin(),value.end(),[](char c){return c>='0' && c<='9';});
}
QByteArray opaqueTag(QByteArray value) {
    value=value.trimmed();if(value.startsWith("W/"))value.remove(0,2);return value;
}
}
QDateTime expiration(const Headers& headers,QDateTime now) {
    int ages=0;qint64 seconds=0;
    for(const auto& directive:directives(header(headers,"cache-control"))) {
        const auto at=directive.indexOf('=');const auto name=directive.left(at<0?directive.size():at).trimmed().toLower();
        if(name=="no-cache" || name=="no-store")return now;
        if(name!="max-age")continue;
        if(++ages>1 || at<0)return now;
        auto value=directive.mid(at+1).trimmed();
        if(value.size()>=2 && value.front()=='"' && value.back()=='"')value=value.mid(1,value.size()-2);
        if(!digits(value))return now;
        bool ok=false;const auto parsed=value.toULongLong(&ok);
        seconds=ok?static_cast<qint64>(std::min(parsed,quint64(31536000))):31536000;
    }
    const auto rawAge=header(headers,"age").trimmed();bool ok=false;const auto parsed=rawAge.toULongLong(&ok);
    const auto age=digits(rawAge)?(ok?std::min(parsed,quint64(31536000)):quint64(31536000)):0;
    const auto date=httpDate(header(headers,"date"),now);
    if(ages) {
        auto result=now.addSecs(std::max(qint64(0),seconds-static_cast<qint64>(age)));
        if(date.isValid())result=std::min(result,date.addSecs(seconds));
        return result;
    }
    const auto rawExpires=header(headers,"expires");
    if(!rawExpires.isEmpty()) {
        const auto expires=httpDate(rawExpires,now);
        if(!expires.isValid())return now;
        const auto lifetime=(date.isValid()?date:now).secsTo(expires);
        return std::min(expires,now.addSecs(std::max(qint64(0),lifetime-static_cast<qint64>(age))));
    }
    return now.addDays(7);
}
Headers validatedHeaders(Headers cached,const Headers& response,QDateTime received) {
    // A successful validation refreshes age. An undated 304 must not retain the
    // old response's Date/Age and immediately expire the freshly validated image.
    cached.erase(std::remove_if(cached.begin(),cached.end(),[](const auto& h){
        const auto name=h.first.toLower();return name=="date" || name=="age";
    }),cached.end());
    cached.erase(std::remove_if(cached.begin(),cached.end(),[&](const auto& old){
        return std::any_of(response.begin(),response.end(),[&](const auto& h){return old.first.toLower()==h.first.toLower();});
    }),cached.end());
    for(const auto& h:response)cached.push_back(h);
    if(header(cached,"date").isEmpty())cached.push_back({"Date",received.toUTC().toString("ddd, dd MMM yyyy HH:mm:ss 'GMT'").toLatin1()});
    return cached;
}
bool matchesValidation(const Headers& cached,const QNetworkRequest& request,const Headers& response) {
    const auto tag=header(cached,"etag"),sent=request.rawHeader("If-None-Match"),returned=header(response,"etag");
    if(!sent.isEmpty())return sent==tag && (returned.isEmpty() || opaqueTag(tag)==opaqueTag(returned));
    const auto modified=header(cached,"last-modified"),since=request.rawHeader("If-Modified-Since");
    return !since.isEmpty() && since==modified && (returned.isEmpty() || (!tag.isEmpty() && opaqueTag(tag)==opaqueTag(returned)));
}
qint64 retryDeadline(const QByteArray& value,qint64 now) {
    const auto raw=value.trimmed();
    if(digits(raw)) {
        bool ok=false;const auto seconds=raw.toULongLong(&ok);const auto maximum=std::numeric_limits<qint64>::max();
        if(!ok || seconds>static_cast<quint64>((maximum-std::max(qint64(0),now))/1000))return maximum;
        return now+static_cast<qint64>(seconds)*1000;
    }
    const auto date=httpDate(raw,QDateTime::fromMSecsSinceEpoch(now,Qt::UTC));
    return date.isValid()?std::max(now,date.toMSecsSinceEpoch()):now;
}
}
