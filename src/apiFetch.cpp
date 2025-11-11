#include "apiFetch.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

ApiFetch::ApiFetch(QObject *parent) : QObject(parent) 
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiFetch::onReplyFinished);
}

void ApiFetch::fetchMetadata(const QString &artist, const QString &album) 
{
    QUrl url("https://musicbrainz.org/ws/2/release/");
    QUrlQuery query;
    query.addQueryItem("query", QString("artist:%1 AND release:%2").arg(artist, album));
    query.addQueryItem("fmt", "json");
    url.setQuery(query);

    QNetworkRequest request(url);
    networkManager->get(request);
}

void ApiFetch::onReplyFinished(QNetworkReply *reply) 
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray releases = jsonObj["releases"].toArray();

        if (!releases.isEmpty()) 
        {
            QJsonObject release = releases.first().toObject();
            QString title = release["title"].toString();
            QString artist = release["artist-credit"].toArray().first().toObject()["name"].toString();
            QString album = release["title"].toString();
            int year = release["date"].toString().left(4).toInt();
            int track = 0; // You can update this to fetch track number if needed
            QString genre = ""; // You can update this to fetch genre if needed

            emit metadataRetrieved(title, artist, album, year, track, genre);
        }
    } 
    else 
    {
        qWarning() << "API request failed:" << reply->errorString();
    }

    reply->deleteLater();
}