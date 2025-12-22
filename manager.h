#ifndef MANAGER_H
#define MANAGER_H

#include <QTabWidget>

namespace Ui {
class Manager;
}

class Manager : public QTabWidget
{
    Q_OBJECT

public:
    explicit Manager(QWidget *parent = nullptr);
    ~Manager();

private:
    Ui::Manager *ui;
    void loadSongList();
};

#endif // MANAGER_H
