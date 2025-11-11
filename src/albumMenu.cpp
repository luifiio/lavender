#include "albumMenu.h"
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <QListWidgetItem>
#include <QPixmap>

AlbumMenu::AlbumMenu(QWidget *parent) : QWidget(parent) 
{
    layout = new QVBoxLayout(this);
    setLayout(layout);

    albumArtLabel = new QLabel(this);
    layout->addWidget(albumArtLabel);

    albumNameLabel = new QLabel(this);
    layout->addWidget(albumNameLabel);

    songListWidget = new QListWidget(this);
    layout->addWidget(songListWidget);

    connect(songListWidget, &QListWidget::itemClicked, this, &AlbumMenu::onSongClicked);
}

void AlbumMenu::loadAlbum(const QString &albumName, const QString &albumPath)
{
    albumNameLabel->setText(albumName);
    songListWidget->clear();

    //get album art 
    QStringList albumArtFiles = {"Folder.jpg", "Front.jpg", "cover.jpg", "cover.png"}; // possible file names

    QPixmap albumArt;
    bool albumArtFound = false; //flag to prompt use of placeholder

    foreach (const QString &fileName, albumArtFiles)
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
        qDebug() << "placeholder used: " << albumName;
        albumArt.load(":/resources/placeholder.jpeg"); // placeholder incase no album art is found
    }

    albumArtLabel->setPixmap(albumArt.scaled(200, 200, Qt::KeepAspectRatio)); // scaling

    QDir dir(albumPath);
    QFileInfoList fileList = dir.entryInfoList(QStringList() << "*.mp3" << "*.flac" << "*.wav", QDir::Files);

    foreach (const QFileInfo &fileInfo, fileList)
    {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName(), songListWidget);
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
    }

    qDebug() << albumName << "with" << fileList.size() << "songs";
}

void AlbumMenu::onSongClicked(QListWidgetItem *item) //prompt to switch to songdetail
{
    QString songPath = item->data(Qt::UserRole).toString();
    emit songSelected(songPath);
}