#include "initdialog.h"
#include "ui_initdialog.h"


InitDialog::InitDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InitDialog)
{
    ui->setupUi(this);

    //默认值设置为文档下的应用文件夹
    QString docDir  = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString Folder= QDir(docDir).absoluteFilePath("MusicSheetsManager");   // Documents/MusicSheetsManager
    ui->PathlineEdit->setText(Folder);

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

void InitDialog::on_buttonBox_accepted()
{
    QString dir = ui->PathlineEdit->text().trimmed();
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先填写配置目录");
        //退出
        //std::exit(-1);
        return;
    }


    emit configPathChanged(dir);   // 把数据发出去
    accept();
}

