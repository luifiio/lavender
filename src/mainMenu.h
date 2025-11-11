#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>

class ClickableLabel : public QLabel 
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override 
    {
        emit clicked();
    }
};

class MainMenu : public QWidget 
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);

    void loadAlbums(const QString &dbPath);
    void updatePlaybackBar(const QString &songTitle, int duration); // update playback bar if user plays song
    void updatePlaybackProgress(int position); //update progress when switching to mainemenu
    

signals:
    void showRecommendationMenu();
    void showPlayback();
    void showAlbumMenu(const QString &albumName, const QString &albumPath);

    //signal to playback menu functinalities 
    void playPauseClicked();
    void stopClicked();     
    void seekPosition(int position); 


private slots:
    void onRecommendationButtonClicked();
    void onAlbumClicked();

    void onPlaybackButtonClicked();

private:
    QGridLayout *layout;

    QPushButton *recommendationButton;
    QPushButton *playbackButton;
    QPushButton *playPauseButton;
    QPushButton *stopButton;

    QLabel *currentSongLabel;
    QSlider *playbackSlider;



};

#endif // MAINMENU_H