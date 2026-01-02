#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

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
    int importSheet();
    void toScoreViewer(const QModelIndex &idx);
    void toScoreViewer(const int sid);
    void toManager();
    void showSongInfo(qint64 songId);
    void toMetronome();
    void toDM();
    void helpPage();
    void userManual();
    void deleteFirstStartSign();
    void clearData();
    void onListViewCustomMenu(const QPoint &pos);

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
