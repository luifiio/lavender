#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QFile>
#include "../src/songMenu.h"

class TestSongDetail : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // metadata basics
    void testLoadSong();
    void testSaveMetadata();
    
    // api interaction mocks
    void testHandleFingerprintResult();
    void testSelectMetadataResult();
    
    // edge cases
    void testLoadInvalidSong();
    void testEmptyMetadataFields();

private:
    SongDetail* songDetail;
    QTemporaryDir tempDir;
    QString testSongPath;
    
    // helper to create a test mp3 file
    void createTestMP3File();
    
    // helper to create mock api responses
    QJsonObject createMockFingerprintResponse();
    QJsonObject createMockRecording();
};

void TestSongDetail::initTestCase()
{
    // create a songdetail instance for testing
    songDetail = new SongDetail();
    
    // create a temporary test mp3 file
    QVERIFY(tempDir.isValid());
    testSongPath = tempDir.path() + "/test_song.mp3";
    createTestMP3File();
    
    qDebug() << "Test setup complete. Using test file:" << testSongPath;
}

void TestSongDetail::cleanupTestCase()
{
    delete songDetail;
}

void TestSongDetail::createTestMP3File()
{
    QFile file(testSongPath);
    if (file.open(QIODevice::WriteOnly)) {
        // create minimal mp3 header
        QByteArray header;
        header.append("ID3"); // id3 tag marker
        header.append(char(3)); // version
        header.append(char(0)); // revision
        header.append(char(0)); // flags
        header.append(char(0)); // size byte 1
        header.append(char(0)); // size byte 2
        header.append(char(0)); // size byte 3
        header.append(char(10)); // size byte 4 (small size)
        
        // write simple id3 tag fields
        // title: "test song"
        QByteArray titleFrame;
        titleFrame.append("TIT2");           // frame id
        titleFrame.append(char(0));          // size byte 1
        titleFrame.append(char(0));          // size byte 2
        titleFrame.append(char(0));          // size byte 3
        titleFrame.append(char(9));          // size byte 4 (9 bytes)
        titleFrame.append(char(0));          // flags 1
        titleFrame.append(char(0));          // flags 2
        titleFrame.append(char(0));          // encoding
        titleFrame.append("Test Song");      // content
        
        // artist: "test artist"
        QByteArray artistFrame;
        artistFrame.append("TPE1");          // frame id
        artistFrame.append(char(0));         // size byte 1
        artistFrame.append(char(0));         // size byte 2
        artistFrame.append(char(0));         // size byte 3
        artistFrame.append(char(11));        // size byte 4 (11 bytes)
        artistFrame.append(char(0));         // flags 1
        artistFrame.append(char(0));         // flags 2
        artistFrame.append(char(0));         // encoding
        artistFrame.append("Test Artist");   // content
        
        // add mp3 frame header
        QByteArray mp3Data = header + titleFrame + artistFrame;
        mp3Data.append(QByteArray::fromHex("FFFB9064")); // frame sync
        
        // add some dummy content
        mp3Data.append(QByteArray(1000, 'a'));
        
        file.write(mp3Data);
        file.close();
    }
}

QJsonObject TestSongDetail::createMockFingerprintResponse()
{
    QJsonObject response;
    
    QJsonArray results;
    QJsonObject result;
    
    // add a recording array to the result
    QJsonArray recordings;
    recordings.append(createMockRecording());
    result["recordings"] = recordings;
    
    results.append(result);
    response["results"] = results;
    
    return response;
}

QJsonObject TestSongDetail::createMockRecording()
{
    QJsonObject recording;
    
    // basic metadata
    recording["title"] = "Test Song Title";
    recording["id"] = "123e4567-e89b-12d3-a456-426614174000";
    
    // artist info
    QJsonArray artists;
    QJsonObject artist;
    artist["id"] = "789e0123-e45b-67d8-a901-234567890123";
    artist["name"] = "Test Artist Name";
    artists.append(artist);
    recording["artists"] = artists;
    
    // release group info
    QJsonArray releaseGroups;
    QJsonObject releaseGroup;
    releaseGroup["id"] = "345e6789-e01b-23d4-a567-890123456789";
    releaseGroup["title"] = "Test Album Title";
    releaseGroup["type"] = "Album";
    
    // add a first release date to the release group
    releaseGroup["first-release-date"] = "2023-04-23";
    
    releaseGroups.append(releaseGroup);
    recording["releasegroups"] = releaseGroups;
    
    // add some genres
    QJsonArray genres;
    QJsonObject genre1;
    genre1["name"] = "Rock";
    QJsonObject genre2;
    genre2["name"] = "Alternative";
    genres.append(genre1);
    genres.append(genre2);
    recording["genres"] = genres;
    
    return recording;
}

