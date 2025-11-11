#include "mainwindow.h"
#include <QDebug>
#include <QStandardPaths>
#include <QFileDialog>
#include <QDir>
#include <QGuiApplication>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) 
{
  
    setWindowTitle("lavender - n1076024 project");

    setFixedHeight(1000); // scale to users screen (mainly for mainemnu grid layout)
    resize(600, 300); 

    stackedWidget = new QStackedWidget(this);

    // --- call constructors for each menu ---//
    mainMenu = new MainMenu(this);
    albumMenu = new AlbumMenu(this);
    songDetail = new SongDetail(this);
    playback = new Playback(this);
    recommendationMenu = new RecommendationMenu(this);
    // --- call constructors for each menu ---//

    // --- add objects to the stacked widget ---//
    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(albumMenu);
    stackedWidget->addWidget(songDetail);
    stackedWidget->addWidget(recommendationMenu);
    stackedWidget->addWidget(playback);
    // --- add objects to the stacked widget ---//

    setCentralWidget(stackedWidget);

    // --- CONNCECTIONS AND SIGNALS --- //
    connect(mainMenu, &MainMenu::showRecommendationMenu, this, [this]() 
    {
        int currentSongId = 1; // get current song for ofline reccomendations ... 
        showRecommendationMenu(currentSongId);
    });

    connect(songDetail, &SongDetail::playSong, this, [this](const QString &songPath) 
    {
    showPlayback(songPath);
    });

    connect(recommendationMenu, &RecommendationMenu::songSelected,this, [this](int songId, const QString &filePath) 
    {

    playback->loadSong(filePath);
    stackedWidget->setCurrentWidget(playback);
    });

    connect(mainMenu, &MainMenu::showAlbumMenu, this, &MainWindow::showAlbumMenu);
    connect(albumMenu, &AlbumMenu::songSelected, this, &MainWindow::showSongMenu);
    connect(songDetail, &SongDetail::backToMainMenu, this, &MainWindow::returnMainMenu);
    connect(playback, &Playback::backToMainMenu, this, &MainWindow::returnMainMenu);
    connect(recommendationMenu, &RecommendationMenu::backToMainMenu, this, &MainWindow::returnMainMenu);

    // -- playback signals -- //
    connect(playback, &Playback::playbackStarted, mainMenu, &MainMenu::updatePlaybackBar);
    connect(playback, &Playback::playbackProgress, mainMenu, &MainMenu::updatePlaybackProgress);
    connect(playback, &Playback::playbackStopped, mainMenu, [this]()
    {
        mainMenu->updatePlaybackBar("No song playing", 0);
    });

    connect(mainMenu, &MainMenu::playPauseClicked, playback, &Playback::togglePlayPause);
    connect(mainMenu, &MainMenu::stopClicked, playback, &Playback::stopPlayback);
    connect(mainMenu, &MainMenu::seekPosition, playback, &Playback::seekPosition);
    // -- playback signals -- //



    // --- CONNCECTIONS AND SIGNALS --- //

    //file dialog prompt for intial application boot
    QString folder = QFileDialog::getExistingDirectory(this, "select music location");
    if (!folder.isEmpty())
    {
        qDebug() << "folder selected:" << folder;

        // get suitable db location
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dbPath); 
        dbPath += "/lavender.db"; // db file 

        qDebug() << dbPath;

        // // incase db exists, skip scan 
        if (QFile::exists(dbPath))
        {
            qDebug() << "db already exists, skip scan";
        } 
        else 
        {
            LibScan::scanMusicLibrary(folder, dbPath); // scan provided dir 
        }

        mainMenu->loadAlbums(dbPath);
        stackedWidget->setCurrentWidget(mainMenu);

    } 
    else 
    {
        qDebug() << "no folder selected";
    }
}

// --- CONNCECTIONS AND SIGNALS --- //
void MainWindow::returnMainMenu() 
{
stackedWidget->setCurrentWidget(mainMenu);
}


void MainWindow::showMainMenu(const QString &folder) 
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/lavender.db";
    mainMenu->loadAlbums(dbPath);
    stackedWidget->setCurrentWidget(mainMenu);

    qDebug() << "mainmenu loaded";
}

void MainWindow::showPlayback(const QString &songPath) 
{
    playback->loadSong(songPath);
    stackedWidget->setCurrentWidget(playback);
}

void MainWindow::showRecommendationMenu(int songId)
{
    recommendationMenu->fetchRecommendations(songId);
    stackedWidget->setCurrentWidget(recommendationMenu);
}

void MainWindow::showPlayback() 
{
    stackedWidget->setCurrentWidget(playback);

}

void MainWindow::showAlbumMenu(const QString &albumName, const QString &albumPath)
{
    albumMenu->loadAlbum(albumName, albumPath);
    stackedWidget->setCurrentWidget(albumMenu);
}

void MainWindow::showSongMenu(const QString &songPath)
{
    songDetail->loadSong(songPath);
    
    #ifdef Q_OS_MAC // macOS specific code (me) 
        setMenuBar(songDetail->getMenuBar());
    #endif
    
    stackedWidget->setCurrentWidget(songDetail);
}
