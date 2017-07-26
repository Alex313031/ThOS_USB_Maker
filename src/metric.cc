#include "metric.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>

#include "config.h"
#include "log.h"

namespace gondar {

namespace {

QNetworkAccessManager* getNetworkManager() {
  static QNetworkAccessManager manager;
  return & manager;
}

std::string getMetricString(Metric metric) {
  switch (metric) {
    case Metric::Use:  return "use";
    // not sure we want to crash the program on a bad metric lookup
    default:   return "unknown";
  }
}

}

void SendMetric(Metric metric) {
    // use QString's equality check
    if (QString("notset") == METRICS_API_KEY) {
        // all production builds should sent metrics
        LOG_WARNING << "not sending metrics!";
        return;
    }
    std::string metricStr = getMetricString(metric);
    QNetworkAccessManager * manager = getNetworkManager();
    QUrl url("https://gondar-metrics.neverware.com/prod");
    QJsonObject json;
    // TODO: use a persistent UUID across a session, and potentially even
    // across multiple runs
    QString id = QUuid::createUuid().toString();
    json["identifier"] = id;
    json.insert("metric", QString::fromStdString(metricStr));
    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("x-api-key"),
                         METRICS_API_KEY);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    QJsonDocument doc(json);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    manager->post(request, QByteArray(strJson.toUtf8()));
}

}
