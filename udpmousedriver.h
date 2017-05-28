/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

#ifndef UDPMOUSEDRIVER_H
#define UDPMOUSEDRIVER_H


#include <QThread>

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>

using Button = int;

class UDPMouseDriver : public QThread
{   
public:
    UDPMouseDriver(QString mousePath, QString ip, int udpPort, int service);

    void readAndSendMouseInput(void);

    void receiveAndExecuteMouseInput(void);

    void executeButtonclick(Button btn, int val);

    void readMouseOffset(void);

    void setToAbsolutePosition(const int x, const int y);

    void readAndSendScrollInput(void);

    void receiveAndExecuteScrollInput(void);

    bool waitForSetup(void);

    int service;

    bool setUp;
    bool running;
    bool virtualMode;

    void run();

    QMutex mutex;
    QWaitCondition setUpTime;

    int offsetX;
    int offsetY;
private:
    QString mousePath;

    QString ip;
    int udpPort;

    int inputDescr;
    struct input_event inputEvent;

    int fdRel, fdAbs;
    struct input_event evRel[2],evAbs[2], evScroll, key, evS;

    bool absInitialized;

    void initRelInputDevice(void);

    bool initAbsInputDevice(void);
};

#endif // UDPMOUSEDRIVER_H
