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

void InitDialog::on_pushButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("选择目录"),
        ui->PathlineEdit->text());
    if (!dir.isEmpty())
        ui->PathlineEdit->setText(dir);
}

