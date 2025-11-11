#ifndef APIFETCH_H
#define APIFETCH_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ApiFetch : public QObject 
{
    Q_OBJECT

public:
    explicit ApiFetch(QObject *parent = nullptr);
    void fetchMetadata(const QString &artist, const QString &album);

signals:
    void metadataRetrieved(const QString &title, const QString &artist, const QString &album, int year, int track, const QString &genre);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
};

#endif // APIFETCH_H