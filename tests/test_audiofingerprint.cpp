#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include "../src/audiofingerprint.h"

class TestAudioFingerprint : public QObject
{
    Q_OBJECT

private slots:
    //funcs to test class methods
    void initTestCase();
    void testFingerprintGeneration();
    void testFingerprintGenerationFailure();
    void testMetadataLookup();
    void testMetadataLookupFailure();
    void testPerformance();
    void testMultipleFormats();
    void testDifferentQualities();
    void cleanupTestCase();

private:
    AudioFingerprint* fingerprinter; // init
    QString getTestFilePath(const QString& filename) const;
    QStringList setupTestFiles();
};

void TestAudioFingerprint::initTestCase()
{
    qDebug() << "Initializing AudioFingerprint test case";
    fingerprinter = new AudioFingerprint(this);
    
    // check if fpcalc is installed
    QProcess checkProcess;
    checkProcess.start("which", QStringList() << "fpcalc");
    checkProcess.waitForFinished();
    QVERIFY2(checkProcess.exitCode() == 0, "fpcalc is not installed, skipping tests");

    // verify test directory exists
    QDir testDir("test_data");
    if (!testDir.exists()) {
        QVERIFY2(QDir().mkpath("test_data"), "Failed to create test data directory");
    }

    // check if we have test files, if not, copy some from resources
    QFileInfo testFile(getTestFilePath("song1.flac"));
    if (!testFile.exists()) {
        qDebug() << "Test files need to be set up. Please place audio files in the test_data directory.";
    }
}

QString TestAudioFingerprint::getTestFilePath(const QString& filename) const
{
    return QDir("test_data").filePath(filename);
}

QStringList TestAudioFingerprint::setupTestFiles()
{
    // return list of actual test files
    QDir testDir("test_data");
    return testDir.entryList(QStringList() << "*.mp3" << "*.flac" << "*.wav", QDir::Files);
}

void TestAudioFingerprint::testFingerprintGeneration()
{
    QStringList testFiles = setupTestFiles();
    if (testFiles.isEmpty()) {
        QSKIP("No test files available");
    }
    
    QString testFile = getTestFilePath(testFiles.first());
    qDebug() << "Testing fingerprint generation with:" << testFile;
    
    // test the fingerprint generation
    bool result = fingerprinter->generateFingerprint(testFile);
    
    // verify the result
    QVERIFY(result);
    QVERIFY(!fingerprinter->getFingerprint().isEmpty());
    QVERIFY(fingerprinter->getDuration() > 0);
    
    qDebug() << "Fingerprint length:" << fingerprinter->getFingerprint().length();
    qDebug() << "Duration:" << fingerprinter->getDuration() << "seconds";
}

void TestAudioFingerprint::testFingerprintGenerationFailure()
{
    // test with a non-existent file
    QString nonExistentFile = getTestFilePath("nonexistent.mp3");
    
    // expect an error signal
    QSignalSpy errorSpy(fingerprinter, &AudioFingerprint::error);
    
    // test the fingerprint generation
    bool result = fingerprinter->generateFingerprint(nonExistentFile);
    
    // verify the result
    QVERIFY(!result);
    QVERIFY(errorSpy.count() > 0);
    QVERIFY(fingerprinter->getFingerprint().isEmpty());
}

