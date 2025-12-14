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

    enum Dim { ByAll = -1, ByCategory, ByComposer, ByKey, ByName };
    void refreshScoreGrid(Dim dim = ByAll, const QString &value = {});

private slots:
    int importSheet();
    void helpPage();
    void deleteFirstStartSign();
    void clearData();
    void onListViewCustomMenu(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *m_scoreModel;
};
#endif // MAINWINDOW_H
