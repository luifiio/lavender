#include <QApplication>
#include <QFile>
#include "mainwindow.h"

int main(int argc, char *argv[]) 
{
    QApplication app(argc, argv);

    //not needed really, but just in case consturctor css styling fails
    QFile styleFile(":/styles.qss"); 
    if (styleFile.open(QFile::ReadOnly)) 
    {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }


    MainWindow mainWindow; //instance of the main window to gen the UI
    mainWindow.show();

    return app.exec();
}