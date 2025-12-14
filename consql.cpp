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
