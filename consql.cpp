#include "consql.h"
#include "config.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <QSqlError>

QAtomicPointer<Consql> Consql::m_instance = nullptr;
QMutex Consql::m_mutex;

// 确保目标目录存在，返回绝对路径
static QString ensureDir(const QString &absPath)
{
    QDir d;
    if (!d.mkpath(absPath))
        qWarning() << "mkpath failed:" << absPath;
    return absPath;
}

// 复制文件到目标目录，返回新绝对路径（处理文件名冲突）
static QString copyTo(const QString &source, const QString &destDir)
{
    QFileInfo src(source);
    if (!src.exists()) {
        qWarning() << "Source file does not exist:" << source;
        return QString();
    }

    QString baseName = src.completeBaseName();
    QString suffix = src.suffix();

    // 构建初始文件名
    QString fileName = src.fileName();
    QString newFile = QDir(destDir).absoluteFilePath(fileName);

    // 处理文件名冲突
    int counter = 1;
    while (QFile::exists(newFile)) {
        // 如果文件已存在，尝试添加数字后缀
        fileName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix);
        newFile = QDir(destDir).absoluteFilePath(fileName);
        counter++;

        // 避免无限循环
        if (counter > 1000) {
            qWarning() << "Too many file name conflicts for:" << source;
            return QString();
        }
    }

    // 确保目标目录存在
    QDir().mkpath(QFileInfo(newFile).path());

    if (QFile::copy(source, newFile)) {
        qDebug() << "Copied:" << source << "->" << newFile;
        return newFile;
    }

    qWarning() << "Copy failed:" << source << "->" << newFile;
    return QString();
}

Consql *Consql::instance(const QString &dbPath)
{
    if (m_instance.loadAcquire() == nullptr) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.loadAcquire() == nullptr) {
            Consql *obj = new Consql;
            QString path = dbPath.isEmpty()
                               ? AppConfig::instance()->getStoragePath() + "/data.db"
                               : dbPath;
            if (!obj->openDb(path)) {
                delete obj;
                return nullptr;
            }
            m_instance.storeRelease(obj);
        }
    }
    return m_instance.loadAcquire();
}

Consql::Consql(QObject *parent) : QObject(parent) {}

Consql::~Consql()
{
    closeDb();
    m_instance.storeRelease(nullptr);
}

bool Consql::openDb(const QString &path)
{
    if (m_db.isValid())
        return true;

    QString conn = "ConsqlSingleton";
    m_db = QSqlDatabase::addDatabase("QSQLITE", conn);
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qWarning() << "Open DB failed:" << m_db.lastError();
        m_db = QSqlDatabase();
        return false;
    }

    // 启用外键约束
    QSqlQuery q(m_db);
    if (!q.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "Failed to enable foreign keys:" << q.lastError();
    }

    return true;
}

void Consql::closeDb()
{
    if (m_db.isValid()) {
        QString conn = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(conn);
    }
}

int Consql::initDb()
{
    return runFile("init.sql") ? 1 : 0;
}

void Consql::run(QString cmd)
{
    QSqlQuery q(m_db);
    if (!q.exec(cmd))
        qDebug() << "SQL Error:" << q.lastError().text() << "\nCommand:" << cmd;
}

bool Consql::runFile(QString fname)
{
    QFile f(":/sql/" + fname);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open SQL file:" << fname;
        return false;
    }

    QString sql = f.readAll();
    f.close();

    // 删除单行注释
    QRegularExpression re("--[^\n]*");
    sql = sql.replace(re, "");

    if (!m_db.transaction()) {
        qCritical() << "Failed to start transaction";
        return false;
    }

    QStringList scripts = sql.split(';', Qt::SkipEmptyParts);
    for (QString &s : scripts) {
        QString trimmed = s.simplified();
        if (trimmed.isEmpty()) continue;

        QSqlQuery q(m_db);
        if (!q.exec(trimmed)) {
            m_db.rollback();
            qCritical() << "SQL execution error:" << q.lastError().text() << "\nSQL:" << trimmed;
            return false;
        }
    }

    if (!m_db.commit()) {
        qCritical() << "Failed to commit transaction";
        return false;
    }

    qDebug() << "SQL file executed successfully:" << fname;
    return true;
}

