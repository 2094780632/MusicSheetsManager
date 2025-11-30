#include "mainwindow.h"
#include "initdialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //初始化设置
    InitDialog i;
    i.show();

    //MainWindow w;
    //w.show();
    return a.exec();
}
