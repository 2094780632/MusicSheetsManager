#include "consql.h"
#include "config.h"

QAtomicPointer<Consql> Consql::m_instance = nullptr;
QMutex                 Consql::m_mutex;

Consql *Consql::instance(const QString &dbPath)
{
    if (m_instance.loadAcquire() == nullptr) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.loadAcquire() == nullptr) {
            Consql *obj = new Consql;
            QString path = dbPath.isEmpty()
                               ? AppConfig::instance()->getStoragePath() + "/data.db"
                               : dbPath;
            if (!obj->openDb(path)) {   // 打开失败
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
    if (m_db.isValid())                 // 已打开
        return true;

    QString conn = "ConsqlSingleton";   // 固定连接名
    m_db = QSqlDatabase::addDatabase("QSQLITE", conn);
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        qWarning() << "open DB failed" << m_db.lastError();
        m_db = QSqlDatabase();          // 还原空对象
        return false;
    }
    QSqlQuery q(m_db);
    //每次都要手动打开外键约束
    q.exec("PRAGMA foreign_keys = ON");
    return true;
}

void Consql::closeDb()
{
    if (m_db.isValid()) {
        QString conn = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();                     // 成员副本先清空
        QSqlDatabase::removeDatabase(conn);        // 再移除连接
    }
}
//Consql::Consql(bool flag){}

int Consql::initDb(){
    runFile("init.sql");
    qDebug("sqlite:init done");
    return 1;
}

void Consql::run(QString cmd){
    QSqlQuery q(m_db);
    if(!q.exec(cmd))
        qDebug()<<q.lastError().text();
}

bool Consql::runFile(QString fname){
    QFile f(":/sql/"+fname);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qFatal("sqlite:open init.sql failed");
        return false;
    }
    QString sql = f.readAll();
    f.close();

    //删除单行注释
    QRegularExpression re("--[^\n]*");
    sql = sql.replace(re, "");

    if (!m_db.transaction()) return false;
    QStringList scripts = sql.split(';', Qt::SkipEmptyParts);
    for (QString& s : scripts) {
        //QString trimmed = s.trimmed();        Qt版本问题，原本用QStringRef s会报错
        QString trimmed = s.simplified();   //掐头去尾
        qDebug()<<"sqlite:"<<trimmed;
        if (trimmed.isEmpty()) continue;
        QSqlQuery q(m_db);
        if (!q.exec(trimmed)) {
            m_db.rollback();
            qFatal() << "sqlite:init sql error:" << q.lastError().text();
        }
    }
    if (!m_db.commit())
        return false;
    return true;
}

// 确保目标目录存在，返回绝对路径
static QString ensureDir(const QString &absPath)
{
    QDir d;
    if (!d.mkpath(absPath))
        qWarning() << "mkpath failed" << absPath;
    return absPath;
}

// 复制文件到目标目录，返回新绝对路径
static QString copyTo(const QString &source, const QString &destDir)
{
    QFileInfo src(source);
    QString newFile = QDir(destDir).absoluteFilePath(src.fileName());
    if (QFile::copy(source, newFile))
        return newFile;
    qWarning() << "copy failed" << source << "->" << newFile;
    return QString();   // 失败返回空串
}

//插入乐谱
qint64 Consql::insertScore(const ScoreMeta &meta, const QStringList &files)
{
    if (files.isEmpty() || meta.name.isEmpty())
        return -1;

    QSqlQuery q(m_db);
    if (!m_db.transaction())
        return -1;

    //1.Song表
    q.prepare("INSERT INTO Song (s_name, s_composer, s_key, s_addDate, s_isFav,"
              " s_remark, s_version, s_type, c_id) "
              "VALUES (?, ?, ?, DATE('now'), 0, ?, ?, ?, ?)");
    q.addBindValue(meta.name);
    q.addBindValue(meta.composer);
    q.addBindValue(meta.key);
    q.addBindValue(meta.remark);
    q.addBindValue(meta.version);
    q.addBindValue(meta.type);
    q.addBindValue(meta.categoryId == 0 ? QVariant(QVariant::Int)
                                        : QVariant(meta.categoryId));
    if (!q.exec()) {
        m_db.rollback();
        qWarning() << "insert Song failed" << q.lastError();
        return -1;
    }
    qint64 songId = q.lastInsertId().toLongLong();

    //2.File表
    int index = 0;
    for (const QString &f : files) {
        q.prepare("INSERT INTO File (f_index, f_path, f_type, s_id) "
                  "VALUES (?, ?, ?, ?)");
        q.addBindValue(index++);
        q.addBindValue(QDir::toNativeSeparators(f));
        q.addBindValue(meta.type == "PDF" ? "pdf" : "img");
        q.addBindValue(songId);
        if (!q.exec()) {
            m_db.rollback();
            qWarning() << "insert File failed" << q.lastError();
            return -1;
        }
    }

    //3.复制文件
    QString repoRoot = AppConfig::instance()->getStoragePath();
    QString songDir  = ensureDir(QDir(repoRoot).absoluteFilePath(QString::number(songId)));

    for (const QString &oldPath : files) {
        QString newPath = copyTo(oldPath, songDir);
        if (newPath.isEmpty()) {
            //复制失败也继续，只警告
            qWarning() << "copy failed, keep original path" << oldPath;
            continue;
        }
        //更新数据库里这一条的路径
        QSqlQuery uq(m_db);
        uq.prepare("UPDATE File SET f_path = ? WHERE f_path = ? AND s_id = ?");
        uq.addBindValue(QDir::toNativeSeparators(newPath));
        uq.addBindValue(QDir::toNativeSeparators(oldPath));
        uq.addBindValue(songId);
        if (!uq.exec())
            qWarning() << "update f_path failed" << uq.lastError();
    }

    m_db.commit();
    return songId;   // 成功返回新 s_id
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

    qint64 rowsAffected = q.numRowsAffected();
    qDebug() << "deleteFileRecordsBySongId: Deleted" << rowsAffected << "file records for song ID" << songId;
    return true;
}


