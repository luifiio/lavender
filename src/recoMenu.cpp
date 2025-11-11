#include "recoMenu.h"
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCoreApplication>
#include <QTabWidget>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QFont>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QRegularExpression>  

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>


RecommendationMenu::RecommendationMenu(QWidget *parent) : QWidget(parent) 
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // -- title labels -- //
    QLabel *titleLabel = new QLabel("recommendations for you!", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // two tabs (genre & artist)
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // -- album tabs based on genre -- //
    QWidget *genreBasedTab = new QWidget();
    QVBoxLayout *genreBasedLayout = new QVBoxLayout(genreBasedTab);

    QLabel *genreBasedLabel = new QLabel("albums based on genres in your library:", genreBasedTab);
    genreBasedLayout->addWidget(genreBasedLabel);

    albumRecommendationsList = new QListWidget(genreBasedTab); 
    genreBasedLayout->addWidget(albumRecommendationsList);

    tabWidget->addTab(genreBasedTab, "album Recommendations");
     // -- album tabs based on genre -- //

    // -- more albums based on artists -- //
    QWidget *artistTab = new QWidget();
    QVBoxLayout *artistLayout = new QVBoxLayout(artistTab);

    QLabel *artistLabel = new QLabel("artist albums you may like", artistTab);
    artistLayout->addWidget(artistLabel);

    artistRecommendationsList = new QListWidget(artistTab);
    artistLayout->addWidget(artistRecommendationsList);

    tabWidget->addTab(artistTab, "artist recommendations");
    // -- more albums based on artists -- //

    mainLayout->addWidget(tabWidget);
    

    backButton = new QPushButton("Back to Main Menu", this);
    mainLayout->addWidget(backButton);


    // -- connections -- //
    connect(backButton, &QPushButton::clicked, this, &RecommendationMenu::backToMainMenu);
    connect(albumRecommendationsList, &QListWidget::itemClicked, this, &RecommendationMenu::onRecommendationClicked);
    connect(artistRecommendationsList, &QListWidget::itemClicked, this, &RecommendationMenu::onRecommendationClicked);
    // -- connections -- //
    
    setLayout(mainLayout);
    
    // seting values for api limit 
    apiCallLimit = 20;
    apiCallsMade = 0;

    currentGenre = "";
    currentAlbum = "";
    pythonProcess = nullptr;
}


void RecommendationMenu::fetchRecommendations(int songId) 
{
    qDebug() << "fetchRecommendations for: " << songId;
    
    // init db connection if one doesn't exist
    if (!QSqlDatabase::database("lavender_connection").isValid()) 
    {
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/lavender.db";
        qDebug() << dbPath;
        setupDatabase(dbPath);
    }
    
    QSqlDatabase db = QSqlDatabase::database("lavender_connection");
    if (!db.isOpen()) 
    {
        qDebug() << "db not opened";
        return;
    }
    
    // get song in question 
    QSqlQuery songQuery(db);
    songQuery.prepare("SELECT name, artist, genre, album, album_id FROM songs WHERE id = :id");
    songQuery.bindValue(":id", songId);
    
    if (!songQuery.exec() || !songQuery.next())
    {
        qDebug() << "db query failed:" << songQuery.lastError().text();
        return;
    }
    
    //get song details from query
    QString songName = songQuery.value(0).toString();
    QString artistName = songQuery.value(1).toString();
    currentGenre = songQuery.value(2).toString();
    currentAlbum = songQuery.value(3).toString();
    int albumId = songQuery.value(4).toInt();
    
    qDebug() << songName << artistName << currentGenre << currentAlbum;
    
    
    if (!pythonProcess) 
    {
        pythonProcess = new QProcess(this);

        connect(pythonProcess, &QProcess::readyReadStandardOutput, this, &RecommendationMenu::handlePythonOutput);
        connect(pythonProcess, &QProcess::readyReadStandardError, this, [this]() 
        {
            QByteArray output = pythonProcess->readAllStandardError();
            qDebug() << "stderr:" << output;
        });
    } 
    else if (pythonProcess->state() == QProcess::Running) // if process exists get rid of it ...
    {
        pythonProcess->terminate();
        pythonProcess->waitForFinished(1000);
    }
    
    // incase user has checked the reccomendations before
    albumRecommendationsList->clear();
    artistRecommendationsList->clear();
    
    // prompt user that we are processing
    albumRecommendationsList->addItem("processing...");
    artistRecommendationsList->addItem("processing...");
    
    // prepare data
    QSqlQuery allSongsQuery(db);
    if (!allSongsQuery.exec("SELECT id, name, artist, genre, album, album_id, path FROM songs")) 
    {
        qDebug() << allSongsQuery.lastError().text();

        return;
    }
    
    QStringList pythonData;

    while (allSongsQuery.next()) //iterate through all songs 
    {
        int id = allSongsQuery.value(0).toInt();
        int albumId = allSongsQuery.value(5).toInt();

        QString name = allSongsQuery.value(1).toString();
        QString artist = allSongsQuery.value(2).toString();

        QString genre = allSongsQuery.value(3).toString();
        QString album = allSongsQuery.value(4).toString();

        QString filePath = allSongsQuery.value(6).toString();
        
        // genre fallback
        if (genre.isEmpty() && !filePath.isEmpty() && QFile::exists(filePath)) 
        {
            TagLib::FileRef fileRef(filePath.toUtf8().constData());
            if (!fileRef.isNull() && fileRef.tag()) 
            {
                genre = QString::fromUtf8(fileRef.tag()->genre().toCString(true)).trimmed();
            }
        }
        
        // formatting 
        pythonData.append(QString("%1|%2|%3|%4|%5|%6|%7")
                          .arg(id)
                          .arg(name.replace("|", " "))
                          .arg(artist.replace("|", " "))
                          .arg(genre.replace("|", " "))
                          .arg(album.replace("|", " "))
                          .arg(albumId)
                          .arg(filePath.replace("|", " ")));
    }
    
    // py script
    QString pythonScript = QCoreApplication::applicationDirPath() + "/scripts/recoEngine.py";
    QStringList arguments;
    arguments << pythonScript << QString::number(songId) << QString::number(albumId);
    
    qDebug() << pythonScript << arguments; //check arguments are correct
    pythonProcess->start("python3", arguments);
    
    if (!pythonProcess->waitForStarted(3000)) // 30 seconds i beleive 
    {
        qDebug() << pythonProcess->errorString();
        return;
    }
    
    QByteArray inputData = pythonData.join("\n").toUtf8();

    pythonProcess->write(inputData);
    pythonProcess->closeWriteChannel(); 
}

