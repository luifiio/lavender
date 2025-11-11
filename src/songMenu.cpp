#include "songMenu.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QBuffer>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QTimer>

SongDetail::SongDetail(QWidget *parent) : QWidget(parent) 
{
   
   layout = new QVBoxLayout(this);
   menuBar = new QMenuBar(this);

   fingerprintGenerator = new AudioFingerprint(this);

    // file menu //
   fileMenu = menuBar->addMenu("file");
   saveAction = fileMenu->addAction("save Changes");
   saveAction->setShortcut(QKeySequence::Save); // macro
   backAction = fileMenu->addAction("Back");
   backAction->setShortcut(QKeySequence(Qt::Key_Escape)); //macro
   playAction = fileMenu->addAction("play Song");
   playAction->setShortcut(QKeySequence(Qt::Key_Space)); //macro
   
   // edit menu
   editMenu = menuBar->addMenu("edit");
   
   // metadata menu 
   metadataMenu = menuBar->addMenu("metadata");
   fetchMetadataAction = metadataMenu->addAction("ftch Metadata");
   fetchMetadataAction->setShortcut(QKeySequence("Ctrl+M")); 
   fetchAlbumArtAction = metadataMenu->addAction("fetch Album Art");
   fetchAlbumArtAction->setShortcut(QKeySequence("Ctrl+A"));
   uploadAlbumArtAction = metadataMenu->addAction("upload Album Art");
   uploadAlbumArtAction->setShortcut(QKeySequence("Ctrl+U")); 
   

    // fingerprinting interaction (object already initialised)
    identifyByFingerprintAction = metadataMenu->addAction("identify by audio");
    identifyByFingerprintAction->setShortcut(QKeySequence("Ctrl+F"));


   identifyByFingerprintButton = new QPushButton("identify by audio", this);
   QIcon fingerprintIcon(":/resources/fingerprint_icon.png"); // icon for button
   identifyByFingerprintButton->setIcon(fingerprintIcon);
   
   // adjustment for buttons
   QHBoxLayout *fingerprintLayout = new QHBoxLayout();
   fingerprintLayout->addWidget(identifyByFingerprintButton);
   fingerprintLayout->addStretch();
   layout->addLayout(fingerprintLayout);
   

   QHBoxLayout *headerLayout = new QHBoxLayout();
   songInfo = new QLabel("Song Details", this);
   headerLayout->addWidget(songInfo, 1); 

    playButton = new QPushButton("play Song", this);
    headerLayout->addWidget(playButton);
    
    backButton = new QPushButton("back", this); //mainmenu return
    headerLayout->addWidget(backButton);
    
    layout->addLayout(headerLayout);
    
    QTabWidget *tabWidget = new QTabWidget(this); //each submenu in a tab 
    
    //-- METADATA TAB --//
    QWidget *metadataTab = new QWidget(); //metadata tab
    QVBoxLayout *metadataLayout = new QVBoxLayout(metadataTab);
    
    metadataLayout->addWidget(new QLabel("title:", this)); 
    titleEdit = new QLineEdit(this);
    metadataLayout->addWidget(titleEdit);

    metadataLayout->addWidget(new QLabel("artist:", this));
    artistEdit = new QLineEdit(this);
    metadataLayout->addWidget(artistEdit);

    metadataLayout->addWidget(new QLabel("album:", this));
    albumEdit = new QLineEdit(this);
    metadataLayout->addWidget(albumEdit);

    metadataLayout->addWidget(new QLabel("year:", this));
    yearEdit = new QLineEdit(this);
    metadataLayout->addWidget(yearEdit);

    metadataLayout->addWidget(new QLabel("track:", this));
    trackEdit = new QLineEdit(this);
    metadataLayout->addWidget(trackEdit);

    metadataLayout->addWidget(new QLabel("genre:", this));
    genreEdit = new QLineEdit(this);
    metadataLayout->addWidget(genreEdit);
    
    saveButton = new QPushButton("save changes", this);
    metadataLayout->addWidget(saveButton);
    
    tabWidget->addTab(metadataTab, "metadata");
        
    //-- METADATA TAB --//


   // -- ALBUM ART TAB --//
    QWidget *artworkTab = new QWidget();
    QVBoxLayout *artworkLayout = new QVBoxLayout(artworkTab);
    
    albumArtLabel = new QLabel(this);
    albumArtLabel->setFixedSize(200, 200);
    albumArtLabel->setStyleSheet("border: 1px solid black;");
    albumArtLabel->setAlignment(Qt::AlignCenter);
    artworkLayout->addWidget(albumArtLabel);
    
  
    QHBoxLayout *artworkButtonLayout = new QHBoxLayout();  
    uploadAlbumArtButton = new QPushButton("upload cover art", this);
    artworkButtonLayout->addWidget(uploadAlbumArtButton);
    
    fetchAlbumArtButton = new QPushButton("fetch album art", this);
    artworkButtonLayout->addWidget(fetchAlbumArtButton);
    
    artworkLayout->addLayout(artworkButtonLayout);
    
    tabWidget->addTab(artworkTab, "album art");
    // -- ALBUM ART TAB --//

   // -- TECHNICAL TAB --//
    QWidget *technicalTab = new QWidget();
    QVBoxLayout *technicalLayout = new QVBoxLayout(technicalTab);
    
    // Technical metadata displays
    fileTypeLabel = new QLabel("file Type: ", this);
    technicalLayout->addWidget(fileTypeLabel);

    bitRateLabel = new QLabel("bit Rate: ", this);
    technicalLayout->addWidget(bitRateLabel);

    sampleRateLabel = new QLabel("sample Rate: ", this);
    technicalLayout->addWidget(sampleRateLabel);

    durationLabel = new QLabel("duration: ", this);
    technicalLayout->addWidget(durationLabel);

    channelsLabel = new QLabel("channels: ", this);
    technicalLayout->addWidget(channelsLabel);

    codecLabel = new QLabel("codec: ", this);
    technicalLayout->addWidget(codecLabel);

    fileSizeLabel = new QLabel("file Size: ", this);
    technicalLayout->addWidget(fileSizeLabel);

    bitDepthLabel = new QLabel("bit Depth: ", this);
    technicalLayout->addWidget(bitDepthLabel);

    filePathLabel = new QLabel("file Path: ", this);
    technicalLayout->addWidget(filePathLabel);
    
    tabWidget->addTab(technicalTab, "technical info");
    // -- TECHNICAL TAB --//

    //-- LYRICS TAB --//
    QWidget *lyricsTab = new QWidget();
    QVBoxLayout *lyricsLayout = new QVBoxLayout(lyricsTab);
    
    lyricsDisplay = new QTextEdit(this);
    lyricsDisplay->setReadOnly(true);
    lyricsLayout->addWidget(lyricsDisplay);
    
    tabWidget->addTab(lyricsTab, "lyrics");
    //-- LYRICS TAB --//

    //-- SEARCH RESULTS TAB --//
    QWidget *resultsTab = new QWidget();
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsTab);

    fetchMetadataButton = new QPushButton("fetch metadata", this);
    resultsLayout->addWidget(fetchMetadataButton);
    
    resultListWidget = new QListWidget(this);
    resultsLayout->addWidget(new QLabel("select result:", this));
    resultsLayout->addWidget(resultListWidget);
    
    tabWidget->addTab(resultsTab, "search results");
     //-- SEARCH RESULTS TAB --//

    
    layout->addWidget(tabWidget); // add tabs to main layout 
    

    // - SIGNALS AND CONNECTIONS -- //
    connect(playButton, &QPushButton::clicked, this, [this]() 
    {
        emit playSong(currentSongPath);
    });

    connect(resultListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) 
    {

        int index = item->data(Qt::UserRole).toInt(); // get the index from the item
        
      
        if (index >= 0 && index < recordingData.size()) //ensure index is valid
        {
            QJsonObject recording = recordingData[index];
            
          
            QString title = recording.contains("title") ? recording["title"].toString() : "";
            if (!title.isEmpty()) 
            {
                titleEdit->setText(title);
            }
            
            // Update artist
            if (recording.contains("artist-credit") && !recording["artist-credit"].toArray().isEmpty()) 
            {
                QJsonObject artistObj = recording["artist-credit"].toArray()[0].toObject();
                if (artistObj.contains("name"))
                {
                    artistEdit->setText(artistObj["name"].toString());
                }
            }
            
            
            if (recording.contains("releases") && !recording["releases"].toArray().isEmpty()) 
            {
                QJsonObject release = recording["releases"].toArray()[0].toObject(); 
                
                if (release.contains("title")) //update album title 
                {
                    albumEdit->setText(release["title"].toString());
                }
                
                if (release.contains("date")) 
                {
                    QString dateStr = release["date"].toString();
                    if (dateStr.contains("-")) 
                    {
                        yearEdit->setText(dateStr.split("-")[0]); // just year thanks
                    } 
                    else 
                    {
                        yearEdit->setText(dateStr);
                    }
                }
       
                if (release.contains("media") && !release["media"].toArray().isEmpty()) //update track no if available for future use
                {
                    QJsonObject media = release["media"].toArray()[0].toObject();
                    if (media.contains("track-count")) 
                    {
                        trackEdit->setText(QString::number(media["track-count"].toInt()));
                    }
                }
                
                // if release has id, fetch detailed metadata (should ways be the case)
                if (release.contains("id")) 
                {
                    fetchDetailedReleaseMetadata(release["id"].toString());
                }
            }
            
            QStringList genres;
            
            // check genre array 
            if (recording.contains("genres") && !recording["genres"].toArray().isEmpty()) 
            {
                for (const QJsonValue &genreValue : recording["genres"].toArray()) 
                {

                    if (genreValue.isObject() && genreValue.toObject().contains("name")) 
                    {
                        genres.append(genreValue.toObject()["name"].toString());
                    }
                }
            }
            
            //try tag array as fallback method 
            if (genres.isEmpty() && recording.contains("tags") && !recording["tags"].toArray().isEmpty())
            {
                for (const QJsonValue &tagValue : recording["tags"].toArray()) 
                {
                    if (tagValue.isObject() && tagValue.toObject().contains("name"))
                    {
                        genres.append(tagValue.toObject()["name"].toString());
                    }
                }
            }
            
            //updata genre field
            if (!genres.isEmpty())
            {
                genreEdit->setText(genres.join(", "));
            }
            
            songInfo->setText("metadata updated from selected recording."); // prompt user 
            
            // try album art to for efficient of api calls
            if (recording.contains("releases") && !recording["releases"].toArray().isEmpty())
            {
                QJsonObject release = recording["releases"].toArray()[0].toObject();
                if (release.contains("release-group") && release["release-group"].toObject().contains("id")) 
                {
                    QString releaseGroupId = release["release-group"].toObject()["id"].toString();
                    fetchCoverArt(releaseGroupId);
                }
            }
        }
    });

    // -- connections -- //
    connect(backButton, &QPushButton::clicked, this, &SongDetail::onBackButtonClicked);
    connect(saveButton, &QPushButton::clicked, this, &SongDetail::saveMetadata);
    connect(fetchMetadataButton, &QPushButton::clicked, this, &SongDetail::fetchMetadata);
    connect(uploadAlbumArtButton, &QPushButton::clicked, this, &SongDetail::uploadAlbumArtwork);
    connect(fetchAlbumArtButton, &QPushButton::clicked, this, &SongDetail::fetchAlbumArt);

    // fingerprint
    connect(identifyByFingerprintButton, &QPushButton::clicked,this, &SongDetail::identifySongByFingerprint);

    connect(fingerprintGenerator, &AudioFingerprint::metadataFound,this, &SongDetail::handleFingerprintResult); //fix the method name 
    connect(fingerprintGenerator, &AudioFingerprint::error, this, [this](const QString &error) 
    {
        songInfo->setText("Fingerprinting error: " + error);
    });
    // -- connections -- //

    setLayout(layout);
}

