#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //窗体初始化
    setWindowTitle("MusicSheets Manager");
}

MainWindow::~MainWindow()
{
    delete ui;
}
