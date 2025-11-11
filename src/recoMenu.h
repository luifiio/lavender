#ifndef RECOMMENDATIONMENU_H
#define RECOMMENDATIONMENU_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QProcess>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

#include <QSqlQuery> 
#include <QSqlError>
#include <QNetworkAccessManager> 
#include <QNetworkRequest>       
#include <QNetworkReply>


class RecommendationMenu : public QWidget 
{
    Q_OBJECT

public:
    explicit RecommendationMenu(QWidget *parent = nullptr);
    void fetchRecommendations(int songId);
  
private:
    QListWidget *albumRecommendationsList;  //return albums based on genre
    QListWidget *artistRecommendationsList;  // based on albums... -> artists!
    QString analyzeAlbumGenre(const QString &albumId);

    QPushButton *backButton; 

    QProcess *pythonProcess; //recoengine.py process 

    // for limiiting API calls 
    int apiCallLimit;
    int apiCallsMade;

    QString currentGenre;  
    QString currentAlbum;  

    void setupDatabase(const QString &dbPath); // redundant
    void initUI();

    void onRecommendationClicked(QListWidgetItem *item);
    
    void fetchReleasesByGenreId(const QString &genreId, const QString &genreName);
    void fetchAlbumDetails(const QString &albumId);

    void fetchAlbumRecommendationsFromAPI(const QString &artist);
    void fetchGenreRecommendationsFromAPI(const QString &genre, bool isRetry = false);
    void fetchReleasesByBrowseMethod(const QString &genre, bool isRetry);

    void fetchAlbumsForGenreArtists(const QList<QPair<QString, QString>> &artists, const QString &genre);

    void displayFallbackOrRetry(const QString &genre, bool isRetry);
    void displayGenreAlbumResults(const QList<QJsonObject>&albums, const QString &genre);
    
    void addFallbackRecommendations(); //offline recomendations 


    void showSongDetails(const QString &filePath);

signals:
    void backToMainMenu();
    void songSelected(int songId, const QString &filePath);

private slots:
    void handlePythonOutput(); 
};

#endif // RECOMMENDATIONMENU_H