#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QSettings>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //窗体初始化 ui 里写了
    //setWindowTitle("Music Sheets Manager");

    //spliter初始宽度设置
    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,5);

    //连接
    //帮助页面
    connect(ui->action_about,&QAction::triggered,this,&MainWindow::helpPage);
    //重置初次启动标识
    connect(ui->action_R,&QAction::triggered,this,&MainWindow::deleteFirstStartSign);
    //退出应用
    connect(ui->action_X, &QAction::triggered, qApp, &QCoreApplication::quit);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//帮助页面
void MainWindow::helpPage(){
    QMessageBox::information(this,
                             "帮助",
                             "Music Sheets Manager ver 1.0");
    qDebug("helpPage");
}

//重置初次启动标识
void MainWindow::deleteFirstStartSign(){
    QSettings s("2024413670", "MusicSheetsManager");
    s.clear();
    QMessageBox::information(this,
                             "成功！",
                             "已清除注册表！");
    qDebug("deleteFirstStartSign");
}
