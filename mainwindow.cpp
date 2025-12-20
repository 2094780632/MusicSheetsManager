#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "consql.h"
#include "config.h"
#include "importdialog.h"
#include "scoreviewer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //窗体初始化 ui 里写了
    //setWindowTitle("Music Sheets Manager");

    //spliter初始宽度设置
    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,5);

    //乐谱ListView
    m_scoreModel = new QStandardItemModel(this);
    ui->listView->setModel(m_scoreModel);
    ui->listView->setViewMode(QListView::IconMode);
    ui->listView->setIconSize(QSize(120, 160));
    ui->listView->setGridSize(QSize(140, 200));
    ui->listView->setSpacing(10);
    ui->listView->setResizeMode(QListView::Adjust);
    ui->listView->setMovement(QListView::Static);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //treeView初始化
    //只读
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_treeModel = new QStandardItemModel(this);
    m_treeModel->setHorizontalHeaderLabels(QStringList() << "分组" << "数量");
    ui->treeView->setModel(m_treeModel);
    ui->treeView->setRootIsDecorated(true);

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                rebuildTree(static_cast<Dim>(idx));
            });



    rebuildTree(ByAll);
    refreshScoreGrid();

    //连接
    //导入乐谱
    connect(ui->action_O,&QAction::triggered,this,&MainWindow::importSheet);

    //ListView双击乐谱
    connect(ui->listView, &QListView::doubleClicked,
            this, [this](const QModelIndex &idx){
                if (!idx.isValid()) return;
                qint64 id = idx.data(Qt::UserRole).toLongLong();
                qDebug() << "MainWindow:打开乐谱" << id;
                //打开
                auto *viewer = new ScoreViewer(id, this); // this = 主窗口，做父对象方便生命周期管理
                viewer->show();
            });

    //listView菜单
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
    // 连接信号
    connect(ui->listView, &QListView::customContextMenuRequested,
            this, &MainWindow::onListViewCustomMenu);

    //帮助页面
    connect(ui->action_about,&QAction::triggered,this,&MainWindow::helpPage);
    //用户手册
    connect(ui->action_U,&QAction::triggered,this,&MainWindow::userManual);
    //重置初次启动标识
    connect(ui->action_R,&QAction::triggered,this,&MainWindow::deleteFirstStartSign);
    //清空数据库
    connect(ui->action_deldata,&QAction::triggered,this,&MainWindow::clearData);

    //退出应用
    connect(ui->action_X, &QAction::triggered, qApp, &QCoreApplication::quit);

    //快捷键刷新
    auto *refreshAct = new QAction("刷新乐谱", this);
    refreshAct->setShortcut(QKeySequence("Ctrl+R"));
    connect(refreshAct, &QAction::triggered,
            this, [this]{
                refreshScoreGrid();   // 默认参数 = 全部
            });
    addAction(refreshAct);

    //treeView点击
    connect(ui->treeView, &QTreeView::clicked,
            this, [this](const QModelIndex &idx){
                if (!idx.isValid()) return;

                Dim dim  = static_cast<Dim>(ui->comboBox->currentIndex());
                QString val;

                // 1. 全部维度 → 固定值
                if (dim == ByAll) {
                    val = "-1";
                }
                // 2. 其他维度 → 一律用「分组名」过滤
                else {
                    // 先找到这一行的「根节点文字」
                    QModelIndex rootIdx = idx;
                    while (rootIdx.parent().isValid())
                        rootIdx = rootIdx.parent();
                    val = rootIdx.data(Qt::DisplayRole).toString();
                }
                qDebug()<<"treeView:click->"<<val;
                refreshScoreGrid(dim, val);
            });

}

MainWindow::~MainWindow()
{
    delete ui;
}

//导入乐谱
int MainWindow::importSheet(){
    ImportDialog i;
    if (i.exec() == QDialog::Accepted) {
        //立刻重拉数据
        rebuildTree(ByAll);
        refreshScoreGrid();
    }
    return 1;
}

//帮助页面
void MainWindow::helpPage(){
    QMessageBox::information(this,
                             "帮助",
                             "Music Sheets Manager ver 1.0");
    qDebug("MainWindow:helpPage");
}

//用户手册
void MainWindow::userManual(){
    QMessageBox::information(this,
                             "占位符",
                             "Sth Here");
    qDebug("MainWindow:userManual");
}

//重置初次启动标识
void MainWindow::deleteFirstStartSign(){
    QSettings s("2024413670", "MusicSheetsManager");
    s.clear();
    QMessageBox::information(this,
                             "成功！",
                             "已清除注册表！");
    qDebug("MainWindow:deleteFirstStartSign");
}

