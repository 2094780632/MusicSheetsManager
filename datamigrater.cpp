#include "datamigrater.h"
#include "ui_datamigrater.h"

DataMigrater::DataMigrater(QWidget *parent)
    : QTabWidget(parent)
    , ui(new Ui::DataMigrater)
{
    ui->setupUi(this);

    setWindowFlag(Qt::Window, true);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("数据管理");
    setCurrentIndex(0);
}

DataMigrater::~DataMigrater()
{
    delete ui;
}

void DataMigrater::on_export_pushButton_clicked(){
    QMessageBox::information(this, "待开发！", "由于时间限制，暂时还没有搞定Quazip的使用，此功能待开发！");
}
void DataMigrater::on_import_pushButton_clicked(){
    QMessageBox::information(this, "待开发！", "由于时间限制，暂时还没有搞定Quazip的使用，此功能待开发！");
}
