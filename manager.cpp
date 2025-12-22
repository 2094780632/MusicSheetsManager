#include "manager.h"
#include "ui_manager.h"
#include "consql.h"

Manager::Manager(QWidget *parent)
    : QTabWidget(parent)
    , ui(new Ui::Manager)
{
    ui->setupUi(this);

    //调性选项设置
    QStringList keys = {
        //大调
        "C",
        "G",  "D",  "A",  "E",  "B",  "F♯",  "C♯",
        "F",  "B♭", "E♭", "A♭", "D♭", "G♭", "C♭",
        //和声小调
        "Cm",
        "Gm", "Dm", "Am", "Em", "Bm", "F♯m", "C♯m",
        "Fm", "B♭m","E♭m","A♭m","D♭m","G♭m","C♭m",
        //其他
        "其他"
    };
    ui->s_key_comboBox->addItems(keys);

}

Manager::~Manager()
{
    delete ui;
}

void Manager::loadSongList(){
    //SELECT s_id, s_name FROM Song
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id, s_name FROM Song ORDER BY s_id");
    while(q.next()){

    }
}