//清空数据库
void MainWindow::clearData(){

    int ret = QMessageBox::question(this, "确认清空",
                                    "将删除全部乐谱文件与数据库记录，\n"
                                    "此操作不可恢复！继续吗？",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    Consql::instance()->closeDb();

    //清数据库
    if (!QFile::remove(AppConfig::instance()->getStoragePath() + "/data.db")) {
        QMessageBox::warning(this, "提示", "数据库文件被占用，无法删除！");
        return;
    }

    QString repo = AppConfig::instance()->getStoragePath();
    QDir dir(repo);
    if (dir.exists()) {
        //逐层删
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &info : list) {
            if (info.isDir()) {
                QDir sub(info.absoluteFilePath());
                sub.removeRecursively(); // Qt 5.10+
            } else {
                QFile::remove(info.absoluteFilePath());
            }
        }
        //最后把根目录本身也删掉
        if (!dir.rmdir(repo)) {
            QMessageBox::warning(this, "提示", "部分文件被占用，目录未完全清空！");
        }
    }

    //重建空目录
    if (!QDir().mkpath(repo)) {
        QMessageBox::critical(this, "错误", "无法重新创建仓库目录！");
        return;
    }

    //重建数据库
    QString dbPath = QDir(repo).absoluteFilePath("data.db");
    if (!Consql::instance()->openDb(dbPath) || !Consql::instance()->initDb()) {
        QMessageBox::critical(this, "错误", "数据库重建失败！");
        return;
    }

    QMessageBox::information(this, "完成", "数据库与本地文件已全部清空！");
    qDebug("clearData");
}

