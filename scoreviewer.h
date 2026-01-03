#ifndef SCOREVIEWER_H
#define SCOREVIEWER_H

#pragma once
#include <QWidget>
#include <QtPdf/QPdfDocument>
#include <QPdfView>
#include <QPdfPageNavigator>

#include <QPixmap>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

#include <QFile>
#include <QDebug>

#include <QWheelEvent>
#include <QScrollBar>
#include <QShortcutEvent>
#include <QShortcut>

namespace Ui {
class ScoreViewer;
}

class ScoreViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ScoreViewer(qint64 songId,QWidget *parent = nullptr);
    ~ScoreViewer();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // 缩放相关
    void zoomPdf(qreal factor);     //响应缩放
    void zoomPdfIn();               //放大
    void zoomPdfOut();              //缩小
    void resetPdfTransform();       //重置缩放
    //void updateZoomLabel();

    //翻页相关
    void goToPage(int pageIndex);   //跳转到指定页面
    void prevPage();                //上一页
    void nextPage();                //下一页

    void loadSongInfo(qint64 id);   //加载歌曲详情
    void loadScore(qint64 id);      //渲染乐谱
    void showPdf();                 //展示 PDF类型的 乐谱
    void showImg(qint64 index);     //展示 图片类型的 乐谱
    void addPdfShortcuts();         //为 PDF类型 添加快捷键

private:
    Ui::ScoreViewer *ui;

    /*
    s_id INTEGER PRIMARY KEY AUTOINCREMENT, --id
    s_name TEXT NOT NULL, --歌名
    s_composer TEXT, --作者
    s_key TEXT, --调性
    s_addDate TEXT NOT NULL DEFAULT (DATE('now')), --yyyy-MM--dd
    s_isFav BOOLEAN NOT NULL DEFAULT 0,
    s_remark TEXT, --备注
    s_version TEXT, --版本
    s_type TEXT, --导入类型
    c_id INTEGER, --外键 分类
    */
    qint64 m_songId;
    struct songInfo
    {
        qint64 m_s_id;
        QString m_s_name;
        QString m_s_composer;
        QString m_s_key;
        QString m_s_addDate;
        bool m_s_isFav;
        QString m_s_remark;
        QString m_s_version;
        QString m_s_type;
        qint64 m_c_id;
    }si;

    QString m_f_path;
    QString m_f_type;
    QPdfView *m_pdfView = new QPdfView(this);
    QPdfDocument *m_doc = new QPdfDocument(this);

    int m_currentPage = 0;         // 当前页码（从0开始）
    int m_totalPages = 0;          // 总页数

};


#endif // SCOREVIEWER_H
