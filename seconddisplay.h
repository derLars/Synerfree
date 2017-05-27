#ifndef SECONDDISPLAY_H
#define SECONDDISPLAY_H

#include <QWidget>
#include <QThread>
#include <QDebug>
#include <QDesktopWidget>
#include <QPoint>
#include "ui_seconddisplay.h"

#include "cursorobserver.h"
namespace Ui {
class SecondDisplay;
}

class SecondDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit SecondDisplay(QString ip, int udpPort, QWidget *parent = 0);
    ~SecondDisplay();

    int width;
    int height;

    QSharedPointer<CursorObserver> cursorObserver;
private:
    Ui::SecondDisplay *ui;

    QString ip;
    int udpPort;

    bool virtualMode;

public slots:
    void virtualModeOn(void);
    void virtualModeOff(void);
    void fixCursor(void);
};

#endif // SECONDDISPLAY_H
