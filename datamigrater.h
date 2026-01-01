#ifndef DATAMIGRATER_H
#define DATAMIGRATER_H

#include <QTabWidget>
#include <QMessageBox>

namespace Ui {
class DataMigrater;
}

class DataMigrater : public QTabWidget
{
    Q_OBJECT

public:
    explicit DataMigrater(QWidget *parent = nullptr);
    ~DataMigrater();

private slots:

    void on_export_pushButton_clicked();
    void on_import_pushButton_clicked();


private:
    Ui::DataMigrater *ui;
};

#endif // DATAMIGRATER_H