void RecommendationMenu::handlePythonOutput() 
{
    QByteArray output = pythonProcess->readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output).trimmed();
    
    //incase user has previously used reccomendation menu 
    albumRecommendationsList->clear();  
    artistRecommendationsList->clear(); 

    // process python output 
    QStringList lines = outputStr.split("\n");
    bool readingRecommendations = false;

    QString jsonData;
    
    foreach (const QString &line, lines) {
        if (line == "RECOMMENDATIONS_BEGIN") // macros allow us to see where the python process is at
        {
            readingRecommendations = true;
            continue;
        }
        else if (line == "RECOMMENDATIONS_END") 
        {
            readingRecommendations = false;
            continue;
        }
        
        if (readingRecommendations) 
        {
            jsonData += line + "\n";
        }
    }
    
    if (jsonData.isEmpty()) // if python process does not return anything
    {
        QListWidgetItem *item = new QListWidgetItem("no valid reccomendations found");
        item->setForeground(Qt::gray);

        albumRecommendationsList->addItem(item);
        artistRecommendationsList->addItem(item);

        return;
    }
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData.toUtf8());
    if (jsonDoc.isNull()) 
    {
        QListWidgetItem *errorItem = new QListWidgetItem("error processing recommendations");
        errorItem->setForeground(Qt::red);

        albumRecommendationsList->addItem(errorItem);
        return;
    }

    // -- ARTIST RECOMMENDATIONS -- //
    QJsonObject resultObj = jsonDoc.object();
    
    QSet<QString> processedArtists; // for tracking artists to prevent duplicates
    
   
    if (resultObj.contains("artist_recommendations")) 
    {
        QJsonArray artistRecs = resultObj["artist_recommendations"].toArray();
        qDebug() << artistRecs.size();


        //int debugCount = qMin(3, artistRecs.size()); //for debugging to check json 
        //for (int i = 0; i < debugCount; i++) 
        //{
        //QJsonObject artist = artistRecs[i].toObject();
        //QString artistName = artist["artist"].toString();
        //QString genre = artist["genre"].toString();
        //qDebug() << "  Artist recommendation" << artistName << genre << ")";
        //}

        
        QListWidgetItem *header = new QListWidgetItem("artists based on your library");

        header->setForeground(Qt::blue);
        header->setFlags(Qt::NoItemFlags);

        artistRecommendationsList->addItem(header);
        
        for (const QJsonValue &value : artistRecs) 
        {
            QJsonObject artist = value.toObject();
            QString artistName = artist["artist"].toString();
            
      
            if (processedArtists.contains(artistName)) //if artist already processed
            {
                continue;
            }
            
            processedArtists.insert(artistName);
            QString genre = artist["genre"].toString();
            
            QString displayText = QString("%1 (Genre: %2)").arg(artistName, genre);
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, artistName); //when user wants to see artist 
            artistRecommendationsList->addItem(item);
        }
        
        
        if (!artistRecs.isEmpty())
        {
            // get random artist rather than first one in index

            int randomIndex = 0;
            if (artistRecs.size() > 1) 
            {
                randomIndex = QRandomGenerator::global()->bounded(artistRecs.size());
            }

            QJsonObject randomArtist = artistRecs[randomIndex].toObject();
            QString artistName = randomArtist["artist"].toString();

            fetchAlbumRecommendationsFromAPI(artistName); //get reccomendations based on artist
        }
    }
    // -- ARTIST RECOMMENDATIONS -- //

    // -- GENRE / ALBUM RECOMMENDATIONS -- //
    QSet<QString> processedAlbums;
    
    
    if (resultObj.contains("genre_recommendations")) {
        QJsonArray genreRecs = resultObj["genre_recommendations"].toArray();
        qDebug() << genreRecs.size();
        
        int debugCount = qMin(3, genreRecs.size());
        for (int i = 0; i < debugCount; i++) // get each genre reccomendatiom 
        {
            QJsonObject song = genreRecs[i].toObject();
            QString title = song["title"].toString();
            QString artist = song["artist"].toString();
            QString album = song["album"].toString();
            QString genre = song["genre"].toString();
            qDebug() << album << artist << genre;
        }
        // ...exist

        QListWidgetItem *header = new QListWidgetItem("albums you might like based on your library"); 
        
        header->setForeground(Qt::blue);
        header->setFlags(Qt::NoItemFlags);
        albumRecommendationsList->addItem(header);
        
  
        QStringList validGenres; // store valid genres for api use 
        
        for (const QJsonValue &value : genreRecs) 
        {
            QJsonObject song = value.toObject();

            int songId = song["id"].toInt();

            QString title = song["title"].toString();
            QString artist = song["artist"].toString();
            QString album = song["album"].toString();
            QString genre = song["genre"].toString();
            
            // unique key for album
            QString albumKey = artist + " - " + album;
            
           
            if (processedAlbums.contains(albumKey)) //if key in processed albums
            {
                continue;
            }
            
            processedAlbums.insert(albumKey);
            
          
            qDebug() << album << artist << genre;

            QString displayText = QString("%1 by %2").arg(album, artist);
            QListWidgetItem *item = new QListWidgetItem(displayText);

            item->setToolTip(QString("Genre: %1").arg(genre)); 
            item->setData(Qt::UserRole, songId); 
            
            albumRecommendationsList->addItem(item);
            
            // Collect valid genres for API calls
            if (!genre.isEmpty() && genre.toLower() != "unknown") //should be handled by libscan anyway
            {
                validGenres.append(genre);
            }
        }

        if (!validGenres.isEmpty()) 
        {
            int randomIndex = 0;
            if (validGenres.size() > 1)
            {
                randomIndex = QRandomGenerator::global()->bounded(validGenres.size());
            }

            QString randomGenre = validGenres[randomIndex];
            fetchGenreRecommendationsFromAPI(randomGenre);
        }
    }

    qDebug() << "recomendations done";
}