void SongDetail::onBackButtonClicked() 
{
    emit backToMainMenu(); // emit signal to return to main menu
}

void SongDetail::loadSong(const QString &songPath) // called when song is selected
{
    currentSongPath = songPath; //get songpath 

    identifyByFingerprintButton->setEnabled(true);
    identifyByFingerprintAction->setEnabled(true); // enable fingerprinting action

    TagLib::FileRef file(songPath.toUtf8().constData()); //get file reference for metadata

    if (!file.isNull() && file.tag()) // if fileref is valid, parse metadata
    {
        TagLib::Tag *tag = file.tag();
        titleEdit->setText(tag->title().toCString(true));
        artistEdit->setText(tag->artist().toCString(true));
        albumEdit->setText(tag->album().toCString(true));
        yearEdit->setText(QString::number(tag->year()));
        trackEdit->setText(QString::number(tag->track()));
        genreEdit->setText(tag->genre().toCString(true));

        // audio properties
        TagLib::AudioProperties *properties = file.audioProperties();
        if (properties) 
        {
            fileTypeLabel->setText("file Type: " + QFileInfo(songPath).suffix().toUpper());
            bitRateLabel->setText("bit Rate: " + QString::number(properties->bitrate()) + " kbps");
            sampleRateLabel->setText("sample Rate: " + QString::number(properties->sampleRate()) + " Hz");
            durationLabel->setText("duration: " + QString::number(properties->length() / 60) + " min " + QString::number(properties->length() % 60) + " sec");

            channelsLabel->setText("channels: stereo"); // change me!
            codecLabel->setText("codec: flac"); // change me!

            fileSizeLabel->setText("file Size: " + QString::number(QFileInfo(songPath).size() / 1024.0 / 1024.0, 'f', 2) + " MB");
            bitDepthLabel->setText("dit Depth: 16-bit"); // change me!
            filePathLabel->setText("file Path: " + songPath);
        }

        // skip album artwork load since id3 is not working
        albumArtLabel->setText("album artwork not available.");

        // placeholder for lyrics
        lyricsDisplay->setText("No lyrics available.");
    } 
    else 
    {
        songInfo->setText("failed to load song");
    }

    qDebug() << "song loaded: " << songPath;
}

