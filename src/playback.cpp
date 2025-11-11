#include "playback.h"
#include <QPixmap>
#include <QTime>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <qfileinfo.h>


Playback::Playback(QWidget *parent) : QWidget(parent) 
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    //centre & scale albumart widget
    albumArtLabel = new QLabel(this);
    albumArtLabel->setFixedSize(400, 400); 
    albumArtLabel->setAlignment(Qt::AlignCenter);
    albumArtLabel->setStyleSheet("border: 1px solid black;"); 

    mainLayout->addWidget(albumArtLabel, 0, Qt::AlignCenter);

    //populate song title and artist/album
    songTitleLabel = new QLabel("Song Title", this);
    songTitleLabel->setStyleSheet("font-size: 17px; font-weight: bold;");
    songTitleLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(songTitleLabel);

    artistAlbumLabel = new QLabel("Artist - Album", this);
    artistAlbumLabel->setStyleSheet("font-size: 14px; color: gray;");
    artistAlbumLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(artistAlbumLabel);


    // --- playback controls --- //
    QHBoxLayout *controlsLayout = new QHBoxLayout();

    playPauseButton = new QPushButton("Play", this);
    controlsLayout->addWidget(playPauseButton);

    positionSlider = new QSlider(Qt::Horizontal, this);
    controlsLayout->addWidget(positionSlider);

    currentTimeLabel = new QLabel("0:00", this);
    controlsLayout->addWidget(currentTimeLabel);

    totalTimeLabel = new QLabel("0:00", this);
    controlsLayout->addWidget(totalTimeLabel);

    backButton = new QPushButton("home", this);
    backButton->setStyleSheet("padding: 10px; font-size: 14px;");
    mainLayout->addWidget(backButton, 0, Qt::AlignLeft);
    
    mainLayout->addLayout(controlsLayout);
    // --- playback controls --- //


  
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput); // initalise audio output

    // --- coneections --- //
    connect(backButton, &QPushButton::clicked, this, &Playback::backToMainMenu);
    connect(playPauseButton, &QPushButton::clicked, this, &Playback::playPause);

    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position)
    {
        emit playbackProgress(position / 1000); //seconds
    });

    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Playback::updateDuration);
    connect(positionSlider, &QSlider::sliderMoved, this, &Playback::setPosition);

    setLayout(mainLayout);
}

void Playback::loadSong(const QString &songPath) 
{
    mediaPlayer->setSource(QUrl::fromLocalFile(songPath));

    playPauseButton->setText("play");
    mediaPlayer->stop();

    QString songTitle = QFileInfo(songPath).baseName();

    // --- using taglib to extract & populating song info --- //
    TagLib::FileRef file(songPath.toUtf8().constData());
    if (!file.isNull() && file.tag())
    {
        songTitleLabel->setText(file.tag()->title().toCString(true));
        artistAlbumLabel->setText(QString("%1 - %2")
                                  .arg(file.tag()->artist().toCString(true))
                                  .arg(file.tag()->album().toCString(true)));
    } 
    else 
    {
        songTitleLabel->setText("unknown");
        artistAlbumLabel->setText("unknown Artist / unknown Album");
    }
    // --- using taglib to extract & populating song info --- //


    // get album art 
    QString albumDir = QFileInfo(songPath).absolutePath();
    QStringList possibleCoverNames = {"cover.jpg", "folder.jpg", "album.jpg", "artwork.jpg"};
    QString coverArtPath;

    for (const QString &coverName : possibleCoverNames) // iterate through possible names
    {
        QString potentialPath = albumDir + "/" + coverName;
        if (QFileInfo::exists(potentialPath)) 
        {
            coverArtPath = potentialPath;
            break;
        }
    }

    if (!coverArtPath.isEmpty())
    {
        QPixmap pixmap(coverArtPath);
        albumArtLabel->setPixmap(pixmap.scaled(albumArtLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } 
    else 
    {
        // use placeholder 
        albumArtLabel->setPixmap(QPixmap(":/placeholder.jpg").scaled(albumArtLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this, songTitle](qint64 duration)
    {
        emit playbackStarted(songTitle, duration / 1000); // format to seconds
    });
}

void Playback::playPause() 
{
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) 
    {
        mediaPlayer->pause();
        playPauseButton->setText("play");
    } 
    else 
    {
        mediaPlayer->play();
        playPauseButton->setText("pause");
    }
}

void Playback::updatePosition(qint64 position) 
{
    positionSlider->setValue(position);
    currentTimeLabel->setText(QTime(0, 0).addMSecs(position).toString("m:ss"));
}

void Playback::setPosition(int position) 
{
    mediaPlayer->setPosition(position);
}

void Playback::updateDuration(qint64 duration) 
{
    positionSlider->setMaximum(duration);
    totalTimeLabel->setText(QTime(0, 0).addMSecs(duration).toString("m:ss"));
}

void Playback::togglePlayPause() 
{
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) 
    {
        mediaPlayer->pause();
    } 
    else 
    {
        mediaPlayer->play();
    }
}

void Playback::stopPlayback() 
{
    mediaPlayer->stop();
}

void Playback::seekPosition(int position) {
    mediaPlayer->setPosition(position * 1000); // miliseconds
}