void TestAudioFingerprint::testMetadataLookup()
{
    QStringList testFiles = setupTestFiles();
    if (testFiles.isEmpty()) {
        QSKIP("No test files available");
    }

    // use a well-known song for reliable testing
    QString testFile = getTestFilePath(testFiles.first());
    qDebug() << "Testing metadata lookup with:" << testFile;
    
    // first generate a fingerprint
    bool fpResult = fingerprinter->generateFingerprint(testFile);
    QVERIFY(fpResult);
    
    // set up signal spy for metadata found signal
    QSignalSpy metadataSpy(fingerprinter, &AudioFingerprint::metadataFound);
    QSignalSpy errorSpy(fingerprinter, &AudioFingerprint::error);
    
    // perform the lookup
    fingerprinter->lookupMetadata();
    
    // wait for up to 10 seconds for a response
    QVERIFY(metadataSpy.wait(10000) || errorSpy.wait(10000));
    
    // check if we got metadata or an error
    if (metadataSpy.count() > 0) {
        QJsonObject metadata = metadataSpy.first().at(0).toJsonObject();
        qDebug() << "Received metadata:" << QJsonDocument(metadata).toJson();
        
        // verify we have results
        QVERIFY(metadata.contains("results"));
        QJsonArray results = metadata["results"].toArray();
        QVERIFY(!results.isEmpty());
    } else {
        // if we got an error instead, log it but don't fail the test
        // (since the test file may not be in the acoustid database)
        qDebug() << "Received error:" << errorSpy.first().at(0).toString();
        QSKIP("Metadata lookup failed, but this may be expected for test files");
    }
}

void TestAudioFingerprint::testMetadataLookupFailure()
{
    // make a fingerprint invalid
    fingerprinter->setFingerprint(QByteArray("invalid"));
    fingerprinter->setDuration(30);
    
    // set up signal spy for error signal
    QSignalSpy errorSpy(fingerprinter, &AudioFingerprint::error);
    
    // perform the lookup
    fingerprinter->lookupMetadata();
    
    // wait for the error
    QVERIFY(errorSpy.wait(10000));
    QVERIFY(errorSpy.count() > 0);
}

void TestAudioFingerprint::testPerformance()
{
    QStringList testFiles = setupTestFiles();
    if (testFiles.isEmpty()) {
        QSKIP("No test files available");
    }
    
    QVector<qint64> generationTimes;
    
    // test performance on all test files
    for (const QString& filename : testFiles) {
        QString testFile = getTestFilePath(filename);
        qDebug() << "Measuring performance on:" << testFile;
        
        // measure time to generate fingerprint
        QElapsedTimer timer;
        timer.start();
        
        bool result = fingerprinter->generateFingerprint(testFile);
        
        qint64 elapsed = timer.elapsed();
        
        if (result) {
            qDebug() << "Generation took" << elapsed << "ms for" << QFileInfo(testFile).size() / 1024 << "KB file";
            generationTimes.append(elapsed);
        }
    }
    
    if (!generationTimes.isEmpty()) {
        // calculate statistics
        qint64 totalTime = 0;
        for (qint64 time : generationTimes) {
            totalTime += time;
        }
        
        qreal avgTime = totalTime / static_cast<qreal>(generationTimes.size());
        
        qDebug() << "Performance Statistics:";
        qDebug() << "Average fingerprint generation time:" << avgTime << "ms";
        qDebug() << "Tests run:" << generationTimes.size();
        qDebug() << "Total time:" << totalTime << "ms";
        
        // export the results to a file for later analysis
        QFile perfFile("fingerprint_performance.csv");
        if (perfFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&perfFile);
            out << "Filename,Size(KB),Time(ms)\n";
            
            for (int i = 0; i < testFiles.size(); i++) {
                if (i < generationTimes.size()) {
                    QString testFile = getTestFilePath(testFiles[i]);
                    out << testFiles[i] << ","
                        << QFileInfo(testFile).size() / 1024 << ","
                        << generationTimes[i] << "\n";
                }
            }
        }
    }
}

