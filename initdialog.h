#ifndef INITDIALOG_H
#define INITDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

namespace Ui {
class InitDialog;
}

class InitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitDialog(QWidget *parent = nullptr);
    ~InitDialog();

private slots:
    void on_PathpushButton_clicked();               //设置初始化路径

    void on_buttonBox_accepted();                   //完成按钮 槽函数

signals:
    void configPathChanged(const QString &path);    //文件路径改变信号

private:
    Ui::InitDialog *ui;
};

#endif // INITDIALOG_H
