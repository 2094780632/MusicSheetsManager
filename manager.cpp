#include "manager.h"
#include "ui_manager.h"
#include "consql.h"
#include "importdialog.h"


Manager::Manager(QWidget *parent)
    : QTabWidget(parent)
    , ui(new Ui::Manager)
    , m_listModel(new QStandardItemModel(this))
    , m_file_list(new QStandardItemModel(this))
    , m_currentCategoryId(0)
{
    ui->setupUi(this);

    // 窗口设置
    setWindowFlag(Qt::Window, true);
    setWindowState(windowState() | Qt::WindowMaximized);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("乐谱管理");
    setCurrentIndex(0);

    // 布局设置
    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,2);
    ui->splitter_2->setStretchFactor(0,1);
    ui->splitter_2->setStretchFactor(1,2);

    // 调性选项设置
    QStringList keys = {
        // 大调
        "C", "G", "D", "A", "E", "B", "F♯", "C♯",
        "F", "B♭", "E♭", "A♭", "D♭", "G♭", "C♭",
        // 和声小调
        "Cm", "Gm", "Dm", "Am", "Em", "Bm", "F♯m", "C♯m",
        "Fm", "B♭m", "E♭m", "A♭m", "D♭m", "G♭m", "C♭m",
        // 其他
        "其他"
    };
    ui->s_key_comboBox->addItems(keys);

    // 列表视图设置
    ui->listView->setMovement(QListView::Static);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->category_listView->setMovement(QListView::Static);
    ui->category_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 文件列表设置
    ui->file_listView->setModel(m_file_list);
    ui->file_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 加载数据
    loadCategories();
    loadSongList();

    // 默认加载第一项
    if (m_listModel->rowCount() > 0) {
        QModelIndex firstIndex = m_listModel->index(0, 0);
        loadEditInfo(firstIndex);
        ui->listView->setCurrentIndex(firstIndex);
    }

    if (auto* categoryModel = qobject_cast<QStandardItemModel*>(ui->category_listView->model())) {
        if (categoryModel->rowCount() > 0) {
            QModelIndex firstCateIndex = categoryModel->index(0, 0);
            loadCateInfo(firstCateIndex);
            ui->category_listView->setCurrentIndex(firstCateIndex);
        }
    }

    // 连接信号槽
    connect(ui->listView, &QListView::clicked, this, &Manager::loadEditInfo);
    connect(ui->listView, &QListView::doubleClicked, this, &Manager::loadEditInfo);
    connect(ui->category_listView, &QListView::clicked, this, &Manager::loadCateInfo);
    connect(ui->category_listView, &QListView::doubleClicked, this, &Manager::loadCateInfo);
    connect(ui->file_listView, &QListView::doubleClicked, this, &Manager::onFileListDoubleClicked);
    connect(ui->s_type_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Manager::onTypeChanged);
}

Manager::~Manager()
{
    delete ui;
}

void Manager::loadSongList()
{
    m_listModel->clear();
    qDebug() << "Manager:loadSongList";

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id, s_name FROM Song ORDER BY s_id");

    if (!q.exec()) {
        qDebug() << "Manager: 查询失败:" << q.lastError().text();
        return;
    }

    while (q.next()) {
        QStandardItem *item = new QStandardItem();
        item->setText(q.value(1).toString());
        item->setData(q.value(0).toLongLong(), Qt::UserRole);
        m_listModel->appendRow(item);
    }
    ui->listView->setModel(m_listModel);
}

void Manager::loadCategories()
{
    ui->c_id_comboBox->clear();
    ui->c_id_comboBox->addItem("无", 0);

    auto *cate_list = new QStandardItemModel();
    QSqlQuery q(Consql::instance()->database());
    q.exec("SELECT c_id, c_name FROM Category ORDER BY c_name");

    while (q.next()) {
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui->c_id_comboBox->addItem(name, id);

        auto *item = new QStandardItem(name);
        item->setData(id, Qt::UserRole);
        cate_list->appendRow(item);
    }
    ui->category_listView->setModel(cate_list);
}

