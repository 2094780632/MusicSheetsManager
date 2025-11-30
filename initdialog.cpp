#include "initdialog.h"
#include "ui_initdialog.h"
#include <QFileDialog>

InitDialog::InitDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InitDialog)
{
    ui->setupUi(this);
}

InitDialog::~InitDialog()
{
    delete ui;
}
\
//选择目录按钮的槽函数
void InitDialog::on_PathpushButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("选择目录"),
        ui->PathlineEdit->text());
    if (!dir.isEmpty())
        ui->PathlineEdit->setText(dir);
}

