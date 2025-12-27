#include "mainwindow.h"
#include "initdialog.h"
#include "config.h"
#include "consql.h"

#include <QApplication>

//SQLite
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

//Path
#include <QDir>
#include <QApplication>
#include <QFile>


//Animation
#include <QSplashScreen>
#include <QPixmap>
#include <QThread>

//Messagebox
#include <QMessageBox>

bool isFirstRun()
{
    QSettings s("2024413670", "MusicSheetsManager");
    return !s.contains("hasRun");
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug("Hello World!");
    // 初始化设置
    //初次启动？
    QString configPath;
    bool needInitDb=false;

    if(isFirstRun()){
        qDebug("FistRun!");
        InitDialog i;

        QObject::connect(&i, &InitDialog::configPathChanged,
                         [&configPath](const QString &path){
                             configPath = path;
                         });

        if (i.exec() != QDialog::Accepted||configPath.isEmpty())   // 点取消或×就结束
        {
            QCoreApplication::exit(-1);
            return -1;
        }

        //初始化Config
        configPath = QDir::cleanPath(configPath+"/config.ini");
        QSettings s("2024413670","MusicSheetsManager");
        s.setValue("path",configPath);
        s.setValue("hasRun", true);

        QDir().mkpath(QFileInfo(configPath).absolutePath());
        //qDebug()<<configPath;
        AppConfig::instance(configPath)->load();

        needInitDb=true;
    }else{
        QSettings s("2024413670","MusicSheetsManager");
        configPath = s.value("path").toString();
        QDir().mkpath(QFileInfo(configPath).absolutePath());
        AppConfig::instance(configPath)->load();
        needInitDb=false;
    }
    // 数据库初始化
    QString dbPath = QDir(AppConfig::instance()->getStoragePath()).filePath("data.db");
    if (!Consql::instance(dbPath)){  //首次打开单例
        QMessageBox::critical(nullptr, "致命错误", "无法打开数据库！");
        return -1;
    }

    if(needInitDb)
        Consql::instance()->initDb();
    Consql::instance()->run("SELECT name FROM sqlite_master WHERE type='table';");

    //启动动画
    /*
    QSplashScreen splash(QPixmap(":/splash.png"));
    splash.show();
    QThread::msleep(1000);
    */
    MainWindow w;
    //splash.finish(&w);
    w.show();

    //退出前保存
    AppConfig::instance()->save();
    return a.exec();
}