//刷新listView
void MainWindow::refreshScoreGrid(Dim dim, const QString &value)
{
    if (m_lastFilter.dim == dim && m_lastFilter.value == value) {
        qDebug() << "treeView:same filter, skip tree rebuild";
        return;
    }

    m_scoreModel->clear();

    QSqlQuery q(Consql::instance()->database());

    switch (dim) {
    case ByAll:          // 默认值
        q.exec("SELECT s_id, s_name, s_composer, s_key, s_addDate FROM Song ORDER BY s_addDate DESC");
        break;
    case ByCategory:
        if (value.isEmpty() || value == "-1")
            q.exec("SELECT s_id, s_name, s_composer, s_key, s_addDate FROM Song ORDER BY s_addDate DESC");
        else
            if(value == "未分类"){
            q.exec(QString("SELECT s_id, s_name, s_composer, s_key, s_addDate, c_id "
                           "FROM Song "
                           "WHERE c_id IS NULL "
                           "ORDER BY s_addDate DESC"));
        }else{
            q.exec(QString("SELECT s.s_id, s.s_name, s.s_composer, s.s_key, s.s_addDate, s.c_id "
                           "FROM Song s "
                           "INNER JOIN Category c ON s.c_id = c.c_id "
                           "WHERE c.c_name = '%1' "
                           "ORDER BY s.s_addDate DESC").arg(value));
        }

        break;
    case ByComposer:
        q.prepare("SELECT s_id, s_name, s_composer, s_key, s_addDate FROM Song WHERE s_composer = ? ORDER BY s_addDate DESC");
        q.addBindValue(value);
        q.exec();
        break;
    case ByKey:
        q.prepare("SELECT s_id, s_name, s_composer, s_key, s_addDate FROM Song WHERE s_key = ? ORDER BY s_addDate DESC");
        q.addBindValue(value);
        q.exec();
        break;
    case ByName:
        q.prepare("SELECT s_id, s_name, s_composer, s_key, s_addDate FROM Song WHERE s_name LIKE ? ORDER BY s_addDate DESC");
        q.addBindValue(value + '%');
        q.exec();
        break;
    default:
        qDebug("listView:default case");
        break;
    }


    while (q.next()) {
        qint64 id = q.value(0).toLongLong();
        QString name = q.value(1).toString();
        QString composer = q.value(2).toString();
        QString key = q.value(3).toString();
        QString date = q.value(4).toString();

        //拿第一张文件当封面
        QSqlQuery picQ(Consql::instance()->database());
        picQ.prepare("SELECT f_path, f_type FROM File WHERE s_id=? LIMIT 1");
        picQ.addBindValue(id);
        QPixmap pm;
        if (picQ.exec() && picQ.next()) {
            QString fPath = picQ.value(0).toString();
            QString fType = picQ.value(1).toString(); // "pdf" / "img"

            if (fType == "pdf") {
                //官方 QtPDF 生成第一页缩略图
                QPdfDocument doc;
                doc.load(fPath);
                QImage img = doc.render(0, QSize(120, 160)); // 页号 0
                pm = QPixmap::fromImage(img);
            } else {
                //普通图片直接读
                pm = QPixmap(fPath);
            }
        }
        if (pm.isNull())
            //pm = QPixmap(":/no_cover.png");   // 缺省图

        pm = pm.scaled(120, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QStandardItem *item = new QStandardItem(QIcon(pm), name);
        item->setData(id, Qt::UserRole);
        QString tooltipText = QString("%1 by %2 , %3 调, 于 %4 导入").arg(name)
                                  .arg(composer)
                                  .arg(key)
                                  .arg(date);
        item->setData(tooltipText, Qt::ToolTipRole);

        m_scoreModel->appendRow(item);
    }

    //指纹
    m_lastFilter.dim  = dim;
    m_lastFilter.value = value;

    qDebug("treeView:refresh");
}

//listView右键菜单
void MainWindow::onListViewCustomMenu(const QPoint &pos)
{
    QModelIndex index = ui->listView->indexAt(pos);
    if (!index.isValid()) return;          // 空白处右键直接忽略

    qint64 songId = index.data(Qt::UserRole).toLongLong();
    QString songName = index.data(Qt::DisplayRole).toString();

    QMenu menu(this);
    QAction *openAct  = menu.addAction("打开");
    QAction *editAct  = menu.addAction("编辑");
    QAction *infoAct  = menu.addAction("详情");
    QAction *delAct   = menu.addAction("删除");

    QAction *selected = menu.exec(ui->listView->viewport()->mapToGlobal(pos));
    if (!selected) return;

    if (selected == openAct) {
        qDebug() << "打开" << songId;
        // TODO: 打开 PDF / 图片
    } else if (selected == editAct) {
        qDebug() << "编辑" << songId;
        // TODO: 弹编辑对话框
    } else if (selected == infoAct) {
        qDebug() << "详情" << songId;

    } else if (selected == delAct) {
        qDebug() << "删除" << songId;
        // TODO: 二次确认后调数据库删除 + 本地文件
    }
}

//刷新treeView
void MainWindow::rebuildTree(Dim dim)
{

    m_treeModel->clear();
    m_treeModel->setHorizontalHeaderLabels(QStringList() << "分组");
    ui->treeView->setRootIsDecorated(true);

    /* 统一查询：按维度排序，避免多次 SQL */
    QSqlQuery q(Consql::instance()->database());
    switch (dim) {
    case ByAll:
        q.exec("SELECT s_id, s_name FROM Song ORDER BY s_addDate DESC");
        break;
    case ByName:
        q.exec("SELECT s_id, s_name FROM Song ORDER BY s_name");
        break;
    case ByComposer:
        q.exec("SELECT s_id, s_name, s_composer FROM Song ORDER BY s_composer");
        break;
    case ByKey:
        q.exec("SELECT s_id, s_name, s_key FROM Song ORDER BY s_key");
        break;
    case ByCategory:
        q.exec("SELECT s.s_id, s.s_name, COALESCE(c.c_name,'未分类'), COALESCE(c.c_id,-1) "
               "FROM Song s LEFT JOIN Category c ON s.c_id = c.c_id "
               "ORDER BY c.c_name, s.s_name");
        break;
    }

    /* 内存分组：key → [(id, name), ...] */
    QMap<QString, QList<QPair<qint64, QString>>> groups;

    while (q.next()) {
        qint64  id   = q.value(0).toLongLong();
        QString name = q.value(1).toString();
        QString key;                    // 分组依据
        switch (dim) {
        case ByAll:
            key = "全部";               // 显示文字
            break;
        case ByName:
            key = name.isEmpty() ? QString("?") : name.left(1);   // 只取首字符
            break;
        case ByComposer:
            key = q.value(2).toString();
            if (key.isEmpty()) key = "未知";
            break;
        case ByKey:
            key = q.value(2).toString();
            if (key.isEmpty()) key = "未知";
            break;
        case ByCategory:
            key = q.value(2).toString();   // 已经 COALESCE
            break;         // 统一 (qint64, QString)
        }
        groups[key].append(qMakePair(id, name));
    }

    /* 填树 */
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QString &groupName = it.key();
        QStandardItem *rootItem = new QStandardItem(groupName);
        rootItem->setData((dim == ByAll) ? QVariant(-1) : QVariant(groupName), Qt::UserRole);
        m_treeModel->appendRow(rootItem);

        for (const auto &pair : it.value()) {
            QStandardItem *child = new QStandardItem(pair.second);
            child->setData(pair.first, Qt::UserRole);
            rootItem->appendRow(child);
        }
    }
}
