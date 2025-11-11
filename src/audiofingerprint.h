#ifndef AUDIOFINGERPRINT_H
#define AUDIOFINGERPRINT_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AudioFingerprint : public QObject
{
    Q_OBJECT

public:
    explicit AudioFingerprint(QObject *parent = nullptr);
    ~AudioFingerprint(); //cleanup q process
    
    void processMetadataResponse(const QByteArray &responseData);
    void lookupMetadata();

    bool generateFingerprint(const QString &filePath);
 
    
signals:
    void metadataFound(const QJsonObject &metadata);
    void error(const QString &errorMessage); //if fingerprint gen fails
    
private:

    QByteArray m_fingerprint;
    QString m_filePath;
    QNetworkAccessManager *m_networkManager;

    int m_duration;
    bool decodeAudioFile(const QString &filePath, int &duration, QByteArray &fingerprint);
};

#endif // AUDIOFINGERPRINT_H