#ifndef CONSQL_H
#define CONSQL_H

#include <QObject>
#include <QAtomicPointer>
#include <QMutex>
//SQLite
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QDir>
#include <QFile>
#include <QString>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDebug>


class Consql : public QObject
{
    Q_OBJECT
public:
    //单例
    static Consql *instance(const QString &dbPath = QString());

    Consql(const Consql &) = delete;
    Consql &operator=(const Consql &) = delete;

    bool openDb(const QString &dbPath); // 首次或重新打开
    void closeDb();                     // 关闭并移除连接
    bool isOpen() const { return m_db.isOpen(); }
    bool deleteSongWithFiles(qint64 songId);


    struct ScoreMeta {
        QString name;
        QString composer;
        QString key;
        int     categoryId;   // 0 表示 NULL
        QString version;
        QString type;        // "PDF" / "PNG"
        QString remark;
    };

    int initDb();       //初始化数据库
    QSqlDatabase database() const { return m_db; };//只读访问
    void run(QString cmd);
    bool runFile(QString fname);

    bool deleteFileRecordsBySongId(qint64 songId);
    bool deleteSongRecordsBySongId(qint64 songId);
    qint64 insertScore(const ScoreMeta &meta, const QStringList &files);
    bool updateScoreWithFiles(qint64 songId, const ScoreMeta &meta, const QStringList &files);
    QStringList getFilePathsBySongId(qint64 songId);

private:
    explicit Consql(QObject *parent = nullptr);
    ~Consql();

    QSqlDatabase        m_db;
    static QMutex       m_mutex;
    static QAtomicPointer<Consql> m_instance;
};

#endif // CONSQL_H
