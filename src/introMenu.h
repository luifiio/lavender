#ifndef INTROMENU_H
#define INTROMENU_H

#include <QtWidgets/QWidget>

class IntroMenu : public QWidget {
    Q_OBJECT

public:
    explicit IntroMenu(QWidget *parent = nullptr);

signals:
    void musicFolderSelected(const QString &folder);

public slots:
    void selectMusicFolder();
};

#endif // INTROMENU_H