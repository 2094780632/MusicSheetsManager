#include "importdialog.h"
#include "ui_importdialog.h"
#include "config.h"
#include "consql.h"

ImportDialog::ImportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDialog)
{
    ui->setupUi(this);

    // 调性选项设置
    QStringList keys = {
        // 大调
        "C",
        "G",  "D",  "A",  "E",  "B",  "F♯",  "C♯",
        "F",  "B♭", "E♭", "A♭", "D♭", "G♭", "C♭",
        // 和声小调
        "Cm",
        "Gm", "Dm", "Am", "Em", "Bm", "F♯m", "C♯m",
        "Fm", "B♭m", "E♭m", "A♭m", "D♭m", "G♭m", "C♭m",
        // 其他
        "其他"
    };
    ui->s_key_comboBox->addItems(keys);

    // lineEdit 只读
    ui->fPathlineEdit->setReadOnly(true);
    ui->fPathlineEdit->setFocusPolicy(Qt::NoFocus);

    // 文件列表初始化
    m_fileModel = new QStringListModel(this);
    ui->file_listView->setModel(m_fileModel);
    ui->file_listView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑

    // 双击删除
    connect(ui->file_listView, &QListView::doubleClicked,
            this, &ImportDialog::onFileListDoubleClicked);

    // 分类选项加载
    loadCategories();

    // 连接类型切换信号，切换时清空文件列表
    connect(ui->s_type_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ImportDialog::onTypeChanged);

    qDebug() << "ImportDialog: 构造函数";
}

ImportDialog::~ImportDialog()
{
    qDebug() << "ImportDialog: 析构函数";
    delete ui;
}

// 选择文件的窗口槽函数
void ImportDialog::on_fPathpushButton_clicked()
{
    qDebug() << "ImportDialog: 点击文件路径按钮";

    const int idx = ui->s_type_comboBox->currentIndex(); // 0=PDF  1=图片
    QStringList files;

    if (idx == 0) { // PDF 单文件
        QString f = QFileDialog::getOpenFileName(
            this,
            tr("选择 PDF"),
            ui->fPathlineEdit->text(),
            tr("PDF 文件 (*.pdf)"));
        if (!f.isEmpty()) files << f;
        qDebug() << "ImportDialog: 选择的PDF文件:" << f;
    } else { // 图片 多文件
        files = QFileDialog::getOpenFileNames(
            this,
            tr("选择图片"),
            ui->fPathlineEdit->text(),
            tr("图片 (*.png *.jpg *.jpeg *.bmp)"));
        qDebug() << "ImportDialog: 选择的图片文件数量:" << files.size();
    }

    // 把结果写回 LineEdit（用;分隔）
    if (files.isEmpty()) {
        qDebug() << "ImportDialog: 未选择文件";
        return;
    }

    ui->fPathlineEdit->setText(files.join(";"));

    // 把路径列表写进 QListView
    m_fileModel->setStringList(files);
    qDebug() << "ImportDialog: 文件列表已更新";
}

void ImportDialog::onFileListDoubleClicked(const QModelIndex &index)
{
    qDebug() << "ImportDialog: 双击文件列表项";

    if (!index.isValid()) return;

    QStringList cur = m_fileModel->stringList();
    cur.removeAt(index.row());        // 删掉对应行
    m_fileModel->setStringList(cur);  // 刷新模型
    ui->fPathlineEdit->setText(cur.join(";")); // 同步 LineEdit

    qDebug() << "ImportDialog: 删除文件列表项，剩余文件数:" << cur.size();
}

void ImportDialog::on_buttonBox_accepted()
{
    qDebug() << "ImportDialog: 点击确定按钮，开始导入";

    // 收数据
    Consql::ScoreMeta meta;
    meta.name = ui->s_name_lineEdit->text().trimmed();
    meta.composer = ui->s_composer_lineEdit->text().trimmed();
    meta.key = ui->s_key_comboBox->currentText();
    meta.categoryId = ui->c_id_comboBox->currentData().toInt();
    meta.version = ui->s_version_lineEdit->text().trimmed();
    meta.type = ui->s_type_comboBox->currentText();
    meta.remark = ui->textEdit->toPlainText().trimmed();

    QStringList files = ui->fPathlineEdit->text()
                            .split(";", Qt::SkipEmptyParts);

    qDebug() << "ImportDialog: 导入参数 - 名称:" << meta.name
             << "作曲家:" << meta.composer
             << "调性:" << meta.key
             << "类型:" << meta.type
             << "文件数量:" << files.size();

    if (files.isEmpty() || meta.name.isEmpty()) {
        qDebug() << "ImportDialog: 验证失败 - 歌名或文件为空";
        QMessageBox::warning(this, "提示", "歌名和文件不能为空！");
        return;
    }

    // 调单例
    qDebug() << "ImportDialog: 调用数据库插入函数";
    qint64 songId = Consql::instance()->insertScore(meta, files);
    if (songId == -1) {
        qDebug() << "ImportDialog: 数据库插入失败";
        QMessageBox::critical(this, "保存失败", "数据库插入错误！");
        return;
    }

    // 成功关闭对话框
    qDebug() << "ImportDialog: 导入成功，歌曲ID:" << songId;
    QMessageBox::information(this, "导入成功", "已成功导入新的乐谱！");
    accept();
}

// 分类下拉选项读取
void ImportDialog::loadCategories()
{
    qDebug() << "ImportDialog: 加载分类选项";

    ui->c_id_comboBox->clear();
    // 第 0 项：「无」→ id = 0
    ui->c_id_comboBox->addItem("无", 0);

    QSqlQuery q(Consql::instance()->database());
    q.exec("SELECT c_id, c_name FROM Category ORDER BY c_name");

    int count = 0;
    while (q.next()) {
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui->c_id_comboBox->addItem(name, id);   // 用户数据 = 真实主键
        count++;
    }

    qDebug() << "ImportDialog: 加载分类完成，共" << count << "个分类";
}

// 当文件类型改变时清空文件列表
void ImportDialog::onTypeChanged(int index)
{
    qDebug() << "ImportDialog: 文件类型改变，当前索引:" << index;

    Q_UNUSED(index); // 不使用index参数，但保持函数签名一致

    // 清空文件列表模型
    m_fileModel->setStringList(QStringList());

    // 清空路径显示
    ui->fPathlineEdit->clear();

    qDebug() << "ImportDialog: 文件列表已清空";
}
