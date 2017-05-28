/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

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
    explicit SecondDisplay(QString ip, int udpPort, QString scrollEvent, QWidget *parent = 0);
    ~SecondDisplay();

    int width;
    int height;

    QSharedPointer<CursorObserver> cursorObserver;

    QClipboard* clipBoard;
    QString clipBoardContent;

private:
    Ui::SecondDisplay *ui;

    QString ip;
    int udpPort;
    QString scrollEvent;

    bool virtualMode;

signals:
    void passClipboardContent(QString clipBoardText);
public slots:
    void virtualModeOn(void);
    void virtualModeOff(void);
    void fixCursor(void);   
};

#endif // SECONDDISPLAY_H
