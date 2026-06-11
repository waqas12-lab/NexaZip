
#include "ZipEngine.h"

#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <zlib.h>

namespace {
quint32 crcTable[256];
bool crcReady = false;

void initCrc() {
    if (crcReady) return;
    for (quint32 i = 0; i < 256; ++i) {
        quint32 c = i;
        for (int j = 0; j < 8; ++j)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crcTable[i] = c;
    }
    crcReady = true;
}

quint32 updateCrc(quint32 crc, const QByteArray& data) {
    initCrc();
    crc ^= 0xffffffffu;
    for (unsigned char b : data)
        crc = crcTable[(crc ^ b) & 0xff] ^ (crc >> 8);
    return crc ^ 0xffffffffu;
}

void writeU16(QFile& f, quint16 v) {
    char b[2] = { char(v & 0xff), char((v >> 8) & 0xff) };
    f.write(b, 2);
}
void writeU32(QFile& f, quint32 v) {
    char b[4] = { char(v & 0xff), char((v >> 8) & 0xff), char((v >> 16) & 0xff), char((v >> 24) & 0xff) };
    f.write(b, 4);
}
quint16 readU16(const QByteArray& d, int p) {
    if (p + 1 >= d.size()) return 0;
    return quint16(quint8(d[p])) | (quint16(quint8(d[p+1])) << 8);
}
quint32 readU32(const QByteArray& d, int p) {
    if (p + 3 >= d.size()) return 0;
    return quint32(quint8(d[p])) | (quint32(quint8(d[p+1])) << 8) | (quint32(quint8(d[p+2])) << 16) | (quint32(quint8(d[p+3])) << 24);
}
QString safePath(QString p) {
    p = QDir::fromNativeSeparators(p);
    while (p.startsWith('/')) p.remove(0,1);
    p.replace("..", "_");
    return p;
}
QString typeName(const QString& name, bool isDir) {
    if (isDir) return "Folder";
    QString suffix = QFileInfo(name).suffix().toUpper();
    return suffix.isEmpty() ? "File" : suffix + " File";
}
QByteArray deflateRaw(const QByteArray& input, int level, bool* ok) {
    *ok = false;
    if (input.isEmpty()) { *ok = true; return {}; }
    z_stream s{};
    if (deflateInit2(&s, level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) return {};
    QByteArray out;
    out.resize(compressBound(input.size()));
    s.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.constData()));
    s.avail_in = static_cast<uInt>(input.size());
    s.next_out = reinterpret_cast<Bytef*>(out.data());
    s.avail_out = static_cast<uInt>(out.size());
    int r = deflate(&s, Z_FINISH);
    if (r == Z_STREAM_END) { out.resize(s.total_out); *ok = true; } else out.clear();
    deflateEnd(&s);
    return out;
}
QByteArray inflateRaw(const QByteArray& input, quint32 expected, bool* ok) {
    *ok = false;
    z_stream s{};
    if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return {};
    QByteArray out;
    out.resize(expected ? int(expected) : int(input.size()*4 + 1024));
    s.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.constData()));
    s.avail_in = static_cast<uInt>(input.size());
    int r = Z_OK;
    while (r != Z_STREAM_END) {
        if (s.total_out >= static_cast<uLong>(out.size())) out.resize(out.size()*2 + 1024);
        s.next_out = reinterpret_cast<Bytef*>(out.data()+s.total_out);
        s.avail_out = static_cast<uInt>(out.size()-s.total_out);
        r = inflate(&s, Z_NO_FLUSH);
        if (r != Z_OK && r != Z_STREAM_END) { inflateEnd(&s); return {}; }
    }
    out.resize(s.total_out);
    inflateEnd(&s);
    if (expected && quint32(out.size()) != expected) return {};
    *ok = true;
    return out;
}
}

