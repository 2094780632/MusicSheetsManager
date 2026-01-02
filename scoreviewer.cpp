#include "scoreviewer.h"
#include "ui_scoreviewer.h"
#include "consql.h"
#include "metronome.h"
#include <QDebug>
#include <QSqlQuery>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QShortcut>
#include <QScrollBar>

ScoreViewer::ScoreViewer(qint64 songId, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScoreViewer)
{
    ui->setupUi(this);

    // 为 graphicsView 的视口安装事件过滤器
    ui->graphicsView->viewport()->installEventFilter(this);

    // 窗口默认最大化
    setWindowFlag(Qt::Window, true);
    setWindowState(windowState() | Qt::WindowMaximized);
    setAttribute(Qt::WA_DeleteOnClose);

    // 加载并显示信息
    loadSongInfo(songId);
    setWindowTitle(si.m_s_name);
    ui->name->setText(si.m_s_name);
    ui->composer->setText(si.m_s_composer);
    ui->key->setText(si.m_s_key);
    ui->addDate->setText(si.m_s_addDate);
    ui->version->setText(si.m_s_version);
    ui->remark->setText(si.m_s_remark);

    if (si.m_c_id > 0) {
        QSqlQuery q(Consql::instance()->database());
        q.prepare("SELECT c_id, c_name FROM Category WHERE c_id=?");
        q.addBindValue(si.m_c_id);
        if (!q.exec() || !q.next()) {
            qDebug() << "ScoreViewer: category not found";
            ui->category->setText("无");
        } else {
            QString cname = q.value(1).toString();
            ui->category->setText(cname);
        }
    } else {
        ui->category->setText("无");
    }

    // 显示乐谱
    loadScore(songId);

    addPdfShortcuts();

    // 翻页按钮
    connect(ui->btnPrevPage, &QPushButton::clicked, this, &ScoreViewer::prevPage);
    connect(ui->btnNextPage, &QPushButton::clicked, this, &ScoreViewer::nextPage);
    ui->btnPrevPage->setShortcut(QKeySequence(Qt::Key_Left));
    ui->btnNextPage->setShortcut(QKeySequence(Qt::Key_Right));

    // 节拍器
    connect(ui->btnPlayCfg, &QPushButton::clicked, this, [this] {
        Metronome m;
        m.exec();
    });

    qDebug() << "ScoreViewer: 构造函数，歌曲ID:" << songId;
}

ScoreViewer::~ScoreViewer()
{
    qDebug() << "ScoreViewer: 析构函数";
    delete ui;
}

void ScoreViewer::loadSongInfo(qint64 id)
{
    qDebug() << "ScoreViewer: 加载歌曲信息，ID:" << id;

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id, s_name, s_composer, s_key, s_addDate, s_isFav, s_remark, s_version, s_type, c_id FROM Song WHERE s_id=? LIMIT 1");
    q.addBindValue(id);

    if (!q.exec() || !q.next()) {
        qDebug() << "ScoreViewer: Song NOT FOUND";
        return;
    }

    si.m_s_id = q.value(0).toInt();
    si.m_s_name = q.value(1).toString();
    si.m_s_composer = q.value(2).toString();
    si.m_s_key = q.value(3).toString();
    si.m_s_addDate = q.value(4).toString();
    si.m_s_isFav = q.value(5).toBool();
    si.m_s_remark = q.value(6).toString();
    si.m_s_version = q.value(7).toString();
    si.m_s_type = q.value(8).toString();
    si.m_c_id = q.value(9).toInt();

    qDebug() << "ScoreViewer: 歌曲信息加载完成 - 名称:" << si.m_s_name
             << "作曲家:" << si.m_s_composer << "类型:" << si.m_s_type;
}

void ScoreViewer::loadScore(qint64 id)
{
    qDebug() << "ScoreViewer: 加载乐谱文件，歌曲ID:" << id;

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT f_path, f_type, (SELECT COUNT(*) FROM File WHERE s_id = ?) FROM File WHERE s_id=? ORDER BY f_index LIMIT 1");
    q.addBindValue(id);
    q.addBindValue(id);

    if (!q.exec() || !q.next()) {
        qDebug() << "ScoreViewer: NO RELAT FILE";
        return;
    } else {
        m_f_path = q.value(0).toString();
        m_f_type = q.value(1).toString();
        m_totalPages = q.value(2).toInt();

        qDebug() << "ScoreViewer: 文件信息 - 路径:" << m_f_path
                 << "类型:" << m_f_type << "总页数:" << m_totalPages;

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

            // 检查是否按下了 Ctrl 键
            if (wheelEvent->modifiers() & Qt::ControlModifier) {
                // 计算缩放因子
                qreal scaleFactor = 1.1; // 10%
                if (wheelEvent->angleDelta().y() > 0) {
                    // 向上滚动，放大
                    zoomPdfIn();
                } else {
                    // 向下滚动，缩小
                    zoomPdfOut();
                }
                return true; // 事件已处理
            }
        }
    }

    if (obj == ui->graphicsView->viewport()) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

            // 检查是否按下了 Ctrl 键
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

