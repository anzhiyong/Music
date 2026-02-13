#ifndef LRCPAGE_H
#define LRCPAGE_H

#include <QWidget>
#include <QPropertyAnimation>


// LRC行歌词结构
struct LrcWordLine
{
    LrcWordLine(qint64 lrcTime, QString lrcText)
    {
        this->lrcTime = lrcTime;
        this->lrcText = lrcText;
    }

    qint64 lrcTime;
    QString lrcText;
};

namespace Ui {
class LrcPage;
}

class LrcPage : public QWidget
{
    Q_OBJECT

public:
    explicit LrcPage(QWidget *parent = nullptr);
    ~LrcPage();

    bool parseLrcFile(const QString& lrcFilePath);
    void showLrcWordLine(qint64 time);
    int getLrcWordLineIndex(qint64 time);
    QString getLrcWordByIndex(int index);

private:
    Ui::LrcPage *ui;

    QPropertyAnimation* animal;

    QVector<LrcWordLine> lrcWordLines;
};

#endif // LRCPAGE_H
