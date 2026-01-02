#include "initdialog.h"
#include "ui_initdialog.h"
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

InitDialog::InitDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InitDialog)
{
    ui->setupUi(this);

    // 默认值设置为文档下的应用文件夹
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString folder = QDir(docDir).absoluteFilePath("MusicSheetsManager");   // Documents/MusicSheetsManager
    ui->PathlineEdit->setText(folder);

    qDebug() << "InitDialog: 构造函数，默认路径:" << folder;
}

InitDialog::~InitDialog()
{
    qDebug() << "InitDialog: 析构函数";
    delete ui;
}

// 选择目录按钮的槽函数
void InitDialog::on_PathpushButton_clicked()
{
    qDebug() << "InitDialog: 点击选择目录按钮";

    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("选择目录"),
        ui->PathlineEdit->text());

    if (!dir.isEmpty()) {
        ui->PathlineEdit->setText(dir);
        qDebug() << "InitDialog: 选择的目录:" << dir;
    } else {
        qDebug() << "InitDialog: 用户取消目录选择";
    }
}

void InitDialog::on_buttonBox_accepted()
{
    qDebug() << "InitDialog: 点击确定按钮";

    QString dir = ui->PathlineEdit->text().trimmed();
    if (dir.isEmpty()) {
        qDebug() << "InitDialog: 目录为空，显示警告";
        QMessageBox::warning(this, "提示", "请先填写配置目录");
        // 退出
        // std::exit(-1);
        return;
    }

    qDebug() << "InitDialog: 发送配置路径信号:" << dir;
    emit configPathChanged(dir);   // 把数据发出去
    accept();
}