qint64 Consql::insertScore(const ScoreMeta &meta, const QStringList &files)
{
    if (files.isEmpty() || meta.name.isEmpty()) {
        qWarning() << "insertScore: Invalid parameters";
        return -1;
    }

    QSqlQuery q(m_db);
    if (!m_db.transaction()) {
        qWarning() << "insertScore: Failed to start transaction";
        return -1;
    }

    try {
        // 1. 插入歌曲记录
        q.prepare("INSERT INTO Song (s_name, s_composer, s_key, s_addDate, s_isFav,"
                  " s_remark, s_version, s_type, c_id) "
                  "VALUES (?, ?, ?, DATE('now'), 0, ?, ?, ?, ?)");
        q.addBindValue(meta.name);
        q.addBindValue(meta.composer);
        q.addBindValue(meta.key);
        q.addBindValue(meta.remark);
        q.addBindValue(meta.version);
        q.addBindValue(meta.type);
        q.addBindValue(meta.categoryId == 0 ? QVariant() : QVariant(meta.categoryId));

        if (!q.exec()) {
            throw std::runtime_error(QString("Failed to insert song: %1")
                                         .arg(q.lastError().text()).toStdString());
        }

        qint64 songId = q.lastInsertId().toLongLong();
        qDebug() << "Inserted song ID:" << songId;

        // 2. 插入文件记录并复制文件
        QString repoRoot = AppConfig::instance()->getStoragePath();
        QString songDir = ensureDir(QDir(repoRoot).absoluteFilePath(QString::number(songId)));
        QString fileType = meta.type == "PDF" ? "pdf" : "img";

        int index = 0;
        for (const QString &sourcePath : files) {
            // 复制文件
            QString newPath = copyTo(sourcePath, songDir);
            QString finalPath = newPath.isEmpty() ? sourcePath : newPath;

            if (newPath.isEmpty()) {
                qWarning() << "Copy failed, using original path:" << sourcePath;
            }

            // 插入文件记录
            q.prepare("INSERT INTO File (f_index, f_path, f_type, s_id) "
                      "VALUES (?, ?, ?, ?)");
            q.addBindValue(index++);
            q.addBindValue(QDir::toNativeSeparators(finalPath));
            q.addBindValue(fileType);
            q.addBindValue(songId);

            if (!q.exec()) {
                throw std::runtime_error(QString("Failed to insert file: %1")
                                             .arg(q.lastError().text()).toStdString());
            }
        }

        // 3. 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        qDebug() << "insertScore: Successfully inserted song ID" << songId;
        return songId;

    } catch (const std::exception &e) {
        m_db.rollback();
        qWarning() << "insertScore error:" << e.what();
        return -1;
    }
}

