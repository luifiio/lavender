#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

class Playback : public QWidget {
    Q_OBJECT

public:
    explicit Playback(QWidget *parent = nullptr);
    void loadSong(const QString &songPath);

    QSize sizeHint() const override // dont work :(
    {
        return QSize(400, 500); 
    }


signals:
    void backToMainMenu(); 

    //signaling to mainmenu
    void playbackStarted(const QString &songTitle, int duration); 
    void playbackProgress(int position); 
    void playbackStopped(); 

public slots:
    void togglePlayPause();
    void stopPlayback();
    void seekPosition(int position);


private slots:
    void playPause();
    void setPosition(int position);

    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);

private:
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;

    QLabel *albumArtLabel;
    QLabel *songTitleLabel;
    QLabel *artistAlbumLabel;
    QLabel *currentTimeLabel;
    QLabel *totalTimeLabel;

    QPushButton *playPauseButton;
    QPushButton *backButton;

    QSlider *positionSlider;
  
};

#endif // PLAYBACK_H