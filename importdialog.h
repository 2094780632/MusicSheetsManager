#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QStringListModel>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

namespace Ui {
class ImportDialog;
}

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportDialog(QWidget *parent = nullptr);
    ~ImportDialog();
    void loadCategories();                                  //获取 分类信息

    qint64 insertScore();                                   //导入乐谱

private slots:
    void on_fPathpushButton_clicked();                      //文件路径选择 按钮 槽
    void on_buttonBox_accepted();                           //完成 槽
    void onFileListDoubleClicked(const QModelIndex &index); //双击时 删除文件
    void onTypeChanged(int index);                          //当 被双击时 删除被点击的文件

private:
    Ui::ImportDialog *ui;
    QStringListModel *m_fileModel;
};

#endif // IMPORTDIALOG_H