bool Consql::updateScoreWithFiles(qint64 songId, const ScoreMeta &meta, const QStringList &files)
{
    if (songId <= 0 || files.isEmpty() || meta.name.isEmpty()) {
        qWarning() << "updateScoreWithFiles: Invalid parameters";
        return false;
    }

    if (!m_db.transaction()) {
        qWarning() << "updateScoreWithFiles: Failed to start transaction";
        return false;
    }

    try {
        QSqlQuery q(m_db);

        // 1. 更新歌曲基本信息
        q.prepare("UPDATE Song SET s_name = ?, s_composer = ?, s_key = ?, "
                  "s_remark = ?, s_version = ?, s_type = ?, c_id = ? "
                  "WHERE s_id = ?");
        q.addBindValue(meta.name);
        q.addBindValue(meta.composer);
        q.addBindValue(meta.key);
        q.addBindValue(meta.remark);
        q.addBindValue(meta.version);
        q.addBindValue(meta.type);
        q.addBindValue(meta.categoryId == 0 ? QVariant() : QVariant(meta.categoryId));
        q.addBindValue(songId);

        if (!q.exec()) {
            throw std::runtime_error(QString("Failed to update song: %1")
                                         .arg(q.lastError().text()).toStdString());
        }

        // 2. 获取旧的文件路径
        QStringList oldFiles = getFilePathsBySongId(songId);
        qDebug() << "Old files count:" << oldFiles.size();

        // 3. 删除旧的文件记录
        if (!deleteFileRecordsBySongId(songId)) {
            throw std::runtime_error("Failed to delete old file records");
        }

        // 4. 插入新的文件记录并复制文件
        int index = 0;
        QString fileType = meta.type == "PDF" ? "pdf" : "img";
        QString repoRoot = AppConfig::instance()->getStoragePath();
        QString songDir = ensureDir(QDir(repoRoot).absoluteFilePath(QString::number(songId)));

        // 存储新文件的规范路径
        QSet<QString> newFileCanonicalPaths;

        for (const QString &sourcePath : files) {
            QString finalPath;
            QFileInfo sourceInfo(sourcePath);

            if (!sourceInfo.exists()) {
                qWarning() << "Source file does not exist:" << sourcePath;
                continue;
            }

            // 检查文件是否已经在歌曲目录中
            bool alreadyInSongDir = false;
            QString sourceCanonicalPath = sourceInfo.canonicalFilePath();

            for (const QString &oldPath : oldFiles) {
                if (sourceCanonicalPath == QFileInfo(oldPath).canonicalFilePath()) {
                    alreadyInSongDir = true;
                    finalPath = oldPath;
                    qDebug() << "File already in song dir:" << finalPath;

                    // 关键修复：将已存在的文件路径也添加到新文件集合中
                    QFileInfo finalInfo(finalPath);
                    newFileCanonicalPaths.insert(finalInfo.canonicalFilePath());
                    break;
                }
            }

            if (!alreadyInSongDir) {
                // 复制文件到歌曲目录
                finalPath = copyTo(sourcePath, songDir);
                if (finalPath.isEmpty()) {
                    qWarning() << "Copy failed, using original path:" << sourcePath;
                    finalPath = sourcePath;
                }

                // 记录新文件的规范路径
                QFileInfo finalInfo(finalPath);
                newFileCanonicalPaths.insert(finalInfo.canonicalFilePath());
            }

            // 插入文件记录
            q.prepare("INSERT INTO File (f_index, f_path, f_type, s_id) "
                      "VALUES (?, ?, ?, ?)");
            q.addBindValue(index++);
            q.addBindValue(QDir::toNativeSeparators(finalPath));
            q.addBindValue(fileType);
            q.addBindValue(songId);

            if (!q.exec()) {
                throw std::runtime_error(QString("Failed to insert file: %1")
                                             .arg(q.lastError().text()).toStdString());
            }
        }

        // 5. 清理不再使用的物理文件
        qDebug() << "Cleaning up old physical files...";
        qDebug() << "Old files:" << oldFiles;
        qDebug() << "New files canonical paths:" << newFileCanonicalPaths;

        // 关键：获取歌曲目录的规范化路径
        QDir songDirObj(songDir);
        QString songDirCanonicalPath = songDirObj.canonicalPath();
        if (!songDirCanonicalPath.endsWith('/')) {
            songDirCanonicalPath += '/';
        }

        qDebug() << "Song dir canonical path:" << songDirCanonicalPath;

        int deletedCount = 0;

        for (const QString &oldFilePath : oldFiles) {
            QFileInfo oldInfo(oldFilePath);
            QString oldCanonicalPath = oldInfo.canonicalFilePath();

            qDebug() << "\nChecking old file:" << oldFilePath;
            qDebug() << "  Canonical path:" << oldCanonicalPath;

            // 检查文件是否在歌曲目录中（使用规范化路径）
            bool isInSongDir = oldCanonicalPath.startsWith(songDirCanonicalPath);
            qDebug() << "  In song dir?" << isInSongDir;

            if (isInSongDir) {
                // 检查文件是否在新文件集中
                bool isUsed = newFileCanonicalPaths.contains(oldCanonicalPath);
                qDebug() << "  Is used in new set?" << isUsed;

                if (!isUsed) {
                    qDebug() << "  Attempting to delete:" << oldFilePath;
                    if (QFile::exists(oldFilePath)) {
                        if (QFile::remove(oldFilePath)) {
                            deletedCount++;
                            qDebug() << "  Successfully deleted";
                        } else {
                            // 尝试使用规范化路径删除
                            if (QFile::remove(oldCanonicalPath)) {
                                deletedCount++;
                                qDebug() << "  Successfully deleted using canonical path";
                            } else {
                                qWarning() << "  Failed to delete:" << oldFilePath;
                                //qDebug() << "  Error:" << QFile::errorString();
                            }
                        }
                    } else {
                        qDebug() << "  File does not exist, skipping";
                    }
                }
            } else {
                qDebug() << "  Not in song dir, skipping";
            }
        }

        qDebug() << "\nDeleted" << deletedCount << "old files";

        // 6. 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        qDebug() << "updateScoreWithFiles: Successfully updated song ID" << songId;
        return true;

    } catch (const std::exception &e) {
        m_db.rollback();
        qWarning() << "updateScoreWithFiles error:" << e.what();
        return false;
    }
}