bool Consql::deleteSongRecordsBySongId(qint64 songId){
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

    qint64 rowsAffected = q.numRowsAffected();
    qDebug() << "deleteSongRecordsBySongId: Deleted" << rowsAffected << "Song records for song ID" << songId;
    return true;
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

bool Consql::updateScoreWithFiles(qint64 songId, const ScoreMeta &meta, const QStringList &files)
{
    if (songId <= 0 || files.isEmpty() || meta.name.isEmpty()) {
        qWarning() << "updateScoreWithFiles: Invalid parameters";
        return false;
    }

    // 开始事务
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
        q.addBindValue(meta.categoryId == 0 ? QVariant()  //使用空QVariant
                                            : QVariant(meta.categoryId));
        q.addBindValue(songId);

        if (!q.exec()) {
            throw std::runtime_error(QString("Failed to update song: %1")
                                         .arg(q.lastError().text()).toStdString());
        }

        // 2. 获取旧的文件路径（用于删除旧的物理文件）
        QStringList oldFiles = getFilePathsBySongId(songId);

        // 3. 删除旧的文件记录
        if (!deleteFileRecordsBySongId(songId)) {
            throw std::runtime_error("Failed to delete old file records");
        }

        // 4. 插入新的文件记录并处理物理文件
        int index = 0;
        QString fileType = meta.type == "PDF" ? "pdf" : "img";

        // 确保歌曲目录存在
        QString repoRoot = AppConfig::instance()->getStoragePath();
        QString songDir  = ensureDir(QDir(repoRoot).absoluteFilePath(QString::number(songId)));

        // 存储实际插入到数据库的路径（用于后续清理）
        QSet<QString> newFileSet;

        for (const QString &sourcePath : files) {
            // 检查文件是否已经在歌曲目录中（避免重复复制）
            QString finalPath;

            QFileInfo fileInfo(sourcePath);
            QString targetPath = QDir(songDir).absoluteFilePath(fileInfo.fileName());

            // 如果源文件已经在目标目录中，直接使用原路径
            if (QFileInfo(sourcePath).canonicalFilePath() == QFileInfo(targetPath).canonicalFilePath()) {
                finalPath = sourcePath;
            } else {
                // 复制文件到歌曲目录（使用改进的copyTo处理文件名冲突）
                finalPath = copyTo(sourcePath, songDir);
                if (finalPath.isEmpty()) {
                    // 复制失败，使用原路径并记录警告
                    qWarning() << "copy failed, keep original path" << sourcePath;
                    finalPath = sourcePath;
                }
            }

            // 记录最终使用的路径
            newFileSet.insert(QFileInfo(finalPath).canonicalFilePath());

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

        // 5. 清理旧的物理文件（只删除不在新文件列表中的文件）
        qDebug() << "Old files to check for deletion: " << oldFiles;
        qDebug() << "New file set: " << newFileSet;

        for (const QString &oldFilePath : oldFiles) {
            QFileInfo oldFileInfo(oldFilePath);
            QString oldFileCanonicalPath = oldFileInfo.canonicalFilePath();

            qDebug() << "Checking old file: " << oldFileCanonicalPath;

            // 检查文件是否在歌曲目录中
            QFileInfo songDirInfo(songDir);
            QString songDirCanonicalPath = songDirInfo.canonicalFilePath() + QDir::separator();

            bool isInSongDir = oldFileCanonicalPath.startsWith(songDirCanonicalPath);
            qDebug() << "Is in song dir? " << isInSongDir
                     << " Song dir: " << songDirCanonicalPath;

            if (isInSongDir) {
                // 检查是否在新文件集中
                bool fileExists = false;
                for (const QString &newPath : newFileSet) {
                    if (oldFileCanonicalPath == QFileInfo(newPath).canonicalFilePath()) {
                        fileExists = true;
                        qDebug() << "File still exists in new set: " << newPath;
                        break;
                    }
                }

                // 如果文件不在新文件集中，删除它
                if (!fileExists) {
                    if (QFile::exists(oldFilePath)) {
                        qDebug() << "Deleting old physical file: " << oldFilePath;
                        if (!QFile::remove(oldFilePath)) {
                            qWarning() << "Failed to delete old physical file: " << oldFilePath;
                        } else {
                            qDebug() << "Successfully deleted old physical file: " << oldFilePath;
                        }
                    } else {
                        qDebug() << "Old file doesn't exist: " << oldFilePath;
                    }
                }
            } else {
                qDebug() << "Old file not in song dir, skipping: " << oldFilePath;
            }
        }

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
