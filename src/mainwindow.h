#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "mainMenu.h"
#include "albumMenu.h"
#include "songMenu.h"
#include "playback.h"
#include "recoMenu.h"
#include "libScan.h"

class MainWindow : public QMainWindow 
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void returnMainMenu();

    void showMainMenu(const QString &folder);
    void showRecommendationMenu(int songId);
    void showAlbumMenu(const QString &albumName, const QString &albumPath);
    void showSongMenu(const QString &songPath);

    void showPlayback(const QString &songPath);
    void showPlayback();


private:
    QStackedWidget *stackedWidget;

    MainMenu *mainMenu;
    AlbumMenu *albumMenu;
    SongDetail *songDetail;
    RecommendationMenu *recommendationMenu;

    Playback *playback;
};

#endif // MAINWINDOW_H