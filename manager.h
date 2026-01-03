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
    void loadEditInfo(const QModelIndex &index);                //加载歌曲信息
    void loadCateInfo(const QModelIndex &index);                //加载分类信息

    void onTypeChanged(int index);                              //当 分类下拉选项框 变动时 清空文件列表
    void onFileListDoubleClicked(const QModelIndex &index);     //当 被双击时 删除被点击的文件

    int on_pushButton_importSong_clicked();                     //导入歌曲按钮 槽
    void on_pushButton_delsong_clicked();                       //删除歌曲按钮 槽
    void on_pushButton_savesong_clicked();                      //保存修改按钮 槽
    void on_fPathpushButton_clicked();                          //选择文件按钮 槽

    void on_pushButton_newcat_clicked();                        // 新建分类
    void on_pushButton_savecat_clicked();                       // 保存分类修改
    void on_pushButton_delcat_clicked();                        // 删除分类

    void loadSongList();                                        //加载歌曲列表
    void loadCategories();                                      //加载分类列表
    void fileListUpdate();                                      //文件列表更新
    void clearEditFields();                                     //清空编辑字段·

private:
    Ui::Manager *ui;
    QStandardItemModel *m_listModel;
    QStandardItemModel *m_file_list;
    QStringList m_files;

    int m_currentCategoryId = 0;                  // 当前选中的分类ID
    QString m_originalCateName;               // 原始分类名（用于重名检查）



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
