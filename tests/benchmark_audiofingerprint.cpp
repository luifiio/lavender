#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../src/audiofingerprint.h"

class BenchmarkAudioFingerprint : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void benchmark_fingerprintGeneration();
    void benchmark_apiLookup();
    void benchmark_largeDataset();
    void cleanupTestCase();

private:
    AudioFingerprint* fingerprinter;
    QStringList testFiles;
    
    QString getTestFilePath(const QString& filename) const;
    QStringList getTestFiles() const;
    void writeResultsToJson(const QString& filename, const QJsonObject& data);
};

void BenchmarkAudioFingerprint::initTestCase()
{
    qDebug() << "Initializing AudioFingerprint benchmark";
    fingerprinter = new AudioFingerprint(this);
    
    // check if fpcalc is installed
    QProcess checkProcess;
    checkProcess.start("which", QStringList() << "fpcalc");
    checkProcess.waitForFinished();
    QVERIFY2(checkProcess.exitCode() == 0, "fpcalc is not installed, skipping benchmarks");

    // get test files
    testFiles = getTestFiles();
    if (testFiles.isEmpty()) {
        qWarning() << "No test files found in test_data directory";
    } else {
        qDebug() << "Found" << testFiles.size() << "test files for benchmarking";
    }
}

QString BenchmarkAudioFingerprint::getTestFilePath(const QString& filename) const
{
    return QDir("test_data").filePath(filename);
}

QStringList BenchmarkAudioFingerprint::getTestFiles() const
{
    QDir testDir("test_data");
    return testDir.entryList(QStringList() << "*.mp3" << "*.flac" << "*.wav", QDir::Files);
}

void BenchmarkAudioFingerprint::writeResultsToJson(const QString& filename, const QJsonObject& data)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(data);
        file.write(doc.toJson());
    }
}

void BenchmarkAudioFingerprint::benchmark_fingerprintGeneration()
{
    if (testFiles.isEmpty()) {
        QSKIP("No test files available");
    }
    
    QJsonArray results;
    
    QBENCHMARK {
        for (const QString& filename : testFiles) {
            QString testFile = getTestFilePath(filename);
            
            QElapsedTimer timer;
            timer.start();
            
            bool success = fingerprinter->generateFingerprint(testFile);
            
            qint64 elapsed = timer.elapsed();
            
            QJsonObject fileResult;
            fileResult["filename"] = filename;
            fileResult["filesize_kb"] = QFileInfo(testFile).size() / 1024;
            fileResult["success"] = success;
            fileResult["time_ms"] = elapsed;
            
            if (success) {
                fileResult["fingerprint_length"] = fingerprinter->getFingerprint().length();
                fileResult["duration_sec"] = fingerprinter->getDuration();
            }
            
            results.append(fileResult);
        }
    }
    
    QJsonObject resultData;
    resultData["fingerprint_generation"] = results;
    writeResultsToJson("benchmark_fingerprint_generation.json", resultData);
}

void BenchmarkAudioFingerprint::benchmark_apiLookup()
{
    if (testFiles.isEmpty()) {
        QSKIP("No test files available");
    }
    
    // use a small subset of files for api benchmarks to avoid rate limits
    QStringList apiTestFiles = testFiles.mid(0, qMin(3, testFiles.size()));
    QJsonArray results;
    
    for (const QString& filename : apiTestFiles) {
        QString testFile = getTestFilePath(filename);
        QJsonObject fileResult;
        fileResult["filename"] = filename;
        
        // first generate fingerprint
        bool fpSuccess = fingerprinter->generateFingerprint(testFile);
        if (!fpSuccess) {
            fileResult["error"] = "Failed to generate fingerprint";
            results.append(fileResult);
            continue;
        }
        
        // set up signal spy for metadata found signal
        QSignalSpy metadataSpy(fingerprinter, &AudioFingerprint::metadataFound);
        QSignalSpy errorSpy(fingerprinter, &AudioFingerprint::error);
        
        // time the api call
        QElapsedTimer timer;
        timer.start();
        
        fingerprinter->lookupMetadata();
        
        // wait for response (with timeout)
        bool gotResponse = metadataSpy.wait(10000) || errorSpy.wait(10000);
        qint64 elapsed = timer.elapsed();
        
        fileResult["api_response_time_ms"] = elapsed;
        fileResult["got_response"] = gotResponse;
        
        if (metadataSpy.count() > 0) {
            QJsonObject metadata = metadataSpy.first().at(0).toJsonObject();
            fileResult["success"] = true;
            fileResult["result_count"] = metadata["results"].toArray().size();
        } else if (errorSpy.count() > 0) {
            fileResult["success"] = false;
            fileResult["error"] = errorSpy.first().at(0).toString();
        } else {
            fileResult["success"] = false;
            fileResult["error"] = "Timeout waiting for response";
        }
        
        results.append(fileResult);
    }
    
    QJsonObject resultData;
    resultData["api_lookup"] = results;
    writeResultsToJson("benchmark_api_lookup.json", resultData);
}

void BenchmarkAudioFingerprint::benchmark_largeDataset()
{
    // this benchmark requires a lot of test files
    if (testFiles.size() < 10) {
        QSKIP("Not enough test files for large dataset test");
    }
    
    QJsonObject resultData;
    QJsonArray dataPoints;
    
    // track total and average processing times
    qint64 totalProcessingTime = 0;
    int processedFiles = 0;
    
    // process all files in sequence
    for (const QString& filename : testFiles) {
        QString testFile = getTestFilePath(filename);
        QFileInfo fileInfo(testFile);
        
        QElapsedTimer timer;
        timer.start();
        
        bool success = fingerprinter->generateFingerprint(testFile);
        
        qint64 elapsed = timer.elapsed();
        
        if (success) {
            totalProcessingTime += elapsed;
            processedFiles++;
            
            QJsonObject dataPoint;
            dataPoint["file_index"] = processedFiles;
            dataPoint["time_ms"] = elapsed;
            dataPoint["filesize_kb"] = fileInfo.size() / 1024;
            dataPoint["fingerprint_length"] = fingerprinter->getFingerprint().length();
            dataPoint["running_avg_ms"] = totalProcessingTime / processedFiles;
            
            dataPoints.append(dataPoint);
        }
    }
    
    resultData["processing_series"] = dataPoints;
    resultData["total_files"] = processedFiles;
    resultData["total_time_ms"] = totalProcessingTime;
    resultData["average_time_ms"] = processedFiles > 0 ? totalProcessingTime / processedFiles : 0;
    
    writeResultsToJson("benchmark_large_dataset.json", resultData);
    
    qDebug() << "Large dataset benchmark:";
    qDebug() << "Processed" << processedFiles << "files";
    qDebug() << "Total time:" << totalProcessingTime << "ms";
    qDebug() << "Average time per file:" << (processedFiles > 0 ? totalProcessingTime / processedFiles : 0) << "ms";
}

void BenchmarkAudioFingerprint::cleanupTestCase()
{
    delete fingerprinter;
}

QTEST_MAIN(BenchmarkAudioFingerprint)
#include "benchmark_audiofingerprint.moc"