void Manager::loadEditInfo(const QModelIndex &index)
{
    if (!index.isValid()) return;

    qint64 songId = index.data(Qt::UserRole).toLongLong();
    QString songName = index.data(Qt::DisplayRole).toString();

    if (m_lastFilter.id == songId && m_lastFilter.name == songName) {
        qDebug() << "Manager: same filter, skip reload";
        return;
    }

    qDebug() << "Manager: Select Song -> ID:" << songId << " - " << songName;

    // 清空文件列表
    m_files.clear();
    m_file_list->clear();
    ui->fPathlineEdit->clear();

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT s_id, s_name, s_composer, s_key, s_version, s_type, c_id, s_remark "
              "FROM Song WHERE s_id = ? LIMIT 1");
    q.addBindValue(songId);

    if (!q.exec() || !q.next()) {
        qDebug() << "Manager: Song NOT FOUND";
        return;
    }

    // 读取歌曲信息
    si.m_s_id = q.value(0).toLongLong();
    si.m_s_name = q.value(1).toString();
    si.m_s_composer = q.value(2).toString();
    si.m_s_key = q.value(3).toString();
    si.m_s_version = q.value(4).toString();
    si.m_s_type = q.value(5).toString();
    si.m_c_id = q.value(6).toLongLong();
    si.m_s_remark = q.value(7).toString();

    // 显示歌曲信息
    ui->s_name_lineEdit->setText(si.m_s_name);
    ui->s_composer_lineEdit->setText(si.m_s_composer);

    int keyIndex = ui->s_key_comboBox->findText(si.m_s_key);
    ui->s_key_comboBox->setCurrentIndex(keyIndex != -1 ? keyIndex : 0);

    ui->s_version_lineEdit->setText(si.m_s_version);

    int typeIndex = ui->s_type_comboBox->findText(si.m_s_type);
    ui->s_type_comboBox->setCurrentIndex(typeIndex != -1 ? typeIndex : 0);

    // 设置分类
    if (si.m_c_id > 0) {
        q.prepare("SELECT c_name FROM Category WHERE c_id = ?");
        q.addBindValue(si.m_c_id);
        if (q.exec() && q.next()) {
            QString cname = q.value(0).toString();
            int cateIndex = ui->c_id_comboBox->findText(cname);
            ui->c_id_comboBox->setCurrentIndex(cateIndex != -1 ? cateIndex : 0);
        } else {
            ui->c_id_comboBox->setCurrentIndex(0);
        }
    } else {
        ui->c_id_comboBox->setCurrentIndex(0);
    }

    // 读取文件
    q.prepare("SELECT f_path FROM File WHERE s_id = ? ORDER BY f_index");
    q.addBindValue(si.m_s_id);

    if (q.exec()) {
        while (q.next()) {
            m_files.append(q.value(0).toString());
        }
        ui->fPathlineEdit->setText(m_files.join(";"));
    } else {
        ui->fPathlineEdit->setText("NOT FOUND");
    }

    fileListUpdate();
    ui->s_remark_textEdit->setText(si.m_s_remark);

    // 更新指纹
    m_lastFilter.id = songId;
    m_lastFilter.name = songName;
}

void Manager::fileListUpdate()
{
    m_file_list->clear();
    for (const QString &path : m_files) {
        auto *item = new QStandardItem(path);
        m_file_list->appendRow(item);
    }
    ui->file_listView->setModel(m_file_list);
}

void Manager::on_fPathpushButton_clicked()
{
    int idx = ui->s_type_comboBox->currentIndex(); // 0=PDF, 1=图片
    m_files.clear();

    if (idx == 0) { // PDF
        QString file = QFileDialog::getOpenFileName(
            this, "选择 PDF", ui->fPathlineEdit->text(), "PDF 文件 (*.pdf)");
        if (!file.isEmpty()) m_files << file;
    } else { // 图片
        m_files = QFileDialog::getOpenFileNames(
            this, "选择图片", ui->fPathlineEdit->text(),
            "图片 (*.png *.jpg *.jpeg *.bmp)");
    }

    if (!m_files.isEmpty()) {
        ui->fPathlineEdit->setText(m_files.join(";"));
        fileListUpdate();
    }
}

void Manager::onTypeChanged(int index)
{
    Q_UNUSED(index);
    m_file_list->clear();
    ui->fPathlineEdit->clear();
}

void Manager::onFileListDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int row = index.row();
    if (row >= 0 && row < m_files.size()) {
        m_file_list->removeRow(row);
        m_files.removeAt(row);
        ui->fPathlineEdit->setText(m_files.join(";"));
    }
}