void RecommendationMenu::fetchAlbumRecommendationsFromAPI(const QString &artist)
{
    apiCallsMade++;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    QString encodedArtist = QUrl::toPercentEncoding(artist);
    
    QString apiUrl;
    if (artist.length() < 4)
    {
        apiUrl = QString("https://musicbrainz.org/ws/2/release?query=artist:\"%1\" AND status:official&limit=10&fmt=json")
                     .arg(encodedArtist);
    } 
    else 
    {
        apiUrl = QString("https://musicbrainz.org/ws/2/release?query=artist:\"%1\"&limit=10&fmt=json")
                     .arg(encodedArtist);
    }

    qDebug() << apiUrl;

    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
   
    if (apiCallsMade > 1) //if api called already made, gen a thread and chill for abit
    {
        QThread::msleep(2000);
    }

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, artist, manager]() 
    {
        qDebug() << "request finished for " << artist;

    
        artistRecommendationsList->clear(); //incase initialised before
        
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject jsonObj = jsonDoc.object();

            if (jsonObj.contains("releases")) 
            {
                QJsonArray releases = jsonObj["releases"].toArray();
                qDebug() << "album n0:" << releases.size();
                
                QListWidgetItem *header = new QListWidgetItem(QString("Albums by %1").arg(artist));

                header->setForeground(Qt::blue);
                header->setFlags(Qt::NoItemFlags);
                artistRecommendationsList->addItem(header);

                QSet<QString> addedAlbums;
                
                for (const QJsonValue &value : releases) 
                {
                    QJsonObject release = value.toObject();
                    QString title = release["title"].toString();
                    
                    if (addedAlbums.contains(title)) // skip duplicates
                    {
                        continue;
                    }
                    
                    addedAlbums.insert(title);
                    QString releaseId = release["id"].toString();
                    
                    
                    QString releaseDate = ""; //get date if available
                    if (release.contains("date")) 
                    {
                        releaseDate = " (" + release["date"].toString() + ")";
                    }
                    
                    QString displayText = title + releaseDate;
                    
                    QListWidgetItem *item = new QListWidgetItem(displayText);
                    item->setData(Qt::UserRole, releaseId); 
                    artistRecommendationsList->addItem(item);
                }
                
                if (releases.isEmpty() || addedAlbums.isEmpty()) 
                {
                    QListWidgetItem *item = new QListWidgetItem("no albums found");
                    item->setForeground(Qt::gray);
                    artistRecommendationsList->addItem(item);
                }
            } 
            else 
            {
                qDebug() << "api response returned note.";
                QListWidgetItem *item = new QListWidgetItem(QString("no albums found for %1").arg(artist));
                item->setForeground(Qt::gray);
                artistRecommendationsList->addItem(item);
            }
        } 
        else 
        {
            qDebug() << reply->errorString();
            QListWidgetItem *item = new QListWidgetItem(QString("problem fetching albums: %1").arg(reply->errorString()));
            item->setForeground(Qt::red);
            artistRecommendationsList->addItem(item);
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

// infinite loop on this shit fix it 
void RecommendationMenu::addFallbackRecommendations() 
{
    QListWidgetItem *header = new QListWidgetItem("fallback! - albums you may enjoy...");

    header->setForeground(Qt::blue);
    header->setFlags(Qt::NoItemFlags);

    albumRecommendationsList->clear();
    albumRecommendationsList->addItem(header);
    
    struct FallbackAlbum 
    {
        QString title;
        QString artist;
        QString year;
        QString genre;
    };
    
    QList<FallbackAlbum> popularAlbums = //gen for rock 
    {
        {"Thriller", "Michael Jackson", "1982", "Pop"},
        {"The Dark Side of the Moon", "Pink Floyd", "1973", "Rock"},
        {"Back in Black", "AC/DC", "1980", "Hard Rock"},
        {"Abbey Road", "The Beatles", "1969", "Rock"},
        {"Rumours", "Fleetwood Mac", "1977", "Rock"},
        {"Kind of Blue", "Miles Davis", "1959", "Jazz"},
        {"Purple Rain", "Prince", "1984", "Pop"},
        {"Nevermind", "Nirvana", "1991", "Grunge"},
        {"Hotel California", "Eagles", "1976", "Rock"},
        {"Appetite for Destruction", "Guns N' Roses", "1987", "Hard Rock"}
    };
    

    for (const FallbackAlbum &album : popularAlbums) // populate list 
    {
        QString displayText = QString("%1 by %2 (%3)").arg(album.title, album.artist, album.year);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setToolTip(QString("Genre: %1").arg(album.genre));
        albumRecommendationsList->addItem(item);
    }
    
    QListWidgetItem *note = new QListWidgetItem("curated based on popular albums");
    note->setForeground(Qt::gray);
    note->setTextAlignment(Qt::AlignCenter);

    albumRecommendationsList->addItem(note);
}

void RecommendationMenu::fetchGenreRecommendationsFromAPI(const QString &genre, bool isRetry) 
{
    apiCallsMade++;
    
    QString cleanGenre = genre;

    // normalise (mainly removing vers numbers)
    cleanGenre = cleanGenre.replace(QRegularExpression("\\.[0-9]+$"), "");
    cleanGenre = cleanGenre.replace(QRegularExpression("[^a-zA-Z0-9\\s]"), " ");
    cleanGenre = cleanGenre.trimmed();

    // falback msg
    if (isRetry && (cleanGenre == "rock" || cleanGenre == "pop")) 
    {
        addFallbackRecommendations();
        return;
    }

    // fallback if clean fails
    if (cleanGenre.length() < 3) 
    {
        cleanGenre = "pop";
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QString encodedGenre = QUrl::toPercentEncoding(cleanGenre);
    
    QString apiUrl = QString("https://musicbrainz.org/ws/2/genre/%1?fmt=json").arg(encodedGenre);

    qDebug() << apiUrl;

    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
   
    if (apiCallsMade > 1) // get thread goin if api called already made
    {
        QThread::msleep(2000);
    }

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, genre, cleanGenre, manager, isRetry]() 
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject genreObj = jsonDoc.object();
        
            // if genre is valid, it will contain an ID
            if (genreObj.contains("id")) 
            {
                QString genreId = genreObj["id"].toString();
                QString genreName = genreObj["name"].toString();
                qDebug() << genreId << "for: " << genreName;
                
            
                fetchReleasesByGenreId(genreId, genreName);
            }
            else 
            {
                fetchReleasesByBrowseMethod(cleanGenre, isRetry);
            }
        } 
        else if (reply->error() == QNetworkReply::ContentNotFoundError || reply->errorString().contains("server replied")) 
        {
        qDebug() << "browse method initalised"; // fallback to browser method 
        fetchReleasesByBrowseMethod(cleanGenre, isRetry);
        } 
        else 
        {
        qDebug() << reply->errorString();
        fetchReleasesByBrowseMethod(cleanGenre, isRetry);
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void RecommendationMenu::fetchReleasesByBrowseMethod(const QString &genre, bool isRetry)
{
    
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    QString mbUrl = QString("https://musicbrainz.org/ws/2/artist?query=tag:%1 AND type:group AND country:US&limit=30&fmt=json")
    .arg(QUrl::toPercentEncoding(genre));
    
    QString apiUrl = mbUrl; // was going  to implement multiple api solutions but out of scope...
    
    qDebug() << "Artist lookup URL:" << apiUrl;

    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
    QThread::msleep(2000); // respect api call limits 
    
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, genre, manager, isRetry]() 
    {
        albumRecommendationsList->clear();
        
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject rootObj = jsonDoc.object();
    
            QList<QPair<QString, QString>> artists; // name, id pairs
            
            // musicbrainz json
            if (rootObj.contains("artists")) 
            {
                QJsonArray artistArray = rootObj["artists"].toArray();
                qDebug() << artistArray.size() << "artists for genre:" << genre;
                
                for (const QJsonValue &value : artistArray) 
                {
                    QJsonObject artistObj = value.toObject();
                    QString name = artistObj["name"].toString();
                    QString id = artistObj["id"].toString();
                    artists.append(qMakePair(name, id));
                }
            }
            
            if (artists.isEmpty()) // no artists found
            {
                displayFallbackOrRetry(genre, isRetry);
                reply->deleteLater();
                manager->deleteLater();
                return;
            }
            
            fetchAlbumsForGenreArtists(artists, genre);
        } 
        else 
        {
            qDebug() << reply->errorString();
            displayFallbackOrRetry(genre, isRetry);
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void RecommendationMenu::fetchAlbumsForGenreArtists(const QList<QPair<QString, QString>> &artists, const QString &genre) 
{
    if (artists.isEmpty()) 
    {
        displayFallbackOrRetry(genre, false);
        return;
    }
    
    int artistsToUse = qMin(5, artists.size());
    QList<QPair<QString, QString>> selectedArtists = artists.mid(0, artistsToUse);
    
    // for handling requests
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    int *completedRequests = new int(0);  // for tracking completed requests

    QList<QJsonObject> *albumResults = new QList<QJsonObject>();
    
    for (const auto &artistPair : selectedArtists) 
    {
        QString artistName = artistPair.first;
        QString artistId = artistPair.second;
        
        QString apiUrl = QString("https://musicbrainz.org/ws/2/release-group?artist=%1&type=album&fmt=json").arg(artistId);
        
        qDebug() << artistName << " " << artistId << " ";
        
        QNetworkRequest request{QUrl(apiUrl)}; 
        request.setHeader(QNetworkRequest::UserAgentHeader, "lavender(university project) (n1076024@my.ntu.ac.uk)");
        
        QThread::msleep(2000);// delay between requests
        
        QNetworkReply *reply = manager->get(request);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, manager, artistName, genre,completedRequests, albumResults, artistsToUse,selectedArtists]() 
        {
            (*completedRequests)++; //increment inside reply to prevent false positive calls
            
            if (reply->error() == QNetworkReply::NoError) 
            {
                QByteArray responseData = reply->readAll();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
                QJsonObject rootObj = jsonDoc.object();
                
                if (rootObj.contains("release-groups")) 
                {
                    QJsonArray releaseGroups = rootObj["release-groups"].toArray();
                    qDebug() << releaseGroups.size() << "albums for artist:" << artistName;
                    
                   // have to limit to 5 due to api call limits 
                    int albumsToTake = qMin(5, releaseGroups.size());
                    for (int i = 0; i < albumsToTake; i++) 
                    {
                        QJsonObject album = releaseGroups[i].toObject();
                        albumResults->append(album);
                    }
                }
            } 
            else 
            {
                qDebug() << reply->errorString();
            }
            
            reply->deleteLater();
            
           
            if (*completedRequests >= artistsToUse) //check all requests were made
            {
                displayGenreAlbumResults(*albumResults, genre);
                
                delete completedRequests;
                delete albumResults;
                manager->deleteLater();
            }
        });
    }
}

//-- remove this function when done --//
void RecommendationMenu::displayGenreAlbumResults(const QList<QJsonObject> &albums, const QString &genre) 
{
    albumRecommendationsList->clear();
    
    QListWidgetItem *header = new QListWidgetItem(QString("albums in %1 Genre").arg(genre));

    header->setForeground(Qt::blue);
    header->setFlags(Qt::NoItemFlags);
    albumRecommendationsList->addItem(header);
    
    if (albums.isEmpty())
    {
        QListWidgetItem *item = new QListWidgetItem(QString("no albums found for %1").arg(genre));
        item->setForeground(Qt::gray);
        albumRecommendationsList->addItem(item);
        return;
    }
    
    
    QMap<QString, QString> artistIdToName; //map for known artists
    artistIdToName["c1aa2ec9-53e7-4d90-8d36-bac75832e986"] = "The Supremes";
    artistIdToName["d8df96ae-8fcf-4997-b3e6-e5d1aaf0f69e"] = "The Temptations";
    artistIdToName["e5257dc5-1edd-4fca-b7e6-1158e00522c8"] = "The Jacksons";
    artistIdToName["535afeda-2538-435d-9dd1-5e10be586774"] = "Earth, Wind & Fire";
    artistIdToName["9e2d3f58-0653-4007-bcb7-1746fcdd9363"] = "The Drifters";
    artistIdToName["d6652e7b-33fe-49ef-8336-4c863b4f996f"] = "The E Street Band";
    artistIdToName["83b9cbe7-9857-49e2-ab8e-b57b01038103"] = "Pearl Jam";
    artistIdToName["6faa7ca7-0d99-4a5e-bfa6-1fd5037520c6"] = "Grateful Dead";
    artistIdToName["e01646f2-2a04-450d-8bf2-0d993082e058"] = "Phish";
    artistIdToName["5b11f4ce-a62d-471e-81fc-a69a8278c7da"] = "Nirvana";
    
    QMap<QString, QList<QJsonObject>> albumsByArtist;
    QMap<QString, QString> albumTitleToArtist; // for debugging please remove me 
    
    // index through results
    for (int i = 0; i < albums.size(); i++) 
    {
        const QJsonObject &albumObj = albums[i];
        QString title = albumObj["title"].toString();
        QString artist = "Unknown Artist";
        QString artistId = "";
        
        // identify artist through api 
        if (albumObj.contains("artist-credit") && albumObj["artist-credit"].isArray())
        {
            QJsonArray artistCredits = albumObj["artist-credit"].toArray();
            if (!artistCredits.isEmpty())
            {
                QJsonObject artistObj = artistCredits[0].toObject();
                
                // check qmap 
                if (artistObj.contains("name")) 
                {
                    artist = artistObj["name"].toString();
                }
                
                // correlate artist id 
                if (artistObj.contains("artist") && artistObj["artist"].isObject())
                {
                    QJsonObject nestedArtistObj = artistObj["artist"].toObject();
                    if (nestedArtistObj.contains("id")) {
                        artistId = nestedArtistObj["id"].toString();
                        
                      //fallback 1
                        if (artistIdToName.contains(artistId))
                        {
                            artist = artistIdToName[artistId];
                        }
                     // use json result for final fallback
                        else if (nestedArtistObj.contains("name"))
                        {
                            artist = nestedArtistObj["name"].toString();
                        }
                    }
                }
            }
        }

        // -- this part is for debugging & should not be triggered in the main functionality--//

        if (artist == "Unknown Artist") //guess work for known artists in my collection
        {
            // e street band
            if (title.contains("Hammersmith Odeon") || 
                title.contains("Capitol Theatre") || 
                title.contains("Brendan Byrne Arena") ||
                title.contains("Darkness Tour")) {
                artist = "Bruce Springsteen & The E Street Band";
            }
            else if (title == "Sigma Oasis" || title == "Billy Breathes" || 
                     title == "A Picture of Nectar" || title == "Live Bait, Vol. 01") 
                     {
                artist = "Phish";
            }
            // grateful dead
            else if (title == "Terrapin Station" || title == "Blues for Allah" || 
                     title == "The Golden Road (1965–1973)" || 
                     title == "Stayin' Alive" || title == "Eternally Grateful")
                     {
                artist = "Grateful Dead";
            }
            // nirvana
            else if (title == "Incesticide" || title == "Nevermind" || 
                     title.contains("Greatest Hits") || title == "Wipeout")
                     {
                artist = "Nirvana";
            }
            //pearl jam
            else if (title == "Gigaton" || title == "Dark Matter" || 
                     title.contains("Lightning Bolt") || title.contains("Yonkers"))
                     {
                artist = "Pearl Jam";
            }
            // soul
            else if (title == "Millennium" || title == "Diana" || 
                     title.contains("Where Did Our Love Go")) {
                artist = "The Supremes";
            }
            else if (title == "Cloud Nine" || title == "Temptations Sing Smokey" || 
                     title.contains("Wish It Would Rain")) {
                artist = "The Temptations";
            }
            else if (title == "Moving Violation" || title == "Destiny" || 
                     title.contains("Victory")) {
                artist = "The Jacksons";
            }
            else if (title == "That's the Way of the World" || title == "Gratitude" || 
                     title.contains("All 'n All")) {
                artist = "Earth, Wind & Fire";
            }
            else if (title == "Save the Last Dance for Me" || title == "The Good Life With The Drifters" || 
                     title.contains("The Drifters' Golden Hits")) {
                artist = "The Drifters";
            }
        }
        
    
        if (artist == "Unknown Artist") 
        {
            int albumIndex = i;
            int artistsToUse = 5; 
            int albumsPerArtist = 5; 
            
            int artistIndex = albumIndex / albumsPerArtist;
            if (artistIndex < artistsToUse) {
                switch (artistIndex)
                {
                    case 0: artist = "The Supremes"; break;
                    case 1: artist = "The Temptations"; break;
                    case 2: artist = "The Jacksons"; break;
                    case 3: artist = "Earth, Wind & Fire"; break;
                    case 4: artist = "The Drifters"; break;
                    default: artist = "Unknown Artist";
                }
            }
        }
        // -- this part is for debugging & should not be triggered in the main functionality--//

    
        albumTitleToArtist[title] = artist;
        
        if (artist == "Unknown Artist") 
        {
            qDebug() << "still Unknown Artist for album:" << title << "with data:" << QJsonDocument(albumObj).toJson(QJsonDocument::Compact);
        } 
        else 
        {
            qDebug() << "Found artist:" << artist << "for album:" << title;
        }
        
        albumsByArtist[artist].append(albumObj);
    }
    
    int displayedCount = 0;
    QSet<QString> displayedArtists;
    
    // loop through each album 
    for (auto it = albumsByArtist.begin(); it != albumsByArtist.end(); ++it)
    {
        QString artist = it.key();
        const QList<QJsonObject> &artistAlbums = it.value();
        
        int albumsDisplayedForArtist = 0;
        
        for (const QJsonObject &albumObj : artistAlbums) 
        {
            if (albumsDisplayedForArtist >= 3) break; // max due to api call limits
            
            QString title = albumObj["title"].toString();
            
            QString year = "";
            if (albumObj.contains("first-release-date") && !albumObj["first-release-date"].toString().isEmpty()) 
            {
                year = " (" + albumObj["first-release-date"].toString().left(4) + ")";
            }
            
            QString displayText = QString("%1 by %2%3").arg(title, artist, year);
            QListWidgetItem *item = new QListWidgetItem(displayText);
            
            if (albumObj.contains("id")) 
            {
                item->setData(Qt::UserRole, albumObj["id"].toString());
            }
            
            albumRecommendationsList->addItem(item);

            displayedCount++;
            albumsDisplayedForArtist++;

            displayedArtists.insert(artist);
            
           
            if (displayedCount >= 15) break;
        }
        
        // prevent exceedomg 15 items 
        if (displayedCount >= 15) break;
    }
    
    if (displayedCount == 0) 
    {
        QListWidgetItem *item = new QListWidgetItem(QString("no albums found for %1").arg(genre));
        item->setForeground(Qt::gray);
        albumRecommendationsList->addItem(item);
    }
    

    qDebug() << displayedArtists.size() << " artists for genre:" << genre;
}
//-- remove this function when done --//

void RecommendationMenu::displayFallbackOrRetry(const QString &genre, bool isRetry) 
{
    if (!isRetry) 
    {
        QString newGenre = (genre.toLower() == "rock") ? "pop" : "rock";

        fetchGenreRecommendationsFromAPI(newGenre, true);
    } 
    else 
    {
        qDebug() << "using fallback recommendations";
        addFallbackRecommendations();
    }
}

void RecommendationMenu::setupDatabase(const QString &dbPath) 
{ 
    // check if db connection already exists 
    if (QSqlDatabase::contains("lavender_connection")) 
    {
        qDebug() << "db connection already exists.";
        return;
    }

    // init new connection 
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "lavender_connection");
    db.setDatabaseName(dbPath);

    if (!db.open())
     {
        qDebug() << db.lastError().text();
        return;
    }

    qDebug() << "db  opened at:" << dbPath;
}