void TestSongDetail::testLoadSong()
{
    // load our test song
    songDetail->loadSong(testSongPath);
    
    // since we're working with a minimal mp3 with basic tags,
    // just verify that the fields aren't empty
    QVERIFY(!songDetail->titleEdit->text().isEmpty());
    QVERIFY(!songDetail->artistEdit->text().isEmpty());
    
    // verify that identification button is enabled
    QVERIFY(songDetail->identifyByFingerprintButton->isEnabled());
}

void TestSongDetail::testSaveMetadata()
{
    // load the test song first
    songDetail->loadSong(testSongPath);
    
    // change metadata
    QString testTitle = "Modified Title";
    QString testArtist = "Modified Artist";
    QString testAlbum = "Modified Album";
    
    songDetail->titleEdit->setText(testTitle);
    songDetail->artistEdit->setText(testArtist);
    songDetail->albumEdit->setText(testAlbum);
    
    // save changes
    songDetail->saveMetadata();
    
    // reload the song to see if changes persisted
    songDetail->loadSong(testSongPath);
    
    // verify the changes were saved
    // note: actual file saving might fail in a test environment depending on permissions,
    // so these checks might need to be adjusted based on your setup
    QCOMPARE(songDetail->titleEdit->text(), testTitle);
    QCOMPARE(songDetail->artistEdit->text(), testArtist);
    QCOMPARE(songDetail->albumEdit->text(), testAlbum);
}

void TestSongDetail::testHandleFingerprintResult()
{
    //load test song
    songDetail->loadSong(testSongPath);
    
    //clear just in case
    songDetail->resultListWidget->clear();
    songDetail->recordingData.clear();
    
    //mock json response
    QJsonObject mockResult = createMockFingerprintResponse();
    
    //call the handler with the fake json
    songDetail->handleFingerprintResult(mockResult);
    
    //check if the result list widget was populated
    QVERIFY(songDetail->resultListWidget->count() > 0);
    QVERIFY(!songDetail->recordingData.isEmpty());
}

void TestSongDetail::testSelectMetadataResult()
{
    // load test song
    songDetail->loadSong(testSongPath);
    
    // set up mock data
    QJsonObject recording = createMockRecording();
    songDetail->recordingData.clear();
    songDetail->recordingData.append(recording);
    
    // create a mock item
    QListWidgetItem* item = new QListWidgetItem("Test Result");
    item->setData(Qt::UserRole, 0); // index 0 in the recordingdata array
    
    // trigger selection (simulating a click)
    songDetail->resultListWidget->addItem(item);
    songDetail->resultListWidget->setCurrentItem(item);
    QMetaObject::invokeMethod(songDetail->resultListWidget, "itemClicked", 
                             Q_ARG(QListWidgetItem*, item));
    
    // verify that metadata fields were updated
    QCOMPARE(songDetail->titleEdit->text(), "Test Song Title");
    QCOMPARE(songDetail->artistEdit->text(), "Test Artist Name");
    QCOMPARE(songDetail->albumEdit->text(), "Test Album Title");
    QCOMPARE(songDetail->yearEdit->text(), "2023");
    QCOMPARE(songDetail->genreEdit->text(), "Rock, Alternative");
}

void TestSongDetail::testLoadInvalidSong()
{
    // create an invalid file path
    QString invalidPath = tempDir.path() + "/nonexistent.mp3";
    
    // set up a signal spy to verify that an error message was displayed
    songDetail->songInfo->setText("");
    
    // load the invalid song
    songDetail->loadSong(invalidPath);
    
    // verify that an error was shown
    QVERIFY(!songDetail->songInfo->text().isEmpty());
    QVERIFY(songDetail->songInfo->text().contains("Failed", Qt::CaseInsensitive));
}

void TestSongDetail::testEmptyMetadataFields()
{
    // load the test song first
    songDetail->loadSong(testSongPath);
    
    // clear all fields
    songDetail->titleEdit->clear();
    songDetail->artistEdit->clear();
    songDetail->albumEdit->clear();
    songDetail->yearEdit->clear();
    songDetail->trackEdit->clear();
    songDetail->genreEdit->clear();
    
    // try to save with empty fields
    songDetail->saveMetadata();
    
    // reload to check what was saved
    songDetail->loadSong(testSongPath);
    
    // all fields should be empty or zero after save and reload
    QVERIFY(songDetail->titleEdit->text().isEmpty());
    QVERIFY(songDetail->artistEdit->text().isEmpty());
    QVERIFY(songDetail->albumEdit->text().isEmpty());
    
    // year and track are integers so they're expected to be 0
    QCOMPARE(songDetail->yearEdit->text(), "0");
    QCOMPARE(songDetail->trackEdit->text(), "0");
}

QTEST_MAIN(TestSongDetail)
#include "test_songdetail.moc"