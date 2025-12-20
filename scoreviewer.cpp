#include "scoreviewer.h"
#include "ui_scoreviewer.h"
#include "consql.h"

ScoreViewer::ScoreViewer(qint64 songId,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScoreViewer)
{
    ui->setupUi(this);

    //为graphicsView的视口安装事件过滤器
    ui->graphicsView->viewport()->installEventFilter(this);

    //窗口 默认最大化
    setWindowFlag(Qt::Window, true);
    setWindowState(windowState() | Qt::WindowMaximized);
    setAttribute(Qt::WA_DeleteOnClose);

    //加载并显示信息
    loadSongInfo(songId);
    setWindowTitle(si.m_s_name);
    ui->name->setText(si.m_s_name);
    ui->composer->setText(si.m_s_composer);
    ui->key->setText(si.m_s_key);
    ui->addDate->setText(si.m_s_addDate);
    ui->version->setText(si.m_s_version);
    ui->remark->setText(si.m_s_remark);

    //显示乐谱
    loadScore(songId);

    addPdfShortcuts();

    //翻页按钮
    connect(ui->btnPrevPage, &QPushButton::clicked, this, &ScoreViewer::prevPage);
    connect(ui->btnNextPage, &QPushButton::clicked, this, &ScoreViewer::nextPage);
    ui->btnPrevPage->setShortcut(QKeySequence(Qt::Key_Left));
    ui->btnNextPage->setShortcut(QKeySequence(Qt::Key_Right));
}

ScoreViewer::~ScoreViewer()
{
    delete ui;
}

void ScoreViewer::loadSongInfo(qint64 id){
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id,s_name,s_composer,s_key,s_addDate,s_isFav,s_remark,s_version,s_type,c_id FROM Song WHERE s_id=? LIMIT 1");
    q.addBindValue(id);
    if (!q.exec() || !q.next()) {
        qDebug()<<"ScoreViewer:查找歌曲失败";
        return;
    }

    si.m_s_id=q.value(0).toInt();
    si.m_s_name=q.value(1).toString();
    si.m_s_composer=q.value(2).toString();
    si.m_s_key=q.value(3).toString();
    si.m_s_addDate=q.value(4).toString();
    si.m_s_isFav=q.value(5).toBool();
    si.m_s_remark=q.value(6).toString();
    si.m_s_version=q.value(7).toString();
    si.m_s_type=q.value(8).toString();
    si.m_c_id=q.value(9).toInt();

}

void ScoreViewer::loadScore(qint64 id){
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT f_path, f_type, (SELECT COUNT(*) FROM File WHERE s_id = ?) FROM File WHERE s_id=? ORDER BY f_index LIMIT 1");
    q.addBindValue(id);
    q.addBindValue(id);
    if (!q.exec() || !q.next()) {
        qDebug()<<"ScoreViewer:无关联文件";
        return;
    }else{
        m_f_path = q.value(0).toString();
        m_f_type = q.value(1).toString();
        m_totalPages = q.value(2).toInt();
        if (m_f_type == "pdf") {
            showPdf();
        } else {
            showImg(0);
            ui->graphicsView->scale(0.3, 0.3);
        }
    }


 }

