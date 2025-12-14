#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QStringListModel>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

namespace Ui {
class ImportDialog;
}

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportDialog(QWidget *parent = nullptr);
    ~ImportDialog();

    qint64 insertScore();

private slots:
    void on_fPathpushButton_clicked();
    void on_buttonBox_accepted();
    void onFileListDoubleClicked(const QModelIndex &index);

private:
    Ui::ImportDialog *ui;
    QStringListModel *m_fileModel;
};

#endif // IMPORTDIALOG_H