void Manager::on_pushButton_savesong_clicked()
{
    qDebug() << "Manager: Save Song Edit";

    // 验证
    if (si.m_s_id <= 0) {
        QMessageBox::warning(this, "提示", "请先选择要保存的歌曲！");
        return;
    }

    if (m_files.isEmpty()) {
        QMessageBox::warning(this, "提示", "文件列表不能为空！");
        return;
    }

    Consql::ScoreMeta meta;
    meta.name = ui->s_name_lineEdit->text().trimmed();
    meta.composer = ui->s_composer_lineEdit->text().trimmed();
    meta.key = ui->s_key_comboBox->currentText();
    meta.categoryId = ui->c_id_comboBox->currentData().toInt();
    meta.version = ui->s_version_lineEdit->text().trimmed();
    meta.type = ui->s_type_comboBox->currentText();
    meta.remark = ui->s_remark_textEdit->toPlainText().trimmed();

    if (meta.name.isEmpty()) {
        QMessageBox::warning(this, "提示", "歌名不能为空！");
        return;
    }

    // 检查文件是否有变化
    QStringList oldFiles = Consql::instance()->getFilePathsBySongId(si.m_s_id);
    bool filesChanged = (oldFiles != m_files);

    // 执行更新
    bool success = false;
    if (filesChanged) {
        success = Consql::instance()->updateScoreWithFiles(si.m_s_id, meta, m_files);
    } else {
        // 只更新基本信息
        QSqlQuery q(Consql::instance()->database());
        q.prepare("UPDATE Song SET s_name=?, s_composer=?, s_key=?, "
                  "s_remark=?, s_version=?, s_type=?, c_id=? WHERE s_id=?");
        q.addBindValue(meta.name);
        q.addBindValue(meta.composer);
        q.addBindValue(meta.key);
        q.addBindValue(meta.remark);
        q.addBindValue(meta.version);
        q.addBindValue(meta.type);
        q.addBindValue(meta.categoryId == 0 ? QVariant() : QVariant(meta.categoryId));
        q.addBindValue(si.m_s_id);
        success = q.exec();
    }

    if (success) {
        QMessageBox::information(this, "修改成功", "已成功修改乐谱信息！");
        m_lastFilter.id = si.m_s_id;
        m_lastFilter.name = meta.name;
        loadSongList();
    } else {
        QMessageBox::critical(this, "保存失败", "数据库修改错误！");
    }
}

void Manager::on_pushButton_delsong_clicked()
{
    if (si.m_s_id <= 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的歌曲！");
        return;
    }

    QString songName = ui->s_name_lineEdit->text().trimmed();
    if (songName.isEmpty()) songName = QString("ID: %1").arg(si.m_s_id);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除歌曲《%1》吗？\n\n此操作将删除歌曲信息、文件记录和物理文件。\n此操作不可恢复！")
            .arg(songName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        qDebug() << "删除操作已取消";
        return;
    }

    qint64 deletingSongId = si.m_s_id;
    bool success = Consql::instance()->deleteSongWithFiles(deletingSongId);

    if (success) {
        QMessageBox::information(this, "删除成功", QString("已成功删除歌曲《%1》").arg(songName));

        clearEditFields();
        loadSongList();
        loadCategories();

        if (m_listModel->rowCount() > 0) {
            QModelIndex firstIndex = m_listModel->index(0, 0);
            loadEditInfo(firstIndex);
            ui->listView->setCurrentIndex(firstIndex);
        }
    } else {
        QMessageBox::critical(this, "删除失败", QString("删除歌曲《%1》失败！").arg(songName));
    }
}

void Manager::clearEditFields()
{
    // 清空歌曲编辑区域
    ui->s_name_lineEdit->clear();
    ui->s_composer_lineEdit->clear();
    ui->s_key_comboBox->setCurrentIndex(0);
    ui->s_version_lineEdit->clear();
    ui->s_type_comboBox->setCurrentIndex(0);
    ui->c_id_comboBox->setCurrentIndex(0);
    ui->s_remark_textEdit->clear();

    // 清空文件区域
    ui->fPathlineEdit->clear();
    m_files.clear();
    m_file_list->clear();

    // 重置结构体
    si.m_s_id = 0;
    si.m_s_name.clear();
    si.m_s_composer.clear();
    si.m_s_key.clear();
    si.m_s_remark.clear();
    si.m_s_version.clear();
    si.m_s_type.clear();
    si.m_c_id = 0;

    // 重置指纹
    m_lastFilter.id = 0;
    m_lastFilter.name.clear();

    // 清空选择
    ui->listView->clearSelection();
    ui->category_listView->clearSelection();

    // 清空分类编辑区域
    ui->catename_lineEdit->clear();
    ui->cateremark_textEdit->clear();
}

int Manager::on_pushButton_importSong_clicked(){
    ImportDialog i;
    if (i.exec() == QDialog::Accepted) {
        // 导入成功，刷新歌曲列表
        loadSongList();
        return 1;
    }
    return 0;
}



//TAB2 分类管理实现


