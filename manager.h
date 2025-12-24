#ifndef MANAGER_H
#define MANAGER_H

#include <QTabWidget>
#include <QStandardItemModel>
#include <QStringList>
#include <QMessageBox>
#include <QSqlQuery>

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
    void on_pushButton_delsong_clicked();
    void on_pushButton_savesong_clicked();
    void on_fPathpushButton_clicked();

private:
    Ui::Manager *ui;
    QStandardItemModel *m_listModel;
    QStandardItemModel *m_file_list;
    QStringList m_files;

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
