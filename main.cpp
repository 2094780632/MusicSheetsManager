#include "mainwindow.h"
#include "initdialog.h"
#include "config.h"
#include "consql.h"

#include <QApplication>

// SQLite
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

// Path
#include <QDir>
#include <QApplication>
#include <QFile>

// Animation
#include <QSplashScreen>
#include <QPixmap>
#include <QThread>

// Messagebox
#include <QMessageBox>

bool isFirstRun()
{
    QSettings s("2024413670", "MusicSheetsManager");
    return !s.contains("hasRun");
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "Main: Hello World!";

    // 初始化设置
    // 初次启动？
    QString configPath;
    bool needInitDb = false;

    if (isFirstRun()) {
        qDebug() << "Main: 首次运行";

        InitDialog i;
        QObject::connect(&i, &InitDialog::configPathChanged,
                         [&configPath](const QString &path) {
                             configPath = path;
                         });

        if (i.exec() != QDialog::Accepted || configPath.isEmpty()) {
            // 点取消或×就结束
            qDebug() << "Main: 用户取消或路径为空，退出程序";
            QCoreApplication::exit(-1);
            return -1;
        }

        // 初始化 Config
        configPath = QDir::cleanPath(configPath + "/config.ini");
        QSettings s("2024413670", "MusicSheetsManager");
        s.setValue("path", configPath);
        s.setValue("hasRun", true);

        QDir().mkpath(QFileInfo(configPath).absolutePath());
        AppConfig::instance(configPath)->load();

        needInitDb = true;

        qDebug() << "Main: 首次运行配置完成，配置路径:" << configPath;
    } else {
        QSettings s("2024413670", "MusicSheetsManager");
        configPath = s.value("path").toString();
        QDir().mkpath(QFileInfo(configPath).absolutePath());
        AppConfig::instance(configPath)->load();
        needInitDb = false;

        qDebug() << "Main: 非首次运行，配置路径:" << configPath;
    }

    // 数据库初始化
    QString dbPath = QDir(AppConfig::instance()->getStoragePath()).filePath("data.db");
    qDebug() << "Main: 数据库路径:" << dbPath;

    if (!Consql::instance(dbPath)) {
        // 首次打开单例
        qDebug() << "Main: 无法打开数据库，退出程序";
        QMessageBox::critical(nullptr, "致命错误", "无法打开数据库！");
        return -1;
    }

    if (needInitDb) {
        qDebug() << "Main: 初始化数据库结构";
        Consql::instance()->initDb();
    }

    qDebug() << "Main: 测试数据库连接";
    Consql::instance()->run("SELECT name FROM sqlite_master WHERE type='table';");

    // 启动动画
    qDebug() << "Main: 显示启动动画";

    QPixmap splashPixmap(":/splash.png");
    splashPixmap = splashPixmap.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QSplashScreen splash(splashPixmap);
    splash.show();
    QThread::msleep(1000);

    qDebug() << "Main: 创建主窗口";
    MainWindow w;
    splash.finish(&w);
    w.show();

    // 退出前保存
    qDebug() << "Main: 程序启动完成，进入事件循环";
    int result = a.exec();

    qDebug() << "Main: 程序退出，保存配置";
    AppConfig::instance()->save();

    return result;
}
