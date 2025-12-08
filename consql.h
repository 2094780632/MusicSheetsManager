#ifndef CONSQL_H
#define CONSQL_H

//SQLite
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QDir>
#include <QFile>
#include <QString>

#include <QMessageBox>
#include <QDebug>

class Consql
{
public:
    Consql();           //创建连接
    //Consql(bool flag);  //空初始化 已弃用
    int initDb();       //初始化数据库
    void run(QString cmd);
private:
    QString dbPath;
    QSqlDatabase db;
};

#endif // CONSQL_H
