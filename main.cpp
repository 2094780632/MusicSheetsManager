#include "mainwindow.h"
#include "initdialog.h"
#include "config.h"

#include <QDir>
#include <QApplication>
#include <QFile>

#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QThread>

bool isFirstRun()
{
    QSettings s("2024413670", "MusicSheetsManager");
    bool first = !s.contains("hasRun");
    if (first)
        s.setValue("hasRun", true);//标识非第一次启动
    return first;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //初始化设置
    //初次启动？
    QString configPath;
    if(isFirstRun()){
        qDebug("FistRun!");
        InitDialog i;


        QObject::connect(&i, &InitDialog::configPathChanged,
                         [&configPath](const QString &path){
                             configPath = path;
                         });

        if (i.exec() != QDialog::Accepted)   // 点取消或×就结束
            return 0;

        //初始化Config
        configPath = QDir::cleanPath(configPath+"/config.ini");
        QSettings s("2024413670","MusicSheetsManager");
        s.setValue("path",configPath);
        QDir().mkpath(QFileInfo(configPath).absolutePath());
        //qDebug()<<configPath;
        AppConfig::instance(configPath)->load();
    }else{
        QSettings s("2024413670","MusicSheetsManager");
        configPath = s.value("path").toString();
        QDir().mkpath(QFileInfo(configPath).absolutePath());
        AppConfig::instance(configPath)->load();
    }


    //启动动画
    QSplashScreen splash(QPixmap(":/splash.png"));
    splash.show();
    QThread::msleep(1000);

    MainWindow w;
    splash.finish(&w);
    w.show();

    //退出前保存
    AppConfig::instance()->save();
    return a.exec();
}
