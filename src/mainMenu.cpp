#include "mainMenu.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMouseEvent>
#include <QPixmap>
#include <QVBoxLayout>

MainMenu::MainMenu(QWidget *parent) : QWidget(parent) 
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); //resizing for grid layout 
 
    layout = new QGridLayout(); //grid layout for album art / instances
    mainLayout->addLayout(layout); 

  
    recommendationButton = new QPushButton("Recommendations", this);
    playbackButton = new QPushButton("Playback", this);

    layout->addWidget(recommendationButton, 0, 1);
    layout->addWidget(playbackButton, 0, 2);

    connect(recommendationButton, &QPushButton::clicked, this, &MainMenu::onRecommendationButtonClicked);
    connect(playbackButton, &QPushButton::clicked, this, &MainMenu::onPlaybackButtonClicked);

    // ---- playback bar stuff ------- //

    QHBoxLayout *playbackLayout = new QHBoxLayout();

    currentSongLabel = new QLabel("n/a", this);
    playbackLayout->addWidget(currentSongLabel);

    playbackSlider = new QSlider(Qt::Horizontal, this);
    playbackSlider->setRange(0, 100); 
    playbackLayout->addWidget(playbackSlider);

    playPauseButton = new QPushButton("Play", this);
    playbackLayout->addWidget(playPauseButton);

    stopButton = new QPushButton("Stop", this);
    playbackLayout->addWidget(stopButton);

    mainLayout->addLayout(playbackLayout);

    connect(playPauseButton, &QPushButton::clicked, this, &MainMenu::playPauseClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainMenu::stopClicked);
    connect(playbackSlider, &QSlider::sliderMoved, this, &MainMenu::seekPosition);

    // ---- playback bar stuff ------- //


    mainLayout->setSpacing(10); // grid spacing 

    setLayout(mainLayout);

    qDebug() << "mainmenu initialized";
}

void MainMenu::loadAlbums(const QString &dbPath)
{
    qDebug() << "loadAlbums func called" << dbPath;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open()) 
    {
        qWarning() << "db could not be opened:";
        return;
    }

    QSqlQuery query(db);

    if (!query.exec("SELECT name, path FROM albums")) 
    {
        qWarning() << "Failed to execute query:" << query.lastError().text();
        return;
    }

    int row = 1; // prevent overlapping
    int col = 0;

    while (query.next())
    {
        QString albumName = query.value(0).toString();
        QString albumPath = query.value(1).toString();
        qDebug() << albumName << " path: " << albumPath;


        QStringList albumArtFiles = {"Folder.jpg", "Front.jpg", "cover.jpg", "cover.png"};
        QPixmap albumArt;
        bool albumArtFound = false; //prompt to use placeholder 

        foreach (const QString &fileName, albumArtFiles) //check if an instance of album art exists 
        {
            QString albumArtPath = albumPath + "/" + fileName;
            albumArt.load(albumArtPath);
            if (!albumArt.isNull()) 
            {
                albumArtFound = true;
                break;
            }
        }

        if (!albumArtFound)
        {
            qDebug() << "using placeholder for: " << albumName;
            albumArt.load(":/resources/placeholder.jpeg"); //get placeholder image
        }

        //------ cusotmised label for album art -------//
        ClickableLabel *albumLabel = new ClickableLabel(this);
        albumLabel->setPixmap(albumArt.scaled(150, 150, Qt::KeepAspectRatio)); // scaling to fit grid 
        albumLabel->setAlignment(Qt::AlignCenter);
        albumLabel->setToolTip(albumName); // if user hovers over album, show name 
        albumLabel->setProperty("albumName", albumName);
        albumLabel->setProperty("albumPath", albumPath);
        connect(albumLabel, &ClickableLabel::clicked, this, &MainMenu::onAlbumClicked);

        layout->addWidget(albumLabel, row, col);
        //------ cusotmised label for album art -------//


        col++; // increment column for next album
        if (col >= 4) 
        { 
            col = 0;
            row++;
        }
    }
    db.close();
    qDebug() << "albums loaded";
    this->update(); //update layout to show laoded albums
}

//----------- connections / signals ---------------//
void MainMenu::onRecommendationButtonClicked() 
{
    emit showRecommendationMenu();
}

void MainMenu::onPlaybackButtonClicked()
{
    qDebug() << "Playback button clicked.";
    emit showPlayback();
}

void MainMenu::onAlbumClicked() 
{
    ClickableLabel *albumLabel = qobject_cast<ClickableLabel *>(sender());
    if (albumLabel) {
        QString albumName = albumLabel->property("albumName").toString();
        QString albumPath = albumLabel->property("albumPath").toString();
        emit showAlbumMenu(albumName, albumPath);
    }
}

void MainMenu::updatePlaybackBar(const QString &songTitle, int duration) 
{
    currentSongLabel->setText(songTitle);
    playbackSlider->setRange(0, duration); // update slider
    playbackSlider->setValue(0);           // reset 
}

void MainMenu::updatePlaybackProgress(int position) 
{
    playbackSlider->setValue(position); // get current position
}

//----------- connections / signals ---------------//
