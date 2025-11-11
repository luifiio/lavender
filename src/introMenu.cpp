#include "introMenu.h"
#include "libScan.h"
#include <QFileDialog>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

IntroMenu::IntroMenu(QWidget *parent) : QWidget(parent) {
    // Initialization code if needed
    selectMusicFolder(); // Show the file dialog when the IntroMenu is created
}

void IntroMenu::selectMusicFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select Music Folder");
    if (!folder.isEmpty()) {
        qDebug() << "Selected music folder:" << folder;

        // Determine the path to the database file
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dbPath); // Ensure the directory exists
        dbPath += "/lavender.db"; // Set the path to your database

        qDebug() << "Database path:" << dbPath;

        // Check if the database already exists
        if (QFile::exists(dbPath)) {
            qDebug() << "Database already exists, skipping scan";
        } else {
            // Call the scanMusicLibrary function
            LibScan::scanMusicLibrary(folder, dbPath);
        }

        emit musicFolderSelected(folder);
    } else {
        qDebug() << "No folder selected";
    }
}