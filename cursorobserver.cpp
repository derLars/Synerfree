/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

#include "cursorobserver.h"

CursorObserver::CursorObserver(int width, int height, QString ip, int udpPort, QString scrollEvent, bool server):width(width),height(height),ip(ip),udpPort(udpPort),scrollEvent(scrollEvent),server(server) {
    x=400;
    y=400;
    prevX = 0;
    prevY = 0;
    virtualMode = false;
}

void CursorObserver::run() {
    if(server) {
        handleAsUbuntuServer();
    }else {
        handleAsArchClient();
    }
}

void CursorObserver::handleAsUbuntuServer(void) {
    QUdpSocket udpSocket;
    QByteArray datagram;
    datagram.resize(1);
    if(!udpSocket.bind(udpPort)) {
        qDebug() << "Could not bind to port " << udpPort;
        return;
    }
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mice",ip,udpPort,1));

    QList<int> indToDel;
    for(int i=0; i<drivers.size(); i++) {
        drivers[i]->start();
        if(!drivers[i]->waitForSetup()) {
            indToDel.insert(0,i);
        }
    }

    for(auto i : indToDel) {
        drivers[i]->wait();
        drivers.removeAt(i);
    }

    display = QSharedPointer<Display>(XOpenDisplay(0));
    window = XRootWindow(display.data(),0);

    running = true;
    while(running && !drivers.isEmpty()) {
        for(int i=0; i<drivers.size(); i++) {
            if(!drivers[i]->running) {
                drivers.removeAt(i);
                break;
            }
            drivers[i]->virtualMode = virtualMode;
        }
        getAbsCoord(x,y);

        if(virtualMode) {
            datagram.clear();
            datagram.resize(1);
            while(udpSocket.hasPendingDatagrams()) {
                udpSocket.readDatagram(datagram.data(),1);
            }

            if(datagram.at(0) == 0x01) {
                virtualMode = false;
                emit virtualModeOff();
            }
            datagram.replace(0,1,(const char*)"9");
        }
        while(udpSocket.hasPendingDatagrams()) {
            udpSocket.readDatagram(datagram.data(),1);
        }

        if(prevX != x || prevY != y) {
            if(!virtualMode && x >= width-2) {
                emit virtualModeOn();
                virtualMode = true;
            }
            //qDebug() << "x: " << x << " y: " << y;
        }

        prevX = x;
        prevY = y;
        QThread::msleep(10);
    }

}

void CursorObserver::handleAsServer(void) {
    QUdpSocket udpSocket;
    QByteArray datagram;

    datagram.resize(1);
    if(!udpSocket.bind(udpPort)) {
        qDebug() << "Clould not bind to udpPort!";
    }

    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse0",ip,udpPort,1));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse1",ip,udpPort,1));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse2",ip,udpPort,1));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse3",ip,udpPort,1));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse4",ip,udpPort,1));

    for(int i=0; i<drivers.size(); i++) {
        drivers[i]->start();       
    }

    running = true;
    drivers.last()->setToAbsolutePosition(x,y);
    while(running && !drivers.isEmpty()) {
        for(int i=0; i<drivers.size(); i++) {
            if(!drivers[i]->running) {
                drivers.removeAt(i);
                break;
            }
            drivers[i]->virtualMode = virtualMode;

            QMutexLocker lock(&(drivers[i]->mutex));
            x += drivers[i]->offsetX;
            y -= drivers[i]->offsetY;

            x = x > 0 ? x : 0;
            y = y > 0 ? y : 0;

            //x = x < width ? x : width;
            y = y < height ? y : height;

            drivers[i]->offsetX = 0;
            drivers[i]->offsetY = 0;
        }

        drivers.last()->setToAbsolutePosition(x,y);
        if(prevX != x || prevY != y) {
            qDebug() << "x: " << x << " y: " << y;

            if(x > width) {
                virtualMode = true;
                emit virtualModeOff();
                x=width-2;
            }
            if(virtualMode) {
                while(udpSocket.hasPendingDatagrams()) {
                    udpSocket.readDatagram(datagram.data(),1);
                }
                if(datagram.at(0) == 0x01) {
                    virtualMode = false;
                    emit virtualModeOff();
                }
            }
        }
    }
}

void CursorObserver::handleAsArchClient(void) {
    QUdpSocket udpSocket;
    QByteArray left,right;
    left.append(0x01);
    right.append(0x02);

    drivers.append(QSharedPointer<UDPMouseDriver>::create("",ip,udpPort,2));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse0",ip,udpPort,0));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse1",ip,udpPort,0));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse2",ip,udpPort,0));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse3",ip,udpPort,0));
    drivers.append(QSharedPointer<UDPMouseDriver>::create("/dev/input/mouse4",ip,udpPort,0));

    QList<int> indToDel;
    for(int i=0; i<drivers.size(); i++) {
        drivers[i]->start();
        if(!drivers[i]->waitForSetup()) {
            indToDel.insert(0,i);
        }
    }

    for(auto i : indToDel) {
        drivers[i]->wait();
        drivers.removeAt(i);
    }

    running = true;
    drivers.last()->setToAbsolutePosition(x,y);
    while(running && !drivers.isEmpty()) {
        for(int i=0; i<drivers.size(); i++) {
            if(!drivers[i]->running) {
                drivers.removeAt(i);
                break;
            }

            QMutexLocker lock(&(drivers[i]->mutex));
            x += drivers[i]->offsetX;
            y -= drivers[i]->offsetY;

            //x = x > 0 ? x : 0;
            y = y > 0 ? y : 0;

            //x = x < width ? x : width;
            y = y < height ? y : height;

            drivers[i]->offsetX = 0;
            drivers[i]->offsetY = 0;
        }
        drivers.last()->setToAbsolutePosition(x,y);
        if(prevX != x || prevY != y) {
            qDebug() << "x: " << x << " y: " << y;

            if(x < 0) {
                udpSocket.writeDatagram(left, 1,QHostAddress(ip), udpPort);
                qDebug() << "send left";
                x=10;
            } else if(x > width) {
                udpSocket.writeDatagram(right, 1,QHostAddress(ip), udpPort);
                x=width;
            }
        }
        prevX = x;
        prevY = y;
        QThread::msleep(10);
    }
}

void CursorObserver::getAbsCoord(int& x, int& y) {
    int winX, winY;
    unsigned int maskReturn;

    XQueryPointer(display.data(), window, &retWindow, &retWindow, &x, &y, &winX, &winY, &maskReturn);
}
