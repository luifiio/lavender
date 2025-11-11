#ifndef ALBUMMENU_H
#define ALBUMMENU_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

class AlbumMenu : public QWidget 
{
    Q_OBJECT

public:
    explicit AlbumMenu(QWidget *parent = nullptr);

    void loadAlbum(const QString &albumName, const QString &albumPath); //for populating songlist widget

signals:
    void songSelected(const QString &songPath);

private slots:
    void onSongClicked(QListWidgetItem *item);

private:
    QVBoxLayout *layout;
    QLabel *albumArtLabel;
    QLabel *albumNameLabel;
    QListWidget *songListWidget;
};

#endif // ALBUMMENU_H