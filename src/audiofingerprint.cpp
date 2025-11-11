#include "audiofingerprint.h"

#include <chromaprint.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QCryptographicHash>
#include <QDebug>
#include <QProcess>

extern "C" //stops the compiler from complaining 
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
    #include <libavutil/opt.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/time.h>
    #include <libswresample/swresample.h>
}


AudioFingerprint::AudioFingerprint(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_duration = 0;
}

AudioFingerprint::~AudioFingerprint()
{
    // cleanup qprocesses generated
}

bool AudioFingerprint::generateFingerprint(const QString &filePath)
{
    m_filePath = filePath;
    m_duration = 0; //double work 

    m_fingerprint.clear(); //incase user has done a previous fingerprint
    
    return decodeAudioFile(filePath, m_duration, m_fingerprint); //run decoder and get bool 
}

bool AudioFingerprint::decodeAudioFile(const QString &filePath, int &duration, QByteArray &fingerprint)
{
    QProcess checkProcess;

    checkProcess.start("which", QStringList() << "fpcalc");
    checkProcess.waitForFinished();

    if (checkProcess.exitCode() != 0) //user prob dont have fpcalc
    {
        emit error("get fpcalc!");
        return false;
    }
    

    QFile file(filePath);
    if (!file.exists())
    {
        emit error("song does not exist!: " + filePath);
        return false;
    }
    
    qDebug() << "fingerprinting begun!" << filePath;
    
    // make sure fpcalc runs 
    QProcess testProcess;
    testProcess.start("fpcalc", QStringList() << "-version");
    testProcess.waitForFinished();
    qDebug() << "version:" << testProcess.readAllStandardOutput();
    
    // iniatialse process and capture args
    QProcess process;
    QStringList args;

    args << "-json";
    args << filePath;
    
    qDebug() << "args:" << args.join(" ");
    process.start("fpcalc", args);
    
    // timeout handle
    if (!process.waitForFinished(30000))
    { // 30 sec
        process.kill();
        emit error("timeout");
        return false;
    }
    
    
    if (process.exitCode() != 0) // we messed up
    {
        QString errorOutput = process.readAllStandardError();
        qDebug() << "fpcalc error output:" << errorOutput;

        emit error("fingerprinting failed!: " + errorOutput);
        return false;
    }
    
    QByteArray output = process.readAllStandardOutput();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    
    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << parseError.errorString() << "at offset" << parseError.offset;
        emit error("parsing output failed... " + parseError.errorString());

        return false;
    }
    
    if (!doc.isObject()) // this shouldn't happen but good to see where the errors were occuring...
    {
        qDebug() << "not valid json";
        return false;
    }
    
    QJsonObject obj = doc.object();
    qDebug() << obj.keys(); // see the keys generated
    

    qDebug() <<  obj["fingerprint"].toString().toUtf8(); //make sure values are nice
    qDebug() << qRound(obj["duration"].toDouble());  // duration is a crazy float 
    
    fingerprint = obj["fingerprint"].toString().toUtf8();
    duration = qRound(obj["duration"].toDouble());
    return true; 
}

void AudioFingerprint::lookupMetadata() 
{

    if (m_fingerprint.isEmpty()) //just incase ! lol
    {
        emit error("no fingerprint!");
        return;
    }
    

    if (m_duration <= 0)
    {
        qDebug() << m_duration; // check if duration is kosher
        m_duration = m_duration > 0 ? m_duration : 30; 
    }
    
    QUrl url("https://api.acoustid.org/v2/lookup"); //acousticid base url 
    QUrlQuery query;
    
    //  params 
    query.addQueryItem("client", "GngcI3Divj"); // api key ****(limitation note in report!!!!!)**
    query.addQueryItem("fingerprint", QString::fromUtf8(m_fingerprint));
    query.addQueryItem("duration", QString::number(m_duration));
    query.addQueryItem("meta", "recordings+releasegroups+compress");
    query.addQueryItem("threshold", "0.35"); // low threshold, prevent constant api calls
    
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
    qDebug() << "API request URL: " << url.toString();
    qDebug() << "Using duration:" << m_duration << "seconds";
    qDebug() << "Sending API request...";
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() 
    {
        qDebug() << "api request succesf1ul!";
        
        if (reply->error() == QNetworkReply::NoError) // no errors thrown back by the api, parse response to the songdetail class 
        {
            QByteArray responseData = reply->readAll();
            processMetadataResponse(responseData);
        } 
        else 
        {
            QString errorMsg = reply->errorString();
            qDebug() << errorMsg;
            QByteArray responseData = reply->readAll();
            qDebug() << responseData;
            
            // incase api returned json instance with errors
            QJsonDocument errorDoc = QJsonDocument::fromJson(responseData);
            if (!errorDoc.isNull() && errorDoc.isObject() && errorDoc.object().contains("error"))
            {
                QJsonObject errorObj = errorDoc.object()["error"].toObject();
                QString fullError = errorObj["message"].toString();
                qDebug() << fullError;
                emit error(fullError);
            } 
            else 
            {
                emit error(errorMsg);
            }
        }
        
        reply->deleteLater(); //cleanup
    });
}

void AudioFingerprint::processMetadataResponse(const QByteArray &responseData) 
{
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) //doublework, incase the lookupmetadata json check fails
    {
        emit error("not json...");
        return;
    }

    QJsonObject response = doc.object();
    if (response["status"].toString() != "ok") 
    {
        emit error("api error: " + response["status"].toString());
        return;
    }
  
    qDebug() << doc.toJson(QJsonDocument::Compact); //full response


    if (!response.contains("results") || response["results"].toArray().isEmpty())
    {
        emit error("no results found");
        return;
    }

    QJsonArray fingerprintResults = response["results"].toArray();
    QJsonObject bestMatch = fingerprintResults[0].toObject(); //get best match based on threshold
    
    QJsonObject metadata;
    metadata["results"] = fingerprintResults; 
    
    emit metadataFound(metadata); //emit to songmenu 
}