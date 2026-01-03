#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

#include <QDesktopServices>
#include <QUrl>

#include <QtPdf/QPdfDocument>
#include <QtPdf/QPdfPageRenderer>

#include <QSettings>
#include <QStandardItemModel>
#include <QMessageBox>

#include <QDate>

#include <QMenu>
#include <QAction>

//#include "metronomewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    int importSheet();                              //转到 导入乐谱 窗口
    void toScoreViewer(const QModelIndex &idx);     //转到 乐谱浏览器 以索引方式
    void toScoreViewer(const int sid);              //转到 乐谱浏览器 以s_id方式
    void toManager();                               //转到 数据管理器
    void showSongInfo(qint64 songId);               //乐谱右键菜单 获取歌曲详情
    void toMetronome();                             //转到 节拍器
    void toDM();                                    //转到 数据迁移工具
    void helpPage();                                //打开 帮助页面
    void userManual();                              //打开 用户手册页面
    void deleteFirstStartSign();                    //清除 非首次启动标识
    void clearData();                               //清除 数据
    void onListViewCustomMenu(const QPoint &pos);   //乐谱右键菜单
    void onLinkActivated(const QString &link);      //用户手册页面 超链接跳转

private:
    Ui::MainWindow *ui;
    QStandardItemModel *m_scoreModel;
    QStandardItemModel *m_treeModel; //treeView
    enum Dim { ByAll = 0, ByName, ByComposer, ByKey, ByCategory };
    void rebuildTree(Dim dim);

    void refreshScoreGrid(Dim dim = ByAll, const QString &value = {});
    //内容指纹锁
    struct FilterCache {
        Dim  dim  = ByAll;
        QString value = "-";
    };
    FilterCache m_lastFilter;

};
#endif // MAINWINDOW_H
