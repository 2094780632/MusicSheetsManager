#include "manager.h"
#include "ui_manager.h"
#include "consql.h"

Manager::Manager(QWidget *parent)
    : QTabWidget(parent)
    , ui(new Ui::Manager)
    , m_listModel(new QStandardItemModel(this))
    , m_file_list(new QStandardItemModel(this))
{
    ui->setupUi(this);

    /*
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint |
                   Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint |
                   Qt::WindowCloseButtonHint);
    */
    //窗口 默认最大化
    setWindowFlag(Qt::Window, true);
    setWindowState(windowState() | Qt::WindowMaximized);
    setAttribute(Qt::WA_DeleteOnClose);

    // 设置窗口标题
    setWindowTitle("乐谱管理");

    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,2);
    ui->splitter_2->setStretchFactor(0,1);
    ui->splitter_2->setStretchFactor(1,2);

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

    ui->listView->setMovement(QListView::Static);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->category_listView->setMovement(QListView::Static);
    ui->category_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //文件列表初始化
    ui->file_listView->setModel(m_file_list);  // 设置文件列表模型
    ui->file_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    loadCategories();
    loadSongList();

    //加载详细信息
    connect(ui->listView, &QListView::clicked,
            this, &Manager::loadEditInfo);
    connect(ui->listView, &QListView::doubleClicked,
            this, &Manager::loadEditInfo);
    connect(ui->category_listView, &QListView::clicked,
            this, &Manager::loadCateInfo);
    connect(ui->category_listView, &QListView::doubleClicked,
            this, &Manager::loadCateInfo);

    // 连接类型切换信号，切换时清空文件列表
    connect(ui->s_type_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Manager::onTypeChanged);
}

Manager::~Manager()
{
    delete ui;
}

void Manager::loadSongList(){
    m_listModel->clear();
    qDebug()<<"Manager:loadSongList";

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id, s_name FROM Song ORDER BY s_id");

    if (!q.exec()) {
        qDebug() << "Manager:   查询失败:" << q.lastError().text();
        return;
    }

    while(q.next()){
        si.m_s_id=q.value(0).toInt();
        si.m_s_name=q.value(1).toString();
        /*
        si.m_s_composer=q.value(2).toString();
        si.m_s_key=q.value(3).toString();
        //si.m_s_addDate=q.value(4).toString();
        //si.m_s_isFav=q.value(5).toBool();
        si.m_s_remark=q.value(7).toString();
        si.m_s_version=q.value(4).toString();
        si.m_s_type=q.value(5).toString();
        si.m_c_id=q.value(6).toInt();
        */

        QStandardItem *item = new QStandardItem();
        item->setText(si.m_s_name);
        item->setData(si.m_s_id, Qt::UserRole);
        qDebug()<<"Manager:loadSongList->"<<si.m_s_name;
        m_listModel->appendRow(item);
    }
    ui->listView->setModel(m_listModel);
}

//分类读取
void Manager::loadCategories()
{
    //TAB-1
    ui->c_id_comboBox->clear();
    // 第 0 项：「无」→ id = 0
    ui->c_id_comboBox->addItem("无", 0);

    QStandardItemModel *cate_list = new QStandardItemModel();

    QSqlQuery q(Consql::instance()->database());
    q.exec("SELECT c_id, c_name FROM Category ORDER BY c_name");
    while (q.next()) {
        int  id   = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui->c_id_comboBox->addItem(name, id);   // 用户数据 = 真实主键

        QStandardItem *i = new QStandardItem();
        i->setText(name);
        i->setData(id,Qt::UserRole);
        cate_list->appendRow(i);
    }

    ui->category_listView->setModel(cate_list);
}

