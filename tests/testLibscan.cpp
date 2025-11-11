#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include "../src/libScan.h"

class TestLibScan : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testDatabaseInitialization();
    void testScanEmptyDirectory();
    void testScanSingleFile();
    void testScanNestedDirectories();
    void testRescanWithAddedFiles();
    void testNonAudioFilesIgnored();

private:
    LibScan* scanner;
    QTemporaryDir tempDir;
    QString dbPath;
    
    void createTestAudioFile(const QString& path, bool valid = true);
    void verifyDatabaseTable(const QString& tableName, int expectedRowCount);
};

void TestLibScan::initTestCase()
{
    // ensure dir is valid
    QVERIFY(tempDir.isValid());
    
    // Create test database path
    dbPath = tempDir.path() + "/test_music.db";
    
    // scanner instance
    scanner = new LibScan(this);
    
    qDebug() << "Test setup complete. Using temporary directory:" << tempDir.path();
    qDebug() << "Database path:" << dbPath;
}

void TestLibScan::cleanupTestCase()
{
    delete scanner;
    
    // remove db 
    QFile dbFile(dbPath);
    if (dbFile.exists()) {
        dbFile.remove();
    }
}

void TestLibScan::createTestAudioFile(const QString& path, bool valid)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        if (valid) {
            // gen temp mp3 files
            QByteArray header;
            header.append("ID3"); // ID3 fake
            header.append(char(3)); // vers
            header.append(char(0)); // revision
            header.append(char(0)); // flags
            header.append(char(0)); // byte 1 size
            header.append(char(0)); // byte size 2 
            header.append(char(0)); // byte size 3
            header.append(char(0)); // byte size 4
            
            // fake mp3 headers
            header.append(QByteArray::fromHex("FFFB9064")); // sync
            
            file.write(header);
            file.write(QByteArray(1000, 'a')); // dummy content
        } else {
            // just write some random data
            file.write(QByteArray(100, 'x'));
        }
        file.close();
    }
}

void TestLibScan::verifyDatabaseTable(const QString& tableName, int expectedRowCount)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "testConnection");
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());
    
    QSqlQuery query(db);
    QVERIFY(query.exec("SELECT COUNT(*) FROM " + tableName));
    QVERIFY(query.next());
    int count = query.value(0).toInt();
    
    db.close();
    QSqlDatabase::removeDatabase("testConnection");
    
    QCOMPARE(count, expectedRowCount);
}

void TestLibScan::testDatabaseInitialization()
{
    bool success = scanner->scanMusicLibrary(tempDir.path(), dbPath);
    QVERIFY(success);
    
    
    QFile dbFile(dbPath);
    QVERIFY(dbFile.exists());
    
    // check tables
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "initTest");
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());
    
    QStringList tables = db.tables();
    QVERIFY(tables.contains("albums"));
    QVERIFY(tables.contains("songs"));
    
    db.close();
    QSqlDatabase::removeDatabase("initTest");
}

void TestLibScan::testScanEmptyDirectory()
{
    // tesst scanning 
    bool success = scanner->scanMusicLibrary(tempDir.path(), dbPath);
    QVERIFY(success);
    
    // Verify no records were added
    verifyDatabaseTable("songs", 0);
    verifyDatabaseTable("albums", 0);
}

void TestLibScan::testScanSingleFile()
{
    //test file
    QString filePath = tempDir.path() + "/test_song.mp3";
    createTestAudioFile(filePath);
    
    // scan dir
    bool success = scanner->scanMusicLibrary(tempDir.path(), dbPath);
    QVERIFY(success);
    

    verifyDatabaseTable("songs", 1);
    
}

void TestLibScan::testScanNestedDirectories()
{
    // nested dir struct
    QDir dir(tempDir.path());
    dir.mkdir("album1");
    dir.mkdir("album2");
    dir.mkdir("album2/disc2");
    
    // gen fake mp3 files   
    createTestAudioFile(tempDir.path() + "/album1/song1.mp3");
    createTestAudioFile(tempDir.path() + "/album1/song2.mp3");
    createTestAudioFile(tempDir.path() + "/album2/song3.mp3");
    createTestAudioFile(tempDir.path() + "/album2/disc2/song4.mp3");
    
    // scan dir & return bool 
    bool success = scanner->scanMusicLibrary(tempDir.path(), dbPath);
    QVERIFY(success);
    
    // and potentialy 5 albums?
    verifyDatabaseTable("songs", 5);
}

void TestLibScan::testRescanWithAddedFiles()
{
    // record current song count
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "countCheck");
    db.setDatabaseName(dbPath);
    db.open();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM songs");
    query.next();
    int initialCount = query.value(0).toInt();
    db.close();
    QSqlDatabase::removeDatabase("countCheck");
    
    createTestAudioFile(tempDir.path() + "/album1/new_song.mp3");
    createTestAudioFile(tempDir.path() + "/album2/another_song.mp3");
    
    bool success = scanner->scanMusicLibrary(tempDir.path(), dbPath);
    QVERIFY(success);
    
    //verify song count
    verifyDatabaseTable("songs", initialCount + 2);
}

void TestLibScan::testNonAudioFilesIgnored()
{
    // record current song count 
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "countCheck2");
    db.setDatabaseName(dbPath);
    db.open();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM songs");
    query.next();
    int initialCount = query.value(0).toInt();
    db.close();
    QSqlDatabase::removeDatabase("countCheck2");
    
    // non audio file
    QFile textFile(tempDir.path() + "/not_audio.txt");
    textFile.open(QIODevice::WriteOnly);
    textFile.write("This is a text file, not an audio file");
    textFile.close();
    
    // Cinvalid mp3
    createTestAudioFile(tempDir.path() + "/invalid.mp3", false);
    
    // rescan
    bool success = scanner->scanMusicLibrary(tempDir.path(), dbPath);
    QVERIFY(success);
    
    // veirfy count
    verifyDatabaseTable("songs", initialCount);
}

QTEST_MAIN(TestLibScan)
#include "test_libscan.moc"