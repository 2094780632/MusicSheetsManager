#ifndef METRONOME_H
#define METRONOME_H

#include <QWidget>
#include <QDialog>
#include <QMessageBox>

namespace Ui {
class Metronome;
}

class Metronome : public QDialog
{
    Q_OBJECT

public:
    explicit Metronome(QWidget *parent = nullptr);
    ~Metronome();

private:
    Ui::Metronome *ui;
};

#endif // METRONOME_H
