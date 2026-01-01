#ifndef MANAGER_H
#define MANAGER_H

#include <QTabWidget>
#include <QStandardItemModel>
#include <QStringList>
#include <QMessageBox>
#include <QSqlQuery>
#include <QFileDialog>
#include <QDebug>

namespace Ui {
class Manager;
}

class Manager : public QTabWidget
{
    Q_OBJECT

public:
    explicit Manager(QWidget *parent = nullptr);
    ~Manager();

private slots:
    void loadEditInfo(const QModelIndex &index);
    void loadCateInfo(const QModelIndex &index);

    void onTypeChanged(int index);
    void onFileListDoubleClicked(const QModelIndex &index);

    int on_pushButton_importSong_clicked();
    void on_pushButton_delsong_clicked();
    void on_pushButton_savesong_clicked();
    void on_fPathpushButton_clicked();

    void on_pushButton_newcat_clicked();     // 新建分类
    void on_pushButton_savecat_clicked();    // 保存分类修改
    void on_pushButton_delcat_clicked();     // 删除分类

private:
    Ui::Manager *ui;
    QStandardItemModel *m_listModel;
    QStandardItemModel *m_file_list;
    QStringList m_files;

    int m_currentCategoryId = 0;                  // 当前选中的分类ID
    QString m_originalCateName;               // 原始分类名（用于重名检查）

    void loadSongList();
    void loadCategories();
    void fileListUpdate();
    void clearEditFields();

    struct SongInfo {
        qint64 m_s_id = 0;
        QString m_s_name;
        QString m_s_composer;
        QString m_s_key;
        QString m_s_remark;
        QString m_s_version;
        QString m_s_type;
        qint64 m_c_id = 0;
    } si;

    struct FilterCache {
        qint64 id = 0;
        QString name;
    } m_lastFilter;
};

#endif // MANAGER_H
