#include "metronome.h"
#include "ui_metronome.h"

Metronome::Metronome(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Metronome)
{
    ui->setupUi(this);

    setWindowFlag(Qt::Window, true);
    setWindowTitle("节拍器");
    connect(ui->pushButton_start, &QPushButton::clicked,
            this, [this]{
                QMessageBox::information(this, "待开发！", "由于时间限制，暂时还没有搞定声音模块的使用，此功能待开发！");
            });
    connect(ui->pushButton_reset, &QPushButton::clicked,
            this, [this]{
                QMessageBox::information(this, "待开发！", "由于时间限制，暂时还没有搞定声音模块的使用，此功能待开发！");
            });
}

Metronome::~Metronome()
{
    delete ui;
}