void Manager::on_pushButton_newcat_clicked()
{
    qDebug() << "Manager: Creating new category";

    QSqlQuery q(Consql::instance()->database());

    // 生成默认分类名（避免重复）
    QString defaultName = "新分类";
    QString baseName = defaultName;
    int counter = 1;

    // 检查是否存在同名分类
    while (true) {
        q.prepare("SELECT COUNT(*) FROM Category WHERE c_name = ?");
        q.addBindValue(defaultName);
        if (q.exec() && q.next() && q.value(0).toInt() == 0) {
            break;  // 名称可用
        }
        defaultName = QString("%1%2").arg(baseName).arg(counter);
        counter++;

        //应该不会超过的吧
        if (counter > 65536) {
            QMessageBox::warning(this, "提示", "无法生成唯一分类名，请手动输入！");
            return;
        }
    }

    // 插入新的分类记录
    q.prepare("INSERT INTO Category (c_name, c_remark) VALUES (?, ?)");
    q.addBindValue(defaultName);
    q.addBindValue("");  // 空备注

    if (!q.exec()) {
        QMessageBox::critical(this, "创建失败",
                              QString("无法创建新分类：\n%1").arg(q.lastError().text()));
        qDebug() << "Failed to insert new category:" << q.lastError().text();
        return;
    }

    qint64 newId = q.lastInsertId().toLongLong();
    qDebug() << "Created new category, ID:" << newId << "Name:" << defaultName;

    // 刷新分类列表
    loadCategories();

    // 选中新创建的分类
    if (auto* categoryModel = qobject_cast<QStandardItemModel*>(ui->category_listView->model())) {
        for (int i = 0; i < categoryModel->rowCount(); ++i) {
            QStandardItem* item = categoryModel->item(i);
            if (item && item->data(Qt::UserRole).toInt() == newId) {
                QModelIndex newIndex = categoryModel->index(i, 0);
                ui->category_listView->setCurrentIndex(newIndex);
                loadCateInfo(newIndex);
                break;
            }
        }
    }

    // 清空编辑区域并设置默认名称
    ui->catename_lineEdit->setText(defaultName);
    ui->cateremark_textEdit->clear();
    m_currentCategoryId = newId;
    m_originalCateName = defaultName;

    QMessageBox::information(this, "创建成功",
                             QString("已创建新分类：%1\n\n你可以修改分类名称和备注。").arg(defaultName));
}

void Manager::on_pushButton_savecat_clicked()
{
    qDebug() << "Manager: Saving category changes";

    QString newName = ui->catename_lineEdit->text().trimmed();
    QString remark = ui->cateremark_textEdit->toPlainText().trimmed();

    // 验证输入
    if (newName.isEmpty()) {
        QMessageBox::warning(this, "提示", "分类名称不能为空！");
        return;
    }

    if (m_currentCategoryId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择要修改的分类！");
        return;
    }

    // 检查名称是否已更改
    if (newName == m_originalCateName) {
        // 名称未改变，直接更新
        QSqlQuery q(Consql::instance()->database());
        q.prepare("UPDATE Category SET c_remark = ? WHERE c_id = ?");
        q.addBindValue(remark);
        q.addBindValue(m_currentCategoryId);

        if (!q.exec()) {
            QMessageBox::critical(this, "保存失败",
                                  QString("无法保存分类信息：\n%1").arg(q.lastError().text()));
            return;
        }

        // 刷新列表
        loadCategories();
        QMessageBox::information(this, "保存成功", "分类备注已更新！");
        return;
    }

    // 名称已更改，检查是否重名
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT COUNT(*) FROM Category WHERE c_name = ? AND c_id != ?");
    q.addBindValue(newName);
    q.addBindValue(m_currentCategoryId);

    if (!q.exec() || !q.next()) {
        QMessageBox::critical(this, "错误", "数据库查询失败！");
        return;
    }

    if (q.value(0).toInt() > 0) {
        QMessageBox::warning(this, "重名错误",
                             QString("分类名称\"%1\"已存在，请使用其他名称！").arg(newName));
        // 恢复原名称
        ui->catename_lineEdit->setText(m_originalCateName);
        return;
    }

    // 更新分类信息
    q.prepare("UPDATE Category SET c_name = ?, c_remark = ? WHERE c_id = ?");
    q.addBindValue(newName);
    q.addBindValue(remark);
    q.addBindValue(m_currentCategoryId);

    if (!q.exec()) {
        QMessageBox::critical(this, "保存失败",
                              QString("无法保存分类信息：\n%1").arg(q.lastError().text()));
        return;
    }

    qDebug() << "Updated category ID:" << m_currentCategoryId
             << "New name:" << newName;

    // 更新原始名称记录
    m_originalCateName = newName;

    // 刷新分类列表
    loadCategories();

    // 刷新歌曲页面的分类下拉框
    loadCategories();  // 这会同时更新 ui->c_id_comboBox

    // 保持当前选中状态
    if (auto* categoryModel = qobject_cast<QStandardItemModel*>(ui->category_listView->model())) {
        for (int i = 0; i < categoryModel->rowCount(); ++i) {
            QStandardItem* item = categoryModel->item(i);
            if (item && item->data(Qt::UserRole).toInt() == m_currentCategoryId) {
                QModelIndex index = categoryModel->index(i, 0);
                ui->category_listView->setCurrentIndex(index);
                break;
            }
        }
    }

    QMessageBox::information(this, "保存成功", "分类信息已更新！");
}