void Manager::loadEditInfo(const QModelIndex &index){
    if (!index.isValid()) return;

    qint64 songId = index.data(Qt::UserRole).toLongLong();
    QString songName = index.data(Qt::DisplayRole).toString();

    if (m_lastFilter.id == songId && m_lastFilter.name == songName) {
        qDebug() << "Manager:same filter, skip reload";
        return;
    }

    qDebug() << "Manager:Select Song -> ID:" << songId <<" - " << songName;


    // 清空上一次的文件列表
    m_files.clear();  // 清空 QStringList
    m_file_list->clear();  // 清空模型
    ui->fPathlineEdit->clear();

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id, s_name, s_composer, s_key, s_version, s_type, c_id, s_remark FROM Song WHERE s_id = ? ORDER BY s_id LIMIT 1");
    q.addBindValue(songId);
    if (!q.exec() || !q.next()) {
        qDebug()<<"Manager:Song NOT FOUND";
        return;
    }

    //读取
    si.m_s_id=q.value(0).toInt();
    si.m_s_name=q.value(1).toString();
    si.m_s_composer=q.value(2).toString();
    si.m_s_key=q.value(3).toString();
    si.m_s_remark=q.value(7).toString();
    si.m_s_version=q.value(4).toString();
    si.m_s_type=q.value(5).toString();
    si.m_c_id=q.value(6).toInt();

    //显示
    ui->s_name_lineEdit->setText(si.m_s_name);
    ui->s_composer_lineEdit->setText(si.m_s_composer);
    int keyIndex = ui->s_key_comboBox->findText(si.m_s_key);
    if(keyIndex != -1){
        ui->s_key_comboBox->setCurrentIndex(keyIndex);
    }else{
        qDebug()<<"Manager:key not found";
    }
    ui->s_version_lineEdit->setText(si.m_s_version);
    int typeIndex = ui->s_type_comboBox->findText(si.m_s_type);
    if(typeIndex != -1){
        ui->s_type_comboBox->setCurrentIndex(typeIndex);
    }else{
        qDebug()<<"Manager:type not found";
    }
    //qDebug()<<si.m_c_id;
    if(si.m_c_id>0){
        q.prepare("SELECT c_id, c_name FROM Category WHERE c_id=?");
        q.addBindValue(si.m_c_id);
        if (!q.exec() || !q.next()) {
            qDebug()<<"Manager:category not found";
            ui->c_id_comboBox->setCurrentIndex(0);
        }else{
            QString cname=q.value(1).toString();
            int cateIndex = ui->c_id_comboBox->findText(cname);
            if(cateIndex != -1){
                ui->c_id_comboBox->setCurrentIndex(cateIndex);
            }else{
                qDebug()<<"Manager:cateindex not found";
            }
        }
    }else{
        ui->c_id_comboBox->setCurrentIndex(0);
    }

    //文件读取

    q.prepare("SELECT f_path FROM File WHERE s_id = ? ORDER BY f_index");
    q.addBindValue(si.m_s_id);

    if(!q.exec()){
        qDebug()<<"Manager:filepath not found";
        ui->fPathlineEdit->setText("NOT FOUND");
    }
    while(q.next()){
        m_files.append(q.value(0).toString());
    }
    ui->fPathlineEdit->setText(m_files.join(";"));

    //QStandardItemModel *file_list = new QStandardItemModel(ui->file_listView);
    fileListUpdate();

    ui->s_remark_textEdit->setText(si.m_s_remark);

    //指纹
    m_lastFilter.id = songId;
    m_lastFilter.name = songName;

}

//同步list与model的更新
void Manager::fileListUpdate(){
    m_file_list->clear();
    for(const QString &i : m_files){
        QStandardItem *item = new QStandardItem(i);
        m_file_list->appendRow(item);
    }
    ui->file_listView->setModel(m_file_list);
}

//当文件类型改变时清空文件列表
void Manager::onTypeChanged(int index)
{
    Q_UNUSED(index); // 不使用index参数，但保持函数签名一致
    // 清空文件列表模型
    m_file_list->clear();
    ui->file_listView->setModel(m_file_list);
    // 清空路径显示
    ui->fPathlineEdit->clear();
}

//双击删除
void Manager::onFileListDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int row = index.row();

    // 从模型中删除
    m_file_list->removeRow(row);

    // 从内存列表中删除
    if (row >= 0 && row < m_files.size()) {
        m_files.removeAt(row);
    }

    // 更新 LineEdit
    ui->fPathlineEdit->setText(m_files.join(";"));
}

//加载分类详细信息
void Manager::loadCateInfo(const QModelIndex &index){
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT c_name, c_remark FROM Category WHERE c_id = ? LIMIT 1;");
    q.addBindValue(index.data(Qt::UserRole).toInt());
    if (!q.exec() || !q.next()) {
        qDebug()<<"Manager:Category NOT FOUND->"<<index.data(Qt::UserRole).toInt();
        return;
    }else{
        ui->catename_lineEdit->setText(q.value(0).toString());
        ui->cateremark_textEdit->setText(q.value(1).toString());
    }
}