void RecommendationMenu::initUI() 
{
    // connect signals and slots
    connect(albumRecommendationsList, &QListWidget::itemClicked, this, &RecommendationMenu::onRecommendationClicked);
    connect(artistRecommendationsList, &QListWidget::itemClicked, this, &RecommendationMenu::onRecommendationClicked);
}

// limited info, this wasnt implemented fully :(
void RecommendationMenu::onRecommendationClicked(QListWidgetItem *item) {


    if (item->flags() == Qt::NoItemFlags) 
    {
        return;
    }
    
    // make sure instance exists
    if (item->listWidget() == artistRecommendationsList) 
    {
        // check if it is an id or artist name
        QVariant data = item->data(Qt::UserRole);
        if (data.toString().contains("-")) 
        { // if hyphens -> musicbrazi
            fetchAlbumDetails(data.toString());
        } 
        else 
        {
            // It's an artist name, fetch albums
            fetchAlbumRecommendationsFromAPI(data.toString());
        }
    }
    else if (item->listWidget() == albumRecommendationsList) 
    {
        int songId = item->data(Qt::UserRole).toInt();
        if (songId > 0) 
        {
            // fetch from db 
            QSqlDatabase db = QSqlDatabase::database("lavender_connection");
            if (db.isOpen())
            {
                QSqlQuery query(db);
                query.prepare("SELECT path FROM songs WHERE id = :id"); // Changed from filePath to path
                query.bindValue(":id", songId);
                
                if (query.exec() && query.next()) 
                {
                    QString filePath = query.value(0).toString();
                    if (!filePath.isEmpty() && QFile::exists(filePath)) 
                    {
                        // Show song details using TagLib
                        showSongDetails(filePath);
                    } 
                    else 
                    {
                        QMessageBox::information(this, "song details", 
                            "song: " + item->text() + "\nfile not found: " + filePath);
                    }
                } 
                else 
                {
                    QMessageBox::information(this, "song Details", 
                        "selected song: " + item->text() + "\ncouldn't retrieve file path from db");
                }
            }
        } else {
            QMessageBox::information(this, "song Details", "selected song: " + item->text()); // limited info, this wasnt implemented fully :(
        }
    }
}

