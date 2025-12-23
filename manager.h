#ifndef MANAGER_H
#define MANAGER_H

#include <QTabWidget>
#include <QDialog>
#include <QStandardItem>
#include <QStringListModel>

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

private:
    Ui::Manager *ui;
    QStandardItemModel *m_listModel;
    QStandardItemModel *m_file_list;
    QStringList m_files;

    void loadSongList();
    void loadCategories();
    void fileListUpdate();

    struct songInfo
    {
        qint64 m_s_id;
        QString m_s_name;
        QString m_s_composer;
        QString m_s_key;
        //QString m_s_addDate;
        //bool m_s_isFav;
        QString m_s_remark;
        QString m_s_version;
        QString m_s_type;
        qint64 m_c_id;
    }si;

    //内容指纹锁
    struct FilterCache {
        int id = -1;
        QString name = "-";
    };
    FilterCache m_lastFilter;

};

#endif // MANAGER_H
