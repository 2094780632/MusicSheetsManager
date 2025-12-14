#ifndef INITDIALOG_H
#define INITDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

namespace Ui {
class InitDialog;
}

class InitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitDialog(QWidget *parent = nullptr);
    ~InitDialog();

private slots:
    void on_PathpushButton_clicked();

    void on_buttonBox_accepted();

signals:
    void configPathChanged(const QString &path);

private:
    Ui::InitDialog *ui;
};

#endif // INITDIALOG_H