void SongDetail::saveMetadata() 
{
    TagLib::FileRef file(currentSongPath.toUtf8().constData()); // create file reference instance of existing song

    if (!file.isNull() && file.tag()) // if valid, alter fields with user input 
    {
        TagLib::Tag *tag = file.tag();
        tag->setTitle(titleEdit->text().toStdString());
        tag->setArtist(artistEdit->text().toStdString());
        tag->setAlbum(albumEdit->text().toStdString());
        tag->setYear(yearEdit->text().toInt());
        tag->setTrack(trackEdit->text().toInt());
        tag->setGenre(genreEdit->text().toStdString());

        if (file.save()) 
        {
            songInfo->setText("metadata saved.");
        } 
        else 
        {
            songInfo->setText("failed to alter metadata.");
        }
    } 
    else 
    {
        songInfo->setText("failed to load song for alteration.");
    }

    qDebug() << "saved metadata:" << currentSongPath;
}

void SongDetail::fetchMetadata() 
{
    qDebug() << "fetching metadata from API...";

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);


    QString title = QUrl::toPercentEncoding(titleEdit->text());
    QString artist = QUrl::toPercentEncoding(artistEdit->text()); // for handling spaces & special chars

    QString apiUrl = QString("https://musicbrainz.org/ws/2/recording?query=title:\"%1\" AND artist:\"%2\"&fmt=json&inc=artist-credits+releases+genres+tags")
                         .arg(title)
                         .arg(artist);
    qDebug() << apiUrl;

 
    QUrl url(apiUrl);
    if (!url.isValid()) 
    {
        qDebug() << "invalid  URL:" << apiUrl;
        songInfo->setText("Invalid  URL.");
        return;
    }

    QNetworkRequest metarequest(url);
    metarequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // user agent header for identification
    metarequest.setRawHeader("User-Agent", "lavender(university project) (n1076024@my.ntu.ac.uk)");

    QNetworkReply *reply = manager->get(metarequest);


    connect(reply, &QNetworkReply::finished, this, [this, reply]() 
    {
        qDebug() << "api request success!";

        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            qDebug() << "API response data:" << responseData;

            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (!jsonDoc.isNull() && jsonDoc.isObject())
            {
                QJsonObject jsonObj = jsonDoc.object();

                if (jsonObj.contains("recordings")) 
                {
                    QJsonArray recordings = jsonObj["recordings"].toArray();
                    resultListWidget->clear();

                    
                    recordingData.clear();
                    if (!recordings.isEmpty()) 
                    {
                        for (const QJsonValue &value : recordings) 
                        {
                            QJsonObject recording = value.toObject();

                            // store for later use.
                            recordingData.append(recording);

                            QString title = recording.contains("title") ? recording["title"].toString() : "Unknown Title";
                            QString artist = "Unknown Artist";

                            if (recording.contains("artist-credit")) 
                            {
                                QJsonArray artistCredit = recording["artist-credit"].toArray();
                                if (!artistCredit.isEmpty())
                                 {
                                    QJsonObject artistObj = artistCredit[0].toObject();
                                    if (artistObj.contains("name")) 
                                    {
                                        artist = artistObj["name"].toString();
                                    }
                                }
                            }

                            // get year if available
                            QString year = "";
                            if (recording.contains("releases") && !recording["releases"].toArray().isEmpty()) 
                            {
                                QJsonObject release = recording["releases"].toArray()[0].toObject();
                                if (release.contains("date"))
                                {
                                    year = release["date"].toString().split("-")[0]; // Get just the year
                                }
                            }

                            QString displayText = QString("%1 - %2").arg(title, artist);
                            if (!year.isEmpty())
                            {
                                displayText += QString(" (%1)").arg(year);
                            }
                            
                            QListWidgetItem* item = new QListWidgetItem(displayText);

                            // keep index for later use
                            item->setData(Qt::UserRole, recordingData.size() - 1);
                            resultListWidget->addItem(item);
                        }

                        songInfo->setText("select a result from the list to update metadata fields.");
                    } 
                    else 
                    {   
                        songInfo->setText("no metadata found.");
                    }
                } 
                else 
                {
                    qDebug() << "invalid response structure";
                    songInfo->setText("invalid response.");
                }
            } 
            else 
            {
                qDebug() << "failed to parse API response";
                songInfo->setText("Failed to parse API response.");
            }
        } 
        else 
        {
            qDebug() << "request error:" << reply->errorString();
            qDebug() << "http:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "server response:" << reply->readAll();
            songInfo->setText("failed to fetch metadata: " + reply->errorString());
        }

        reply->deleteLater();
    });
}

