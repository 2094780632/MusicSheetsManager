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
    //单例实现
    static Consql *instance(const QString &dbPath = QString());

    Consql(const Consql &) = delete;
    Consql &operator=(const Consql &) = delete;

    bool openDb(const QString &dbPath);             // 首次或重新打开
    void closeDb();                                 // 关闭并移除连接
    bool isOpen() const { return m_db.isOpen(); }
    bool deleteSongWithFiles(qint64 songId);

    //数据结构
    struct ScoreMeta {
        QString name;
        QString composer;
        QString key;
        int     categoryId;   // 0 表示 NULL
        QString version;
        QString type;        // "PDF" / "PNG"
        QString remark;
    };

    int initDb();                                   //初始化数据库
    QSqlDatabase database() const { return m_db; }; //只读访问
    void run(QString cmd);                          //运行 指定指令
    bool runFile(QString fname);                    //运行 指定SQL脚本文件

    bool deleteFileRecordsBySongId(qint64 songId);                                              //删除文件记录 以s_id
    bool deleteSongRecordsBySongId(qint64 songId);                                              //删除歌曲记录 以s_id
    qint64 insertScore(const ScoreMeta &meta, const QStringList &files);                        //新增乐谱记录
    bool updateScoreWithFiles(qint64 songId, const ScoreMeta &meta, const QStringList &files);  //更新乐谱文件记录
    QStringList getFilePathsBySongId(qint64 songId);                                            //获取文件路径 以s_id

private:
    explicit Consql(QObject *parent = nullptr);
    ~Consql();

    QSqlDatabase        m_db;
    static QMutex       m_mutex;
    static QAtomicPointer<Consql> m_instance;
};

#endif // CONSQL_H