void RecommendationMenu::fetchAlbumDetails(const QString &mbid) 
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    QString apiUrl = QString("https://musicbrainz.org/ws/2/release/%1?inc=recordings+artist-credits&fmt=json").arg(mbid);

    qDebug() << apiUrl;

    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
    QThread::msleep(1000); //thread for rate limits

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply,manager]() 
    {

        if (reply->error() == QNetworkReply::NoError) 
        {
            QByteArray responseData = reply->readAll();
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject album = jsonDoc.object();
            
            QString albumTitle = album["title"].toString();
            QString artist = "Various Artists";
            
            if (album.contains("artist-credit") && album["artist-credit"].isArray()) 
            {
                QJsonArray artistCredits = album["artist-credit"].toArray();
                if (!artistCredits.isEmpty()) 
                {
                    artist = artistCredits[0].toObject()["name"].toString();
                }
            }
            
            
            QDialog *detailsDialog = new QDialog(this); //dialog for albums 
            detailsDialog->setWindowTitle(albumTitle + " - " + artist);
            
            QVBoxLayout *layout = new QVBoxLayout(detailsDialog);
            
            QLabel *titleLabel = new QLabel(albumTitle, detailsDialog);

            QFont titleFont = titleLabel->font();
            titleFont.setPointSize(16);
            titleFont.setBold(true);
            titleLabel->setFont(titleFont);

            layout->addWidget(titleLabel);
            
            QLabel *artistLabel = new QLabel("Artist: " + artist, detailsDialog);
            layout->addWidget(artistLabel);
            
            if (album.contains("date"))
            {
                QLabel *dateLabel = new QLabel("release date: " + album["date"].toString(), detailsDialog);
                layout->addWidget(dateLabel);
            }
            
            if (album.contains("media") && album["media"].isArray()) 
            {
                int trackCount = 0;
                QJsonArray media = album["media"].toArray();
                for (const QJsonValue &mediaValue : media)
                {
                    QJsonObject mediaObj = mediaValue.toObject();
                    if (mediaObj.contains("track-count"))
                    {
                        trackCount += mediaObj["track-count"].toInt();
                    }
                }
                
                QLabel *trackCountLabel = new QLabel("tracks: " + QString::number(trackCount), detailsDialog);
                layout->addWidget(trackCountLabel);
            }
            
            QLabel *tracksLabel = new QLabel("track List:", detailsDialog);
            tracksLabel->setFont(titleFont);
            layout->addWidget(tracksLabel);
            
            QListWidget *trackList = new QListWidget(detailsDialog);
            
            if (album.contains("media") && album["media"].isArray())
            {
                QJsonArray media = album["media"].toArray();
                for (const QJsonValue &mediaValue : media) {
                    QJsonObject mediaObj = mediaValue.toObject();
                    if (mediaObj.contains("tracks") && mediaObj["tracks"].isArray())
                    {
                        QJsonArray tracks = mediaObj["tracks"].toArray();
                        for (const QJsonValue &trackValue : tracks) {
                            QJsonObject trackObj = trackValue.toObject();
                            QString trackTitle = trackObj["title"].toString();
                            int position = trackObj["position"].toInt();
                            
                            QListWidgetItem *item = new QListWidgetItem(QString("%1. %2").arg(position).arg(trackTitle));
                            trackList->addItem(item);
                        }
                    }
                }
            }
            
            layout->addWidget(trackList);
            
            QPushButton *closeButton = new QPushButton("Close", detailsDialog);
            connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
            layout->addWidget(closeButton);
            
            detailsDialog->setLayout(layout);
            detailsDialog->resize(400, 500);
            detailsDialog->exec();
        }
        else
        {
            qDebug() << "network error:" << reply->errorString();
            QMessageBox::warning(this, "error", "failed to fetch album details: " + reply->errorString());
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void RecommendationMenu::showSongDetails(const QString &filePath) 
{
    TagLib::FileRef fileRef(filePath.toUtf8().constData());
    if (fileRef.isNull() || !fileRef.tag()) {
        QMessageBox::information(this, "Song Details", 
            "Could not read tag information from: " + filePath);
        return;
    }
    
    // Create a dialog to display song details
    QDialog *detailsDialog = new QDialog(this);
    detailsDialog->setWindowTitle("Song Details");
    
    QVBoxLayout *layout = new QVBoxLayout(detailsDialog);
    
    // Extract tag information
    TagLib::Tag *tag = fileRef.tag();
    QString title = QString::fromUtf8(tag->title().toCString(true));
    QString artist = QString::fromUtf8(tag->artist().toCString(true));
    QString album = QString::fromUtf8(tag->album().toCString(true));
    QString genre = QString::fromUtf8(tag->genre().toCString(true));
    int year = tag->year();
    int track = tag->track();
    
    // Get audio properties
    if (fileRef.audioProperties()) {
        TagLib::AudioProperties *props = fileRef.audioProperties();
        int lengthInSeconds = props->lengthInSeconds();
        int bitrate = props->bitrate();
        
        // Format the length as mm:ss
        int minutes = lengthInSeconds / 60;
        int seconds = lengthInSeconds % 60;
        QString length = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        
        // Add audio properties
        QLabel *titleLabel = new QLabel("<b>Title:</b> " + title, detailsDialog);
        QLabel *artistLabel = new QLabel("<b>Artist:</b> " + artist, detailsDialog);
        QLabel *albumLabel = new QLabel("<b>Album:</b> " + album, detailsDialog);
        QLabel *genreLabel = new QLabel("<b>Genre:</b> " + genre, detailsDialog);
        QLabel *yearLabel = new QLabel("<b>Year:</b> " + (year > 0 ? QString::number(year) : "Unknown"), detailsDialog);
        QLabel *trackLabel = new QLabel("<b>Track:</b> " + (track > 0 ? QString::number(track) : "Unknown"), detailsDialog);
        QLabel *lengthLabel = new QLabel("<b>Length:</b> " + length, detailsDialog);
        QLabel *bitrateLabel = new QLabel("<b>Bitrate:</b> " + QString::number(bitrate) + " kbps", detailsDialog);
        
        layout->addWidget(titleLabel);
        layout->addWidget(artistLabel);
        layout->addWidget(albumLabel);
        layout->addWidget(genreLabel);
        layout->addWidget(yearLabel);
        layout->addWidget(trackLabel);
        layout->addWidget(lengthLabel);
        layout->addWidget(bitrateLabel);
        
        // Try to get additional properties
        TagLib::PropertyMap properties = fileRef.file()->properties();
        if (!properties.isEmpty()) {
            QLabel *additionalLabel = new QLabel("<b>Additional Properties:</b>", detailsDialog);
            layout->addWidget(additionalLabel);
            
            QListWidget *propList = new QListWidget(detailsDialog);
            for (TagLib::PropertyMap::ConstIterator it = properties.begin(); it != properties.end(); ++it) {
                QString key = QString::fromUtf8(it->first.toCString(true));
                
                // Skip properties we've already shown
                if (key.toLower() == "title" || key.toLower() == "artist" || 
                    key.toLower() == "album" || key.toLower() == "genre" ||
                    key.toLower() == "date" || key.toLower() == "tracknumber") {
                    continue;
                }
                
                QStringList values;
                for (TagLib::StringList::ConstIterator vIt = it->second.begin(); vIt != it->second.end(); ++vIt) {
                    values.append(QString::fromUtf8(vIt->toCString(true)));
                }
                
                QListWidgetItem *item = new QListWidgetItem(key + ": " + values.join(", "));
                propList->addItem(item);
            }
            
            if (propList->count() > 0) {
                layout->addWidget(propList);
            } else {
                propList->deleteLater();
            }
        }
    }
    
    // Add close button
    QPushButton *closeButton = new QPushButton("Close", detailsDialog);
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
    layout->addWidget(closeButton);
    
    detailsDialog->setLayout(layout);
    detailsDialog->resize(400, 500);
    detailsDialog->exec();
}

void RecommendationMenu::fetchReleasesByGenreId(const QString &genreId, const QString &genreName)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    QString apiUrl = QString("https://musicbrainz.org/ws/2/release-group?genre=%1&type=album&limit=30&fmt=json").arg(genreId);

    qDebug() << genreId << "(" << genreName << ")";
    qDebug() << apiUrl;

    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "lavender(university project) (n1076024@my.ntu.ac.uk)");
    
    QThread::msleep(2000); //delay for api limits
    
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, genreId, genreName]()
    {
        albumRecommendationsList->clear();
        
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject jsonObj = jsonDoc.object();
            
            if (jsonObj.contains("release-groups")) {
                QJsonArray releaseGroups = jsonObj["release-groups"].toArray();
                
                QList<QJsonObject> albums;
                for (const QJsonValue &value : releaseGroups)
                {
                    albums.append(value.toObject());
                }
                
                displayGenreAlbumResults(albums, genreName);
            } 
            else // use browser method
            {
                fetchReleasesByBrowseMethod(genreName, false);
            }
        } 
        else 
        {
            qDebug()<< reply->errorString();
            fetchReleasesByBrowseMethod(genreName, false);
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

QString RecommendationMenu::analyzeAlbumGenre(const QString &albumId) 
{

    QSqlDatabase db = QSqlDatabase::database("lavender_connection");
    if (!db.isOpen())
    {
        qDebug() << "db is not open!";
        return QString();
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT path FROM songs WHERE album_id = :albumId");
    query.bindValue(":albumId", albumId);
    
    if (!query.exec())
    {
        qDebug() << query.lastError().text();
        return QString();
    }
    
    QMap<QString, int> genreCounts;
    int totalSongs = 0;
    
    while (query.next()) 
    {
        QString filePath = query.value(0).toString();
        if (filePath.isEmpty() || !QFile::exists(filePath)) 
        {
            continue;
        }
        
        totalSongs++;
       
        TagLib::FileRef fileRef(filePath.toUtf8().constData());
        if (!fileRef.isNull() && fileRef.tag()) //get genre tag 
        {
            QString genre = QString::fromUtf8(fileRef.tag()->genre().toCString(true)).trimmed();
     
            if (genre.isEmpty())  
            {
                TagLib::PropertyMap properties = fileRef.file()->properties();
                if (properties.contains("GENRE")) 
                {
                    TagLib::StringList genres = properties["GENRE"];
                    if (!genres.isEmpty())
                     {
                        genre = QString::fromUtf8(genres.front().toCString(true)).trimmed();
                    }
                }
            }
            
            genre = genre.toLower(); //normalize genre
            
            // Count this genre
            if (!genre.isEmpty()) 
            {
                genreCounts[genre]++;
            }
        }
    }
    
    QString dominantGenre;
    int maxCount = 0;
    
    for (auto it = genreCounts.begin(); it != genreCounts.end(); ++it) 
    {
        if (it.value() > maxCount)
        {
            maxCount = it.value();
            dominantGenre = it.key();
        }
    }
    
    qDebug() << dominantGenre;
    
    return dominantGenre;
}