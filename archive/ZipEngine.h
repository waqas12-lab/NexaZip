#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include <QFile>

struct ZipEntryInfo {
    QString path;
    QString name;
    QString folder;
    QString type;
    qint64 size = 0;
    qint64 packedSize = 0;
    QDateTime modified;
    bool isDir = false;
    quint32 crc32 = 0;
    quint32 localHeaderOffset = 0;
    quint16 method = 0;
};

class ZipEngine {
public:
    static QVector<ZipEntryInfo> listZip(const QString& zipPath, QString* error);
    static bool createZip(const QStringList& inputPaths, const QString& outputZip, QString* error);
    static bool extractZip(const QString& zipPath, const QString& destination, QString* error);
    static bool testZip(const QString& zipPath, QString* error);

private:
    static bool addPath(QFile& out, const QString& path, const QString& baseName, QVector<ZipEntryInfo>& central, QString* error);
    static bool addFile(QFile& out, const QString& filePath, const QString& archiveName, QVector<ZipEntryInfo>& central, QString* error);
    static bool addDirectory(QFile& out, const QString& archiveName, QVector<ZipEntryInfo>& central, QString* error);
};