bool ZipEngine::createZip(const QStringList& inputPaths, const QString& outputZip, QString* error) {
    QFile out(outputZip);
    if (!out.open(QIODevice::WriteOnly)) {
        if (error) *error = "Cannot create ZIP file: " + outputZip;
        return false;
    }
    QVector<ZipEntryInfo> central;
    for (const QString& p : inputPaths) {
        QFileInfo info(p);
        if (!info.exists()) continue;
        if (!addPath(out, p, info.fileName(), central, error)) { out.close(); return false; }
    }
    quint32 centralStart = quint32(out.pos());
    for (const ZipEntryInfo& e : central) {
        QByteArray name = safePath(e.path).toUtf8();
        writeU32(out, 0x02014b50);
        writeU16(out, 20); writeU16(out, 20); writeU16(out, 0); writeU16(out, e.method);
        writeU16(out, 0); writeU16(out, 0); writeU32(out, e.crc32);
        writeU32(out, quint32(e.packedSize)); writeU32(out, quint32(e.size));
        writeU16(out, quint16(name.size())); writeU16(out, 0); writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0); writeU32(out, e.isDir ? 0x10 : 0);
        writeU32(out, e.localHeaderOffset); out.write(name);
    }
    quint32 centralEnd = quint32(out.pos());
    writeU32(out, 0x06054b50);
    writeU16(out, 0); writeU16(out, 0);
    writeU16(out, quint16(central.size())); writeU16(out, quint16(central.size()));
    writeU32(out, centralEnd-centralStart); writeU32(out, centralStart); writeU16(out, 0);
    return true;
}

bool ZipEngine::addPath(QFile& out, const QString& path, const QString& baseName, QVector<ZipEntryInfo>& central, QString* error) {
    QFileInfo info(path);
    if (info.isFile()) return addFile(out, path, safePath(baseName), central, error);
    if (!info.isDir()) return true;
    QString rootName = safePath(baseName + "/");
    if (!addDirectory(out, rootName, central, error)) return false;
    QDir baseDir(info.absoluteFilePath());
    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString current = it.next();
        QFileInfo child(current);
        QString arcName = safePath(baseName + "/" + baseDir.relativeFilePath(current));
        if (child.isDir()) {
            if (!arcName.endsWith('/')) arcName += "/";
            if (!addDirectory(out, arcName, central, error)) return false;
        } else if (child.isFile()) {
            if (!addFile(out, current, arcName, central, error)) return false;
        }
    }
    return true;
}

bool ZipEngine::addDirectory(QFile& out, const QString& archiveName, QVector<ZipEntryInfo>& central, QString*) {
    ZipEntryInfo e;
    e.path = safePath(archiveName.endsWith('/') ? archiveName : archiveName + "/");
    QString tmp = e.path; if (tmp.endsWith('/')) tmp.chop(1);
    e.name = QFileInfo(tmp).fileName();
    e.folder = QFileInfo(tmp).path() == "." ? "" : QFileInfo(tmp).path();
    e.type = "Folder"; e.isDir = true; e.method = 0; e.localHeaderOffset = quint32(out.pos());
    QByteArray name = e.path.toUtf8();
    writeU32(out, 0x04034b50);
    writeU16(out, 20); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0); writeU16(out, 0);
    writeU32(out, 0); writeU32(out, 0); writeU32(out, 0);
    writeU16(out, quint16(name.size())); writeU16(out, 0); out.write(name);
    central.push_back(e);
    return true;
}

bool ZipEngine::addFile(QFile& out, const QString& filePath, const QString& archiveName, QVector<ZipEntryInfo>& central, QString* error) {
    QFile in(filePath);
    if (!in.open(QIODevice::ReadOnly)) { if (error) *error = "Cannot open file: " + filePath; return false; }
    QByteArray data = in.readAll();
    quint32 crc = updateCrc(0, data);
    bool ok = false;
    QByteArray compressed = deflateRaw(data, Z_BEST_COMPRESSION, &ok);
    QByteArray payload = (ok && !compressed.isEmpty() && compressed.size() < data.size()) ? compressed : data;
    quint16 method = (payload.constData() == compressed.constData() && payload.size() == compressed.size() && payload.size() < data.size()) ? 8 : 0;
    ZipEntryInfo e;
    e.path = safePath(archiveName); e.name = QFileInfo(e.path).fileName();
    e.folder = QFileInfo(e.path).path() == "." ? "" : QFileInfo(e.path).path();
    e.type = typeName(e.name, false); e.isDir = false; e.size = data.size(); e.packedSize = payload.size();
    e.crc32 = crc; e.method = method; e.localHeaderOffset = quint32(out.pos()); e.modified = QFileInfo(filePath).lastModified();
    QByteArray name = e.path.toUtf8();
    writeU32(out, 0x04034b50);
    writeU16(out, 20); writeU16(out, 0); writeU16(out, e.method); writeU16(out, 0); writeU16(out, 0);
    writeU32(out, e.crc32); writeU32(out, quint32(e.packedSize)); writeU32(out, quint32(e.size));
    writeU16(out, quint16(name.size())); writeU16(out, 0); out.write(name); out.write(payload);
    central.push_back(e);
    return true;
}

