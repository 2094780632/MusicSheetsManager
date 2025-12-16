#include "importdialog.h"
#include "ui_importdialog.h"
#include "config.h"
#include "consql.h"

ImportDialog::ImportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDialog)
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

    //lineEdit只读
    ui->fPathlineEdit->setReadOnly(true);   // 关键
    ui->fPathlineEdit->setFocusPolicy(Qt::NoFocus);

    //文件列表初始化
    m_fileModel = new QStringListModel(this);
    ui->file_listView->setModel(m_fileModel);

    ui->file_listView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
    connect(ui->file_listView, &QListView::doubleClicked,
            this, &ImportDialog::onFileListDoubleClicked);

    //分类选项加载
    loadCategories();

}

ImportDialog::~ImportDialog()
{
    delete ui;
}

//选择文件的窗口槽函数
void ImportDialog::on_fPathpushButton_clicked()
{
    const int idx = ui->s_type_comboBox->currentIndex(); // 0=PDF  1=图片
    QStringList files;

    if (idx == 0) { // PDF 单文件
        QString f = QFileDialog::getOpenFileName(
            this,
            tr("选择 PDF"),
            ui->fPathlineEdit->text(),
            tr("PDF 文件 (*.pdf)"));
        if (!f.isEmpty()) files << f;
    } else {        // 图片 多文件
        files = QFileDialog::getOpenFileNames(
            this,
            tr("选择图片"),
            ui->fPathlineEdit->text(),
            tr("图片 (*.png *.jpg *.jpeg *.bmp)"));
    }

    // 把结果写回 LineEdit（用 ; 分隔）
    if (files.isEmpty()) return;

    ui->fPathlineEdit->setText(files.join(";"));

    //把路径列表写进 QListView
    m_fileModel->setStringList(files);
}

void ImportDialog::onFileListDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QStringList cur = m_fileModel->stringList();
    cur.removeAt(index.row());        //删掉对应行
    m_fileModel->setStringList(cur);  //刷新模型
    ui->fPathlineEdit->setText(cur.join(";")); // 同步 LineEdit
}

void ImportDialog::on_buttonBox_accepted()
{
    //收数据
    Consql::ScoreMeta meta;
    meta.name      = ui->s_name_lineEdit->text().trimmed();
    meta.composer  = ui->s_composer_lineEdit->text().trimmed();
    meta.key       = ui->s_key_comboBox->currentText();
    meta.categoryId = ui->c_id_comboBox->currentData().toInt();
    meta.version   = ui->s_version_lineEdit->text().trimmed();
    meta.type      = ui->s_type_comboBox->currentText();
    meta.remark    = ui->textEdit->toPlainText().trimmed();

    QStringList files = ui->fPathlineEdit->text()
                            .split(";", Qt::SkipEmptyParts);
    if (files.isEmpty() || meta.name.isEmpty()) {
        QMessageBox::warning(this, "提示", "歌名和文件不能为空！");
        return;
    }

    //调单例
    qint64 songId = Consql::instance()->insertScore(meta, files);
    if (songId == -1) {
        QMessageBox::critical(this, "保存失败", "数据库插入错误！");
        return;
    }

    //成功关闭对话框
    QMessageBox::information(this, "导入成功", "已成功导入新的乐谱！");
    accept();
}

//分类下拉选项读取
void ImportDialog::loadCategories()
{
    ui->c_id_comboBox->clear();
    // 第 0 项：「无」→ id = 0
    ui->c_id_comboBox->addItem("无", 0);

    QSqlQuery q(Consql::instance()->database());
    q.exec("SELECT c_id, c_name FROM Category ORDER BY c_name");
    while (q.next()) {
        int  id   = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui->c_id_comboBox->addItem(name, id);   // 用户数据 = 真实主键
    }
}