bool Consql::deleteFileRecordsBySongId(qint64 songId)
{
    if (songId <= 0) {
        qWarning() << "deleteFileRecordsBySongId: Invalid song ID" << songId;
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM File WHERE s_id = ?");
    q.addBindValue(songId);

    if (!q.exec()) {
        qWarning() << "deleteFileRecordsBySongId failed:" << q.lastError().text();
        return false;
    }

    qDebug() << "deleteFileRecordsBySongId: Deleted files for song ID" << songId;
    return true;
}

bool Consql::deleteSongRecordsBySongId(qint64 songId)
{
    if (songId <= 0) {
        qWarning() << "deleteSongRecordsBySongId: Invalid song ID" << songId;
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM Song WHERE s_id = ?");
    q.addBindValue(songId);

    if (!q.exec()) {
        qWarning() << "deleteSongRecordsBySongId failed:" << q.lastError().text();
        return false;
    }

    qDebug() << "deleteSongRecordsBySongId: Deleted song ID" << songId;
    return true;
}

bool Consql::deleteSongWithFiles(qint64 songId)
{
    if (songId <= 0) {
        qWarning() << "deleteSongWithFiles: Invalid song ID" << songId;
        return false;
    }

    // 1. 获取文件路径
    QStringList files = getFilePathsBySongId(songId);
    qDebug() << "Found" << files.size() << "files for song" << songId;

    // 2. 开始数据库事务
    if (!m_db.transaction()) {
        qWarning() << "deleteSongWithFiles: Failed to start transaction";
        return false;
    }

    try {
        QSqlQuery q(m_db);

        // 3. 删除文件记录
        q.prepare("DELETE FROM File WHERE s_id = ?");
        q.addBindValue(songId);
        if (!q.exec()) {
            throw std::runtime_error(QString("Failed to delete file records: %1")
                                         .arg(q.lastError().text()).toStdString());
        }

        // 4. 删除歌曲记录
        q.prepare("DELETE FROM Song WHERE s_id = ?");
        q.addBindValue(songId);
        if (!q.exec()) {
            throw std::runtime_error(QString("Failed to delete song record: %1")
                                         .arg(q.lastError().text()).toStdString());
        }

        int rowsAffected = q.numRowsAffected();
        if (rowsAffected == 0) {
            throw std::runtime_error("No song found with the given ID");
        }

        // 5. 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        // 6. 删除物理文件
        int filesDeleted = 0;
        QString repoRoot = AppConfig::instance()->getStoragePath();
        QString songDirPath = QDir(repoRoot).absoluteFilePath(QString::number(songId));

        for (const QString &filePath : files) {
            if (QFile::exists(filePath)) {
                qDebug() << "Deleting file:" << filePath;
                if (QFile::remove(filePath)) {
                    filesDeleted++;
                } else {
                    qWarning() << "Failed to delete file:" << filePath;
                }
            }
        }
        qDebug() << "Deleted" << filesDeleted << "physical files";

        // 7. 尝试删除歌曲目录（如果为空）
        QDir songDir(songDirPath);
        if (songDir.exists()) {
            QStringList entries = songDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
            if (entries.isEmpty()) {
                if (songDir.removeRecursively()) {
                    qDebug() << "Removed empty song directory:" << songDirPath;
                } else {
                    qWarning() << "Failed to remove song directory:" << songDirPath;
                }
            } else {
                qDebug() << "Song directory not empty, keeping:" << songDirPath;
            }
        }

        qDebug() << "deleteSongWithFiles: Successfully deleted song ID" << songId;
        return true;

    } catch (const std::exception &e) {
        m_db.rollback();
        qWarning() << "deleteSongWithFiles error:" << e.what();
        return false;
    }
}

QStringList Consql::getFilePathsBySongId(qint64 songId)
{
    QStringList paths;

    if (songId <= 0) {
        return paths;
    }

    QSqlQuery q(m_db);
    q.prepare("SELECT f_path FROM File WHERE s_id = ? ORDER BY f_index");
    q.addBindValue(songId);

    if (!q.exec()) {
        qWarning() << "getFilePathsBySongId failed:" << q.lastError().text();
        return paths;
    }

    while (q.next()) {
        paths.append(q.value(0).toString());
    }

    qDebug() << "getFilePathsBySongId: Found" << paths.size() << "files for song ID" << songId;
    return paths;
}
