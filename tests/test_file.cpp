// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/FileDocument.h"

#include <QFile>
#include <QTemporaryDir>

class FileTest : public QObject
{
    Q_OBJECT

private slots:
    void loadReadsContent();
    void loadReportsMissingFile();
    void saveWritesContent();
    void externalChangeIsDetected();
    void removingTheFileIsReported();

private:
    static void write(const QString &path, const QString &content);
};

void FileTest::write(const QString &path, const QString &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content.toUtf8());
}

void FileTest::loadReadsContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("doc.md"));
    write(path, QStringLiteral("# Title\n\nbody\n"));

    FileDocument document;
    QString error;
    QVERIFY(document.load(path, &error));
    QVERIFY(error.isEmpty());
    QCOMPARE(document.filePath(), path);
    QCOMPARE(document.content(), QStringLiteral("# Title\n\nbody\n"));
}

void FileTest::loadReportsMissingFile()
{
    FileDocument document;
    QString error;
    QVERIFY(!document.load(QStringLiteral("/nonexistent/does-not-exist.md"), &error));
    QVERIFY(!error.isEmpty());
}

void FileTest::saveWritesContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("out.md"));

    FileDocument document;
    QString error;
    QVERIFY(document.save(path, QStringLiteral("hello\n"), &error));
    QVERIFY(error.isEmpty());
    QCOMPARE(document.content(), QStringLiteral("hello\n"));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("hello\n"));
}

void FileTest::externalChangeIsDetected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("watched.md"));
    write(path, QStringLiteral("initial\n"));

    FileDocument document;
    QString error;
    QVERIFY(document.load(path, &error));

    QSignalSpy spy(&document, &FileDocument::externallyChanged);
    write(path, QStringLiteral("changed externally\n"));

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 3000);
    QCOMPARE(document.content(), QStringLiteral("changed externally\n"));
}

void FileTest::removingTheFileIsReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("gone.md"));
    write(path, QStringLiteral("x\n"));

    FileDocument document;
    QString error;
    QVERIFY(document.load(path, &error));

    QSignalSpy spy(&document, &FileDocument::externallyChanged);
    QVERIFY(QFile::remove(path));

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 3000);
    QVERIFY(!document.exists());
}

QTEST_MAIN(FileTest)
#include "test_file.moc"