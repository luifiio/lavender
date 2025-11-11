#ifndef SONGDETAIL_H
#define SONGDETAIL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QJsonObject>  
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include "audiofingerprint.h"

class SongDetail : public QWidget {
    Q_OBJECT

public:
    explicit SongDetail(QWidget *parent = nullptr);
    void loadSong(const QString &songPath);

    QMenuBar* getMenuBar() const 
    { 
        return menuBar; 
    }

signals:
    void backToMainMenu();
    void playSong(const QString &songPath);

private slots:
    void identifySongByFingerprint();
    void handleFingerprintResult(const QJsonObject &metadata);
    void onBackButtonClicked();
    void saveMetadata();
    void fetchMetadata();
    void uploadAlbumArtwork();
    void fetchAlbumArt();
    void fetchCoverArt(const QString &releaseGroupId);
    void fetchDetailedReleaseMetadata(const QString &releaseId);

private:
    AudioFingerprint *fingerprintGenerator;

    QMenuBar *menuBar;
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *metadataMenu;

    QAction *saveAction;
    QAction *backAction;
    QAction *playAction;
    QAction *fetchMetadataAction;
    QAction *fetchAlbumArtAction;
    QAction *uploadAlbumArtAction;
    QAction *identifyByFingerprintAction;

    QVBoxLayout *layout;

    QLineEdit *titleEdit;
    QLineEdit *artistEdit;
    QLineEdit *albumEdit;
    QLineEdit *yearEdit;
    QLineEdit *trackEdit;
    QLineEdit *genreEdit;

    QLabel *songInfo;
    QLabel *albumArtLabel;
    QLabel *fileTypeLabel;
    QLabel *bitRateLabel;
    QLabel *sampleRateLabel;
    QLabel *durationLabel;
    QLabel *channelsLabel;
    QLabel *codecLabel;
    QLabel *fileSizeLabel;
    QLabel *bitDepthLabel;
    QLabel *filePathLabel;

    QPushButton *uploadAlbumArtButton;
    QPushButton *saveButton;
    QPushButton *backButton;
    QPushButton *playButton;
    QPushButton *fetchMetadataButton;
    QPushButton *fetchAlbumArtButton;
    QPushButton *identifyByFingerprintButton;

    QTextEdit *lyricsDisplay;
    
    QListWidget *resultListWidget;
    
    QString currentSongPath;
    
    QList<QJsonObject> recordingData; //for metadata update
};

#endif // SONGDETAIL_H