void Manager::on_pushButton_delcat_clicked()
{
    if (m_currentCategoryId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的分类！");
        return;
    }

    QString categoryName = ui->catename_lineEdit->text().trimmed();
    if (categoryName.isEmpty()) {
        categoryName = QString("ID: %1").arg(m_currentCategoryId);
    }

    // 检查该分类下是否有歌曲
    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT COUNT(*) FROM Song WHERE c_id = ?");
    q.addBindValue(m_currentCategoryId);

    if (!q.exec() || !q.next()) {
        QMessageBox::critical(this, "错误", "无法检查分类使用情况！");
        return;
    }

    int songCount = q.value(0).toInt();

    QString warningText;
    if (songCount > 0) {
        warningText = QString("确定要删除分类《%1》吗？\n\n"
                              "该分类下有 %2 首歌曲，删除后这些歌曲的分类将被清空。\n"
                              "此操作不可恢复！")
                          .arg(categoryName).arg(songCount);
    } else {
        warningText = QString("确定要删除分类《%1》吗？\n\n"
                              "此操作不可恢复！")
                          .arg(categoryName);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", warningText,
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        qDebug() << "删除分类操作已取消";
        return;
    }

    // 执行删除
    q.prepare("DELETE FROM Category WHERE c_id = ?");
    q.addBindValue(m_currentCategoryId);

    if (!q.exec()) {
        QMessageBox::critical(this, "删除失败",
                              QString("无法删除分类：\n%1").arg(q.lastError().text()));
        return;
    }

    int rowsAffected = q.numRowsAffected();
    if (rowsAffected == 0) {
        QMessageBox::warning(this, "提示", "未找到指定的分类！");
        return;
    }

    qDebug() << "Deleted category ID:" << m_currentCategoryId
             << "Rows affected:" << rowsAffected;

    QMessageBox::information(this, "删除成功",
                             QString("已成功删除分类《%1》").arg(categoryName));

    // 清空编辑区域
    ui->catename_lineEdit->clear();
    ui->cateremark_textEdit->clear();
    m_currentCategoryId = 0;
    m_originalCateName.clear();

    // 刷新分类列表
    loadCategories();

    // 刷新歌曲页面的分类下拉框
    loadCategories();  // 这会同时更新 ui->c_id_comboBox

    // 如果有其他分类，选中第一个
    if (auto* categoryModel = qobject_cast<QStandardItemModel*>(ui->category_listView->model())) {
        if (categoryModel->rowCount() > 0) {
            QModelIndex firstIndex = categoryModel->index(0, 0);
            ui->category_listView->setCurrentIndex(firstIndex);
            loadCateInfo(firstIndex);
        }
    }
}

void Manager::loadCateInfo(const QModelIndex &index)
{
    if (!index.isValid()) {
        m_currentCategoryId = 0;
        m_originalCateName.clear();
        ui->catename_lineEdit->clear();
        ui->cateremark_textEdit->clear();
        return;
    }

    m_currentCategoryId = index.data(Qt::UserRole).toInt();

    QSqlQuery q(Consql::instance()->database());
    q.prepare("SELECT c_name, c_remark FROM Category WHERE c_id = ? LIMIT 1");
    q.addBindValue(m_currentCategoryId);

    if (q.exec() && q.next()) {
        QString cateName = q.value(0).toString();
        QString remark = q.value(1).toString();

        ui->catename_lineEdit->setText(cateName);
        ui->cateremark_textEdit->setText(remark);

        m_originalCateName = cateName;

        qDebug() << "Manager: Select Category -> ID:" << m_currentCategoryId
                 << " - " << cateName;
    } else {
        m_currentCategoryId = 0;
        m_originalCateName.clear();
        ui->catename_lineEdit->clear();
        ui->cateremark_textEdit->clear();

        qDebug() << "Manager: Category NOT FOUND";
    }
}
