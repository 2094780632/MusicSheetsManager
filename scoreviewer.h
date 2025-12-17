#ifndef SCOREVIEWER_H
#define SCOREVIEWER_H

#pragma once
#include <QWidget>
#include <QtPdf/QPdfDocument>

namespace Ui {
class ScoreViewer;
}

class ScoreViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ScoreViewer(qint64 songId,QWidget *parent = nullptr);
    ~ScoreViewer();

private:
    Ui::ScoreViewer *ui;

    void loadScore(qint64 id);

    qint64 m_songId;
    QPdfDocument m_doc;
};

#endif // SCOREVIEWER_H
