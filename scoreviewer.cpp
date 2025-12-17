#include "scoreviewer.h"
#include "ui_scoreviewer.h"
#include "consql.h"

ScoreViewer::ScoreViewer(qint64 songId,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScoreViewer)
{
    ui->setupUi(this);

    //窗口 默认最大化
    setWindowFlag(Qt::Window, true);
    setWindowState(windowState() | Qt::WindowMaximized);
    setAttribute(Qt::WA_DeleteOnClose);

    loadScore(songId);
}

ScoreViewer::~ScoreViewer()
{
    delete ui;
}

void ScoreViewer::loadScore(qint64 id){
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT f_path, f_type FROM File WHERE s_id=? ORDER BY f_index LIMIT 1");
    q.addBindValue(id);
    if (!q.exec() || !q.next()) {
        qDebug()<<"ScoreViewer:无关联文件";
        return;
    }

    QString fPath = q.value(0).toString();
    QString fType = q.value(1).toString();

 }
