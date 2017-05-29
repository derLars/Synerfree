/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

#ifndef CURSOROBSERVER_H
#define CURSOROBSERVER_H

#include "udpmousedriver.h"

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QUdpSocket>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>

class CursorObserver : public QThread
{
    Q_OBJECT
public:
    CursorObserver(int width, int height, QString ip, int udpPort, QString scrollEvent, bool server);

    bool running;
    bool virtualMode;
    void run();

    int x,y;
    int prevX, prevY;

    int width; int height;
private:
    void handleAsServer(void);

    void handleAsUbuntuServer(void);
    void handleAsArchClient(void);

    QString ip;
    int udpPort;
    QString scrollEvent;

    bool server;

    QList<QSharedPointer<UDPMouseDriver> > drivers;

    void getAbsCoord(int& x, int& y);

    Window window, retWindow;
    QSharedPointer<Display> display;
    QSharedPointer<Screen> screen;

signals:
    void virtualModeOn(void);
    void virtualModeOff(void);
    void passClipboardContentSignal(QString clipBoardText);
    void passReceivedClipboardContentSignal(QString clipboardText);

public slots:
    void passClipboardContentSlot(QString clipBoardText);
    void passReceivedClipboardContentSlot(QString clipboardText);
};

#endif // CURSOROBSERVER_H