QVector<ZipEntryInfo> ZipEngine::listZip(const QString& zipPath, QString* error) {
    QVector<ZipEntryInfo> entries;
    QFile f(zipPath);
    if (!f.open(QIODevice::ReadOnly)) { if (error) *error = "Cannot open ZIP file."; return entries; }
    QByteArray d = f.readAll();
    int eocd = -1;
    for (int i = d.size()-22; i >= 0 && i > d.size()-65557; --i) {
        if (readU32(d, i) == 0x06054b50) { eocd = i; break; }
    }
    if (eocd < 0) { if (error) *error = "Invalid ZIP file."; return entries; }
    quint16 count = readU16(d, eocd+10);
    int p = int(readU32(d, eocd+16));
    for (int i=0; i<count && p+46 <= d.size(); ++i) {
        if (readU32(d,p) != 0x02014b50) break;
        quint16 method = readU16(d,p+10);
        quint32 crc = readU32(d,p+16), packed = readU32(d,p+20), size = readU32(d,p+24);
        quint16 nameLen = readU16(d,p+28), extraLen = readU16(d,p+30), commentLen = readU16(d,p+32);
        quint32 localOffset = readU32(d,p+42);
        QString path = QDir::fromNativeSeparators(QString::fromUtf8(d.mid(p+46, nameLen)));
        bool isDir = path.endsWith('/');
        ZipEntryInfo e;
        e.path = path; e.isDir = isDir;
        QString tmp = path; if (tmp.endsWith('/')) tmp.chop(1);
        e.name = QFileInfo(tmp).fileName();
        e.folder = QFileInfo(tmp).path() == "." ? "" : QFileInfo(tmp).path();
        e.type = typeName(e.name, isDir); e.size = size; e.packedSize = packed; e.crc32 = crc; e.localHeaderOffset = localOffset; e.method = method;
        entries.push_back(e);
        p += 46 + nameLen + extraLen + commentLen;
    }
    return entries;
}

bool ZipEngine::extractZip(const QString& zipPath, const QString& destination, QString* error) {
    QFile f(zipPath);
    if (!f.open(QIODevice::ReadOnly)) { if (error) *error = "Cannot open ZIP file."; return false; }
    QByteArray d = f.readAll();
    QVector<ZipEntryInfo> entries = listZip(zipPath, error);
    if (entries.isEmpty() && error && !error->isEmpty()) return false;
    QDir().mkpath(destination);
    for (const ZipEntryInfo& e : entries) {
        QString outPath = QDir(destination).filePath(safePath(e.path));
        if (e.isDir) { QDir().mkpath(outPath); continue; }
        int p = int(e.localHeaderOffset);
        if (p+30 > d.size() || readU32(d,p) != 0x04034b50) { if (error) *error = "Invalid local file header."; return false; }
        int dataStart = p + 30 + readU16(d,p+26) + readU16(d,p+28);
        if (dataStart + e.packedSize > d.size()) { if (error) *error = "ZIP entry out of range."; return false; }
        QByteArray packed = d.mid(dataStart, int(e.packedSize));
        QByteArray bytes;
        if (e.method == 0) bytes = packed;
        else if (e.method == 8) {
            bool ok = false; bytes = inflateRaw(packed, quint32(e.size), &ok);
            if (!ok) { if (error) *error = "Deflate decompression failed for: " + e.path; return false; }
        } else { if (error) *error = "Unsupported ZIP method " + QString::number(e.method); return false; }
        if (updateCrc(0, bytes) != e.crc32) { if (error) *error = "CRC failed for: " + e.path; return false; }
        QDir().mkpath(QFileInfo(outPath).absolutePath());
        QFile out(outPath);
        if (!out.open(QIODevice::WriteOnly)) { if (error) *error = "Cannot write file: " + outPath; return false; }
        out.write(bytes);
    }
    return true;
}

bool ZipEngine::testZip(const QString& zipPath, QString* error) {
    QString tmp = QDir::temp().filePath("NexaZip_Test_" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    bool ok = extractZip(zipPath, tmp, error);
    QDir(tmp).removeRecursively();
    return ok;
}