void ScoreViewer::showImg(qint64 index)
{
    qDebug() << "ScoreViewer: 显示图片，索引:" << index;

    if (m_pdfView) {
        m_pdfView->setVisible(false);
        // 为了确保完全隐藏，可以设置最小尺寸
        m_pdfView->setMinimumSize(0, 0);
        m_pdfView->setMaximumSize(0, 0);
    }

    ui->graphicsView->setVisible(true);

    QGraphicsScene *scene = new QGraphicsScene(this);

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT f_path FROM File WHERE s_id=? AND f_index=? LIMIT 1");
    q.addBindValue(si.m_s_id);
    q.addBindValue(index);

    if (!q.exec()) {
        qDebug() << "ScoreViewer: Img: 查询执行失败:" << q.lastError();
        return;
    }

    if (q.next()) {
        m_f_path = q.value(0).toString();
        qDebug() << "ScoreViewer: Img: 找到路径:" << m_f_path;
    } else {
        qDebug() << "ScoreViewer: Img: 没有找到符合条件的记录";
        return;
    }

    QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(QPixmap(m_f_path));
    scene->addItem(pixmapItem);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    ui->graphicsView->verticalScrollBar()->setValue(0);
    ui->graphicsView->setInteractive(true);
    ui->graphicsView->centerOn(pixmapItem->boundingRect().center().x(), pixmapItem->boundingRect().top());

    qDebug() << "ScoreViewer: 图片显示完成";
}

// PDF
void ScoreViewer::showPdf()
{
    qDebug() << "ScoreViewer: 显示PDF文件";

    // 获取总页数
    m_doc->load(m_f_path);
    m_totalPages = m_doc->pageCount();
    m_currentPage = 0;  // 从第一页开始

    m_pdfView->setDocument(m_doc);

    // 获取 verticalLayout_2
    QVBoxLayout *rightLayout = ui->verticalLayout_2;
    if (!rightLayout) {
        qDebug() << "ScoreViewer: Pdf: 错误, verticalLayout_2 为 nullptr";
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
        qDebug() << "ScoreViewer: Pdf: 在 verticalLayout_2 中找不到 graphicsView";
        return;
    }

    qDebug() << "ScoreViewer: Pdf: 找到 graphicsView 在 verticalLayout_2 中的索引:" << graphicsIndex;

    // 替换隐藏原有的 graphicsView
    ui->graphicsView->setVisible(false);
    rightLayout->insertWidget(graphicsIndex, m_pdfView);  // 添加到布局

    ui->horizontalLayout_main->setStretch(0, 0); // 左侧信息
    ui->horizontalLayout_main->setStretch(1, 1); // 右侧乐谱

    m_pdfView->viewport()->installEventFilter(this);

    qDebug() << "ScoreViewer: PDF 显示完成，总页数:" << m_totalPages;
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

    qDebug() << "ScoreViewer: Pdf: 缩放，当前缩放因子:" << newZoom;
}

// 放大
void ScoreViewer::zoomPdfIn()
{
    qDebug() << "ScoreViewer: Pdf: ZoomIn";
    zoomPdf(1.2);  // 放大20%
}

// 缩小
void ScoreViewer::zoomPdfOut()
{
    qDebug() << "ScoreViewer: Pdf: ZoomOut";
    zoomPdf(1.0 / 1.2);  // 缩小到83%
}

// 重置
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

    qDebug() << "ScoreViewer: Pdf: ResetZoom";
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

    // 重置快捷键 Ctrl+0
    QShortcut *resetZoomShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this);
    connect(resetZoomShortcut, &QShortcut::activated, this, [this]() {
        resetPdfTransform();
    });

    qDebug() << "ScoreViewer: PDF 快捷键已添加";
}

// 跳转到指定页面
void ScoreViewer::goToPage(int pageIndex)
{
    if (!m_pdfView || !m_doc) {
        return;
    }

    // 检查页面索引是否有效
    if (pageIndex < 0 || pageIndex >= m_totalPages) {
        qDebug() << "ScoreViewer: Page: 无效的页面索引:" << pageIndex;
        return;
    }

    // 更新当前页码
    m_currentPage = pageIndex;

    // 使用 QPdfPageNavigator 跳转到指定页面
    QPdfPageNavigator *navigator = m_pdfView->pageNavigator();
    if (navigator) {
        // 跳转到指定页面，位置(0,0)表示页面左上角
        navigator->jump(pageIndex, QPointF(0, 0), 0);
    } else {
        // 如果 navigator 不存在，使用备用方法
        m_pdfView->pageNavigator()->jump(pageIndex, {}, 0);
    }

    qDebug() << "ScoreViewer: Page: 跳转到第" << (pageIndex + 1) << "页";

    if (m_f_type == "img") {
        qDebug() << "ScoreViewer: Img: Page->" << pageIndex;
        showImg(pageIndex);
    }
}

// 上一页
void ScoreViewer::prevPage()
{
    if (m_currentPage > 0) {
        goToPage(m_currentPage - 1);
    } else {
        qDebug() << "ScoreViewer: Page: 已经是第一页";
    }
}

// 下一页
void ScoreViewer::nextPage()
{
    if (m_currentPage < m_totalPages - 1) {
        goToPage(m_currentPage + 1);
    } else {
        qDebug() << "ScoreViewer: Page: 已经是最后一页";
    }
}
