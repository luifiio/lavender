#include "libScan.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <sqlite3.h>



bool LibScan::tableExists(sqlite3 *db, const QString &tableName) 
{
    const QString query = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1';").arg(tableName);
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, query.toUtf8().constData(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        qWarning() << "table gen failed:" << sqlite3_errmsg(db);
        return false;
    }
    rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}


void LibScan::scanMusicLibrary(const QString &directoryPath, const QString &dbPath) 
{
    qDebug() << "seleted dir: " << directoryPath << "& " << dbPath;

    QDir dir(directoryPath);

    if (!dir.exists()) 
    {
        qWarning() << "dir does not exist:" << directoryPath;
        return;
    }

    // db intialisation 
    sqlite3 *db;
    int rc = sqlite3_open(dbPath.toUtf8().constData(), &db);
    if (rc) 
    {
        qWarning() << "db cant be opened:" << sqlite3_errmsg(db);
        return;
    }
    qDebug() << "db opended";

    // check if tbls exist 
    if (!tableExists(db, "albums"))
    {
        const char *createAlbumsTable = "CREATE TABLE albums (id INTEGER PRIMARY KEY, name TEXT, path TEXT)";
        char *errMsg = nullptr;

        rc = sqlite3_exec(db, createAlbumsTable, nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK) 
        {
            qWarning() << "album table:" << errMsg;
            sqlite3_free(errMsg);
        } 
        else 
        {
            qDebug() << "album table success";
        }
    } 
    else 
    {
        qDebug() << "albums table exists.";
    }

    if (!tableExists(db, "songs"))
    {
        const char *createSongsTable = "CREATE TABLE songs (id INTEGER PRIMARY KEY, album_id INTEGER, name TEXT, artist TEXT, album TEXT, genre TEXT, path TEXT)";
        char *errMsg = nullptr;

        rc = sqlite3_exec(db, createSongsTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            qWarning() << "song table failed!:" << errMsg;
            sqlite3_free(errMsg);
        } 
        else 
        {
            qDebug() << "songs table created.";
        }
    } 
    else 
    {
        qDebug() << "song table exists.";
    }

    // recursively scan the dir
    std::function<void(const QDir&)> scanDir = [&](const QDir &dir) 
    {
        QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot); //get list of entries 

        for (const QFileInfo &entry : entries) // iterate through each entry
        {
            if (entry.isDir()) 
            {
                QString albumName = entry.fileName();
                QString albumPath = entry.absoluteFilePath();
                

                //--- insert album instance 
                sqlite3_stmt *stmt;
                const char *insertAlbumSQL = "INSERT INTO albums (name, path) VALUES (?, ?)";

                rc = sqlite3_prepare_v2(db, insertAlbumSQL, -1, &stmt, nullptr);
                if (rc != SQLITE_OK) 
                {
                    qWarning() << "album insert failed:" << sqlite3_errmsg(db);
                    continue;
                }

                sqlite3_bind_text(stmt, 1, albumName.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, albumPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);

                rc = sqlite3_step(stmt);

                if (rc != SQLITE_DONE)
                {
                    qWarning() << "failed to insert album:" << sqlite3_errmsg(db);
                } 
                else 
                {
                    qDebug() << "album inserted:" << albumName;
                }

                sqlite3_finalize(stmt);
                int albumId = sqlite3_last_insert_rowid(db);

                //---- insert songs from the album ---- //

                QDir albumDir(albumPath);
                QFileInfoList songEntries = albumDir.entryInfoList(QDir::Files);
                for (const QFileInfo &songEntry : songEntries)
                {
                    QString songPath = songEntry.absoluteFilePath();
                    TagLib::FileRef f(songPath.toUtf8().constData());

                    if (!f.isNull() && f.tag())  //get metadata from song in question 
                    {
                        TagLib::Tag *tag = f.tag();
                        QString songName = QString::fromStdString(tag->title().to8Bit(true));
                        QString artist = QString::fromStdString(tag->artist().to8Bit(true));
                        QString album = QString::fromStdString(tag->album().to8Bit(true));
                        QString genre = QString::fromStdString(tag->genre().to8Bit(true));

                        if (genre.isEmpty()) // keep musicbrainz api happy
                        {
                            genre = "Unknown";
                        }

                        const char *insertSongSQL = "INSERT INTO songs (album_id, name, artist, album, genre, path) VALUES (?, ?, ?, ?, ?, ?)";

                        rc = sqlite3_prepare_v2(db, insertSongSQL, -1, &stmt, nullptr);
                        if (rc != SQLITE_OK)
                        {
                            qWarning() << "song insertion failed" << sqlite3_errmsg(db);
                            continue;
                        }
                        // if insertion ok, bind taglib values to song db instance 
                        sqlite3_bind_int(stmt, 1, albumId);
                        sqlite3_bind_text(stmt, 2, songName.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 3, artist.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 4, album.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 5, genre.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 6, songPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);

                        rc = sqlite3_step(stmt);
                        if (rc != SQLITE_DONE) 
                        {
                            qWarning() << "song insert failed:" << sqlite3_errmsg(db);
                        } 
                        else 
                        {
                            qDebug() << "song insert success:" << songName;
                        }
                        sqlite3_finalize(stmt);
                    }
                }

                // recusrive scan on album sub dirs
                scanDir(albumDir);
            }
        }
    };

    // check root dir 
    scanDir(dir);

    sqlite3_close(db);
    qDebug() << "db closed";
}