bool ScoreViewer::eventFilter(QObject *obj, QEvent *event)
{
    if (m_pdfView && m_doc && obj == m_pdfView->viewport()) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

            // 检查是否按下了Ctrl键
            if (wheelEvent->modifiers() & Qt::ControlModifier) {
                // 计算缩放因子
                qreal scaleFactor = 1.1; // 10%
                if (wheelEvent->angleDelta().y() > 0) {
                    // 向上滚动，放大
                    //zoomPdf(scaleFactor);
                    zoomPdfIn();
                } else {
                    // 向下滚动，缩小
                    //zoomPdf(1.0 / scaleFactor);
                    zoomPdfOut();
                }
                return true; // 事件已处理
            }
        }
    }

    if (obj == ui->graphicsView->viewport()) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

            // 检查是否按下了Ctrl键
            if (wheelEvent->modifiers() & Qt::ControlModifier) {
                // 计算缩放因子
                double scaleFactor = 1.15;
                if (wheelEvent->angleDelta().y() > 0) {
                    // 向上滚动，放大
                    ui->graphicsView->scale(scaleFactor, scaleFactor);
                } else {
                    // 向下滚动，缩小
                    ui->graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
                }
                return true; // 事件已处理
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ScoreViewer::showImg(qint64 index){
    ui->graphicsView->setVisible(true);

    QGraphicsScene *scene = new QGraphicsScene(this);

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT f_path FROM File WHERE s_id=? AND f_index=? LIMIT 1");
    q.addBindValue(si.m_s_id);
    q.addBindValue(index);

    if (!q.exec()) {
        qDebug() << "Img:查询执行失败:" << q.lastError();
        qDebug()<<QString("SELECT f_path FROM File WHERE s_id=%1 AND f_index=%2 LIMIT 1").arg(si.m_s_id).arg(index);
        return;
    }
    if (q.next()) {
        m_f_path = q.value(0).toString();
        qDebug() << "Img:找到路径:" << m_f_path;
    } else {
        qDebug() << "Img:没有找到符合条件的记录";
        //m_f_path = QString();
    }
    m_f_path = q.value(0).toString();

    //qDebug()<<m_f_path;

    QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(QPixmap(m_f_path));
    scene->addItem(pixmapItem);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    ui->graphicsView->verticalScrollBar()->setValue(0);
    //ui->graphicsView->fitInView(pixmapItem->boundingRect(), Qt::KeepAspectRatio);
    ui->graphicsView->setInteractive(true);
    ui->graphicsView->centerOn(pixmapItem->boundingRect().center().x(), pixmapItem->boundingRect().top());
}

//PDF
void ScoreViewer::showPdf(){
    // 获取总页数
    m_doc->load(m_f_path);

    m_totalPages = m_doc->pageCount();
    m_currentPage = 0;  // 从第一页开始

    m_pdfView->setDocument(m_doc);
    //m_pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);缩放失效

    // 获取 verticalLayout_2
    QVBoxLayout *rightLayout = ui->verticalLayout_2;
    if (!rightLayout) {
        qDebug() << "错误：verticalLayout_2 为 nullptr";
        return;
    }

    // 查找 graphicsView 在 verticalLayout_2 中的索引
    int graphicsIndex = -1;
    for (int i = 0; i < rightLayout->count(); i++) {
        QLayoutItem *item = rightLayout->itemAt(i);
        if (item && item->widget() == ui->graphicsView) {
            graphicsIndex = i;
            break;
        }
    }

    if (graphicsIndex == -1) {
        qDebug() << "在 verticalLayout_2 中找不到 graphicsView";
        return;
    }

    qDebug() << "Pdf:找到 graphicsView 在 verticalLayout_2 中的索引:" << graphicsIndex;

    // 替换隐藏原有的 graphicsView
    ui->graphicsView->setVisible(false);

    rightLayout->insertWidget(graphicsIndex, m_pdfView);  // 添加到布局

    ui->horizontalLayout_main->setStretch(0, 0); // 左侧信息
    ui->horizontalLayout_main->setStretch(1, 1); // 右侧乐谱



    m_pdfView->viewport()->installEventFilter(this);
}

void ScoreViewer::zoomPdf(qreal factor)
{
    if (!m_pdfView || !m_doc) return;

    // 获取当前缩放因子
    qreal currentZoom = m_pdfView->zoomFactor();
    qreal newZoom = currentZoom * factor;

    // 设置缩放限制（可选）
    const qreal minZoom = 0.1;  // 10%
    const qreal maxZoom = 5.0;  // 500%

    if (newZoom < minZoom) {
        newZoom = minZoom;
    } else if (newZoom > maxZoom) {
        newZoom = maxZoom;
    }

    // 应用新的缩放
    m_pdfView->setZoomFactor(newZoom);

}

// 放大
void ScoreViewer::zoomPdfIn()
{
    //qDebug()<<"Pdf:ZoomIn";
    zoomPdf(1.2);  // 放大20%
}

// 缩小
void ScoreViewer::zoomPdfOut()
{
    //qDebug()<<"Pdf:ZoomOut";
    zoomPdf(1.0/1.2);  // 缩小到83%
}

//重置
void ScoreViewer::resetPdfTransform()
{
    if (!m_pdfView) return;

    // 重置缩放
    m_pdfView->setZoomFactor(1.0);  // 100% 原始大小

    // 重置位置：滚动到顶部
    QScrollBar *vScrollBar = m_pdfView->verticalScrollBar();
    QScrollBar *hScrollBar = m_pdfView->horizontalScrollBar();
    if (vScrollBar) vScrollBar->setValue(0);
    if (hScrollBar) hScrollBar->setValue(0);

    qDebug() << "Pdf:ResetZoom";
}


void ScoreViewer::addPdfShortcuts()
{
    if (!m_pdfView) return;

    // 放大快捷键 Ctrl++
    QShortcut *zoomInShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), this);
    connect(zoomInShortcut, &QShortcut::activated, this, [this]() {
        zoomPdfIn(); // 放大
    });

    // 缩小快捷键 Ctrl+-
    QShortcut *zoomOutShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this);
    connect(zoomOutShortcut, &QShortcut::activated, this, [this]() {
        zoomPdfOut(); // 缩小
    });

    //重置快捷键 Ctrl+0
    QShortcut *resetZoomShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this);

    connect(resetZoomShortcut, &QShortcut::activated, this, [this]() {
        resetPdfTransform();
    });
}

// 跳转到指定页面
void ScoreViewer::goToPage(int pageIndex)
{
    if (!m_pdfView || !m_doc) {
        return;
    }

    // 检查页面索引是否有效
    if (pageIndex < 0 || pageIndex >= m_totalPages) {
        qDebug() << "Page:无效的页面索引:" << pageIndex;
        return;
    }

    // 更新当前页码
    m_currentPage = pageIndex;

    // 使用 QPdfPageNavigator 跳转到指定页面
    QPdfPageNavigator *navigator = m_pdfView->pageNavigator();
    if (navigator) {
        //跳转到指定页面，位置(0,0)表示页面左上角
        navigator->jump(pageIndex, QPointF(0, 0), 0);
    } else {
        //如果 navigator 不存在，使用备用方法
        m_pdfView->pageNavigator()->jump(pageIndex, {}, 0);
    }

    qDebug() << "Page:跳转到第" << (pageIndex + 1) << "页";

    if(m_f_type=="img"){
        qDebug()<<"Img:Page->"<<pageIndex;
        showImg(pageIndex);
    }
}

// 上一页
void ScoreViewer::prevPage()
{
    if (m_currentPage > 0) {
        goToPage(m_currentPage - 1);
    } else {
        qDebug() << "Page:已经是第一页";
    }
}

// 下一页
void ScoreViewer::nextPage()
{
    if (m_currentPage < m_totalPages - 1) {
        goToPage(m_currentPage + 1);
    } else {
        qDebug() << "Page:已经是最后一页";
    }
}