void TestAudioFingerprint::testMultipleFormats()
{
    // this test requires files in different formats
    QStringList formats;
    
    if (QFile::exists(getTestFilePath("sample.mp3"))) formats << "mp3";
    if (QFile::exists(getTestFilePath("sample.flac"))) formats << "flac";
    if (QFile::exists(getTestFilePath("sample.wav"))) formats << "wav";
    
    if (formats.size() < 2) {
        QSKIP("Need multiple formats for format comparison test");
    }
    
    QMap<QString, qint64> formatTimes;
    QMap<QString, int> fingerprintSizes;
    
    for (const QString& format : formats) {
        QString testFile = getTestFilePath("sample." + format);
        QElapsedTimer timer;
        timer.start();
        
        bool result = fingerprinter->generateFingerprint(testFile);
        
        qint64 elapsed = timer.elapsed();
        
        if (result) {
            formatTimes[format] = elapsed;
            fingerprintSizes[format] = fingerprinter->getFingerprint().length();
            
            qDebug() << format << "fingerprint generation took" << elapsed
                    << "ms and produced a fingerprint of size" << fingerprintSizes[format];
        }
    }
    
    // output comparison
    qDebug() << "Format Comparison:";
    for (auto it = formatTimes.begin(); it != formatTimes.end(); ++it) {
        qDebug() << it.key() << ": Time=" << it.value() << "ms, Size=" << fingerprintSizes[it.key()];
    }
    
    // export the results
    QFile formatFile("format_comparison.csv");
    if (formatFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&formatFile);
        out << "Format,Time(ms),Size(bytes)\n";
        
        for (auto it = formatTimes.begin(); it != formatTimes.end(); ++it) {
            out << it.key() << "," << it.value() << "," << fingerprintSizes[it.key()] << "\n";
        }
    }
}

void TestAudioFingerprint::testDifferentQualities()
{
    // this test expects files of the same song in different qualities
    if (!QFile::exists(getTestFilePath("quality_high.mp3")) ||
        !QFile::exists(getTestFilePath("quality_medium.mp3")) ||
        !QFile::exists(getTestFilePath("quality_low.mp3"))) {
        QSKIP("Quality comparison files not found");
    }
    
    QVector<QPair<QString, QString>> qualityTests = {
        { "quality_high.mp3", "High (320kbps)" },
        { "quality_medium.mp3", "Medium (128kbps)" },
        { "quality_low.mp3", "Low (64kbps)" }
    };
    
    QMap<QString, QByteArray> fingerprints;
    QMap<QString, qint64> processingTimes;
    
    for (const auto& test : qualityTests) {
        QString testFile = getTestFilePath(test.first);
        QElapsedTimer timer;
        timer.start();
        
        bool result = fingerprinter->generateFingerprint(testFile);
        
        qint64 elapsed = timer.elapsed();
        
        if (result) {
            fingerprints[test.second] = fingerprinter->getFingerprint();
            processingTimes[test.second] = elapsed;
            
            qDebug() << test.second << "quality processed in" << elapsed << "ms";
            qDebug() << "Fingerprint length:" << fingerprinter->getFingerprint().length();
        }
    }
    
    // compare fingerprints
    qDebug() << "\nFingerprint Similarity Analysis:";
    if (fingerprints.size() >= 2) {
        // calculate simple similarity between high and other qualities
        QByteArray highQualityFP = fingerprints["High (320kbps)"];
        
        for (auto it = fingerprints.begin(); it != fingerprints.end(); ++it) {
            if (it.key() != "High (320kbps)") {
                // calculate basic similarity (% of matching bytes)
                QByteArray currentFP = it.value();
                int matchingBytes = 0;
                int minLength = qMin(highQualityFP.length(), currentFP.length());
                
                for (int i = 0; i < minLength; i++) {
                    if (highQualityFP[i] == currentFP[i]) {
                        matchingBytes++;
                    }
                }
                
                double similarity = (minLength > 0) ? 
                    (matchingBytes * 100.0 / minLength) : 0;
                
                qDebug() << "Similarity between High and" << it.key() 
                        << ":" << similarity << "%";
            }
        }
    }
    
    // export the results
    QFile qualityFile("quality_comparison.csv");
    if (qualityFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&qualityFile);
        out << "Quality,Time(ms),FingerprintLength\n";
        
        for (auto it = processingTimes.begin(); it != processingTimes.end(); ++it) {
            out << it.key() << "," << it.value() << ","
                << fingerprints[it.key()].length() << "\n";
        }
    }
}

void TestAudioFingerprint::cleanupTestCase()
{
    delete fingerprinter;
}

QTEST_MAIN(TestAudioFingerprint)
#include "test_audiofingerprint.moc"