void SongDetail::uploadAlbumArtwork() 
{
    QString imagePath = QFileDialog::getOpenFileName(this, "select album artwork", "", "Images (*.png *.jpg *.jpeg)");
    if (imagePath.isEmpty()) 
    {
        return; 
    }

    QPixmap pixmap(imagePath);
    if (pixmap.isNull())
    {
        songInfo->setText("invalid format!");
        return;
    }

    albumArtLabel->setPixmap(pixmap.scaled(albumArtLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); //keep size respected

    TagLib::FileRef file(currentSongPath.toUtf8().constData());
    if (!file.isNull() && file.tag())
    {
        QFile imageFile(imagePath);
        if (imageFile.open(QIODevice::ReadOnly)) {
            QByteArray imageData = imageFile.readAll();
            imageFile.close();

            songInfo->setText("cover art updated");
        }
        else 
        {
            songInfo->setText("failed to open file.");
        }
    } 
    else {
        songInfo->setText("failed to load album art.");
    }
}

void SongDetail::fetchCoverArt(const QString &releaseGroupId) 
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QString apiUrl = QString("https://coverartarchive.org/release-group/%1/front").arg(releaseGroupId); //thanks to cover art archive
    qDebug() << apiUrl;

    QNetworkRequest request((QUrl(apiUrl)));
    request.setRawHeader("User-Agent", "lavender(university project) (n1076024@my.ntu.ac.uk)");

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() 
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray imageData = reply->readAll();

            QPixmap pixmap;
            pixmap.loadFromData(imageData);
            albumArtLabel->setPixmap(pixmap.scaled(albumArtLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            songInfo->setText("album art fetched and displayed.");

            QString savePath = QFileDialog::getSaveFileName(this, "save cover art", "", "Images (*.png *.jpg *.jpeg)");
            if (!savePath.isEmpty()) 
            {
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly))
                {
                    file.write(imageData);
                    file.close();
                    songInfo->setText("album art dir: " + savePath);
                } 
                else 
                {
                    songInfo->setText("failed to save!");
                }
            }
        } 
        else 
        {
            songInfo->setText("failed to fetch cover art " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void SongDetail::fetchAlbumArt() 
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QString album = QUrl::toPercentEncoding(albumEdit->text());
    QString artist = QUrl::toPercentEncoding(artistEdit->text());

    if (artist.isEmpty() || album.isEmpty()) 
    {
        songInfo->setText("artist or album name is missing!");
        return;
    }

    QString apiUrl = QString("https://musicbrainz.org/ws/2/release-group?query=artist:\"%1\" AND release:\"%2\"&fmt=json")
                         .arg(artist)
                         .arg(album);
    qDebug() << "Constructed API URL:" << apiUrl;

    QNetworkRequest request((QUrl(apiUrl)));

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "lavender(university project) (n1076024@my.ntu.ac.uk)");

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() 
    {
        if (reply->error() == QNetworkReply::NoError) 
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (!jsonDoc.isNull() && jsonDoc.isObject())
            {
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj.contains("release-groups"))
                {
                    QJsonArray releaseGroups = jsonObj["release-groups"].toArray();
                    if (!releaseGroups.isEmpty())
                    {
                        QString releaseGroupId = releaseGroups[0].toObject()["id"].toString();
                        fetchCoverArt(releaseGroupId);
                    }
                    else {
                        songInfo->setText("no album art found.");
                    }
                }
                else {
                    songInfo->setText("invalid response from MusicBrainz.");
                }
            }
            else
            {
                songInfo->setText("failed to parse MusicBrainz response.");
            }
        }
        else {
            songInfo->setText("failed to fetch album art: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void SongDetail::fetchDetailedReleaseMetadata(const QString &releaseId) 
{

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QString apiUrl = QString("https://musicbrainz.org/ws/2/release/%1?fmt=json&inc=recordings+artist-credits+genres+tags")
                         .arg(releaseId);
    
    QNetworkRequest request((QUrl(apiUrl)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() 
    {
        if (reply->error() == QNetworkReply::NoError) 
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            
            if (jsonDoc.isObject())
            {
                QJsonObject release = jsonDoc.object();
                
                // update year / release date
                if (release.contains("date"))
                {
                    QString dateStr = release["date"].toString();
                    if (dateStr.contains("-"))
                    {
                        yearEdit->setText(dateStr.split("-")[0]); // get just year
                    }
                    else
                    {
                        yearEdit->setText(dateStr);
                    }
                }
                
                // update genre
                QStringList genres;
                if (release.contains("genres") && !release["genres"].toArray().isEmpty())
                {
                    for (const QJsonValue &genreValue : release["genres"].toArray())
                    {
                        if (genreValue.isObject() && genreValue.toObject().contains("name"))
                        {
                            genres.append(genreValue.toObject()["name"].toString());
                        }
                    }
                    
                    if (!genres.isEmpty())
                    {
                        genreEdit->setText(genres.join(", "));
                    }
                }
                
                // get speficic track no if possible
                if (release.contains("media") && !release["media"].toArray().isEmpty()) 
                {
                    for (const QJsonValue &mediaValue : release["media"].toArray())
                    {
                        QJsonObject media = mediaValue.toObject();
                        if (media.contains("tracks") && !media["tracks"].toArray().isEmpty())
                        {
                            for (const QJsonValue &trackValue : media["tracks"].toArray())
                            {
                                QJsonObject track = trackValue.toObject();
                                if (track.contains("title") && track["title"].toString() == titleEdit->text())
                                {
                                    if (track.contains("number"))
                                    {
                                        trackEdit->setText(track["number"].toString());
                                    } 
                                    else if (track.contains("position"))
                                    {
                                        trackEdit->setText(track["position"].toString());
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                
                songInfo->setText("detailed release metadata applied.");
            }
        } 
        else 
        {
            qDebug() << reply->errorString();
        }
        reply->deleteLater();
    });
}

//fingerprint func here
void SongDetail::identifySongByFingerprint() 
{
    if (currentSongPath.isEmpty())
    {
        songInfo->setText("no song loaded!");
        return;
    }
    
    if (!fingerprintGenerator)
    {
        qDebug() << "fingergen null";
        songInfo->setText("fingerprint gen not initialized!");
        return;
    }
    
    qDebug() << currentSongPath;
    
    // prompt user 
    songInfo->setText("Generating audio fingerprint... this may take a moment!");
    identifyByFingerprintButton->setEnabled(false);
    identifyByFingerprintAction->setEnabled(false);
    
    // wait cursor 
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    // use timer to reset UI if fingerprinting takes too long
    QTimer::singleShot(60000, this, [this]()
    {
        // If still processing after 60 seconds, reset UI
        if (!identifyByFingerprintButton->isEnabled())
        {
            qDebug() << "finger print gen timed out";
            songInfo->setText("fingerprint generation timed out.");

            identifyByFingerprintButton->setEnabled(true);
            identifyByFingerprintAction->setEnabled(true);

            QApplication::restoreOverrideCursor();
        }
    });
    
   
    bool success = fingerprintGenerator->generateFingerprint(currentSongPath);
    
    if (success)
    {
        qDebug() << "fingerprint generated successfully, starting metadata lookup";
        songInfo->setText("looking up song by audio fingerprint...");
        fingerprintGenerator->lookupMetadata();
    }
    else 
    {
        qDebug() << "gen failed";
        // no need for prompt 
        
        // Re-enable controls
        identifyByFingerprintButton->setEnabled(true);
        identifyByFingerprintAction->setEnabled(true);
        QApplication::restoreOverrideCursor();
    }
}

void SongDetail::handleFingerprintResult(const QJsonObject &metadata)
{
    qDebug() << "handling fingerprint result";
    
    // enable controls incase false
    identifyByFingerprintButton->setEnabled(true);
    identifyByFingerprintAction->setEnabled(true);
    QApplication::restoreOverrideCursor();
    
    // clear any prior results
    resultListWidget->clear();
    recordingData.clear();
    
    try 
    {
        if (!metadata.contains("results"))
        {
            qDebug() << "no results";
            songInfo->setText("no results found!");
            return;
        }
        
        QJsonArray results = metadata["results"].toArray();
        if (results.isEmpty()) 
        {
            qDebug() << "empty results array";
            songInfo->setText("no matching results found");
            return;
        }

        QJsonObject bestResult;

        for (const QJsonValue &resultValue : results)
        {
            QJsonObject result = resultValue.toObject();
            if (result.contains("recordings") && !result["recordings"].toArray().isEmpty())
            {
                bestResult = result;
                break;
            }
        }
        
        if (bestResult.isEmpty())
        {
            qDebug() << "no recordings";
            songInfo->setText("no recordings found in the results");
            return;
        }
        
        QJsonArray recordings = bestResult["recordings"].toArray();
        
        if (recordings.isEmpty()) 
        {
            songInfo->setText("no recording information found");
            return;
        }
        
        songInfo->setText("matches fount based on audio fingerprint!");
        qDebug() << recordings.size() << "recordings";
        
        for (const QJsonValue &value : recordings) 
        {
            if (!value.isObject()) {
                qDebug() << "not json object";
                continue;
            }
            
            QJsonObject recording = value.toObject();
            recordingData.append(recording);
            
            QString title = recording.contains("title") ? recording["title"].toString() : "Unknown Title";
            
            // get artist
            QString artist = "Unknown Artist";
            if (recording.contains("artists")) 
            {
                QJsonArray artists = recording["artists"].toArray();
                if (!artists.isEmpty()) 
                {
                    QJsonObject artistObj = artists[0].toObject();
                    if (artistObj.contains("name")) 
                    {
                        artist = artistObj["name"].toString();
                    }
                }
            }
            
            // get release info
            QString release = "";
            if (recording.contains("releasegroups"))
            {
                QJsonArray releaseGroups = recording["releasegroups"].toArray();
                if (!releaseGroups.isEmpty())
                {
                    QJsonObject releaseGroup = releaseGroups[0].toObject();
                    if (releaseGroup.contains("title"))
                    {
                        release = releaseGroup["title"].toString();
                    }
                }
            }
            
            // format text for display
            QString displayText = QString("%1 - %2").arg(title, artist);
            if (!release.isEmpty())
            {
                displayText += QString(" (%1)").arg(release);
            }
            qDebug() << displayText;
            
            QListWidgetItem* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, recordingData.size() - 1);

            resultListWidget->addItem(item);
        }
        

        resultListWidget->setVisible(true); //init results
    }
    catch (const std::exception &e) 
    {
        qDebug() << e.what();
        songInfo->setText("error processing results");
    }
    catch (...) 
    {
        qDebug() << "unknown exception";
        songInfo->setText("unexpected error processing results");
    }
}