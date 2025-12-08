#include "consql.h"
#include "config.h"

Consql::Consql() {
    qDebug("sqlite:connect");
    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();
    dbPath = QDir::cleanPath(AppConfig::instance()->getStoragePath()+"/data.db");
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qFatal("sqlite:failed");
        QMessageBox::critical(nullptr, "数据库错误", db.lastError().text());
        //return -1;
    }
}

//Consql::Consql(bool flag){}

int Consql::initDb(){
    QFile f(":/init.sql");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qFatal("sqlite:open init.sql failed");
    }
    QString sql = f.readAll();
    f.close();

    //删除单行注释
    QRegularExpression re("--[^\n]*");
    sql = sql.replace(re, "");

    if (!db.transaction()) return -1;
    QStringList scripts = sql.split(';', Qt::SkipEmptyParts);
    for (QString& s : scripts) {
        //QString trimmed = s.trimmed();        Qt版本问题，原本用QStringRef s会报错

        QString trimmed = s.simplified();   //掐头去尾
        qDebug()<<"sqlite:"<<trimmed;
        if (trimmed.isEmpty()) continue;
        QSqlQuery q(db);
        if (!q.exec(trimmed)) {
            db.rollback();
            qFatal() << "sqlite:init sql error:" << q.lastError().text();
        }
    }
    db.commit();
    qDebug("sqlite:init done");
    return 1;
}

void Consql::run(QString cmd){
    QSqlQuery q(db);
    if(!q.exec(cmd))
        qDebug()<<q.lastError().text();
}
