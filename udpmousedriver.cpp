/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

#include "udpmousedriver.h"

#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QFile>

UDPMouseDriver::UDPMouseDriver(QString mousePath, QString ip, int udpPort, int service):mousePath(mousePath),ip(ip),udpPort(udpPort),service(service) {
    offsetX = 0;
    offsetY = 0;

    setUp = false;
    running = false;
    virtualMode = false;
    absInitialized = false;
}

void UDPMouseDriver::run() {
    switch(service) {
        case 0:
            readMouseOffset();
            break;
        case 1:
            readAndSendMouseInput();
            break;
        case 2:
            receiveAndExecuteMouseInput();
            break;
        case 3:
            readAndSendScrollInput();
            break;
        case 4:
            receiveClipBoardInput();
        default:
            break;
    }
    running = false;
}

bool UDPMouseDriver::waitForSetup(void) {
    QMutexLocker lock(&mutex);
    while(!setUp) {
        setUpTime.wait(&mutex);        
    }
    return running;
}

void UDPMouseDriver::readMouseOffset(void) {    
    if((inputDescr = open(mousePath.toLatin1().data(), O_RDONLY)) == -1) {
       qDebug() << "No access to path! (" << mousePath << ") Do you have the rights?";
       running = false;
       setUp = true;       
       setUpTime.wakeAll();
       return;
    }

    running = true;
    setUp = true;    
    setUpTime.wakeAll();   
    while(running  && read(inputDescr, &inputEvent, sizeof(struct input_event))) {
        const char *bytes = (char*)&inputEvent;

        QMutexLocker lock(&mutex);
        offsetX += (int)bytes[1];
        offsetY += (int)bytes[2];

        QThread::msleep(5);
    }
}

void UDPMouseDriver::readAndSendMouseInput(void) {
    QUdpSocket udpSocket;
    auto clipBoard = QApplication::clipboard();

    if((inputDescr = open(mousePath.toLatin1().data(), O_RDONLY)) == -1) {
       qDebug() << "No access to path! (" << mousePath << ") Do you have the rights?";
       running = false;
       setUp = true;
       setUpTime.wakeAll();
       return;
    }

    running = true;
    setUp = true;
    setUpTime.wakeAll();
    while(running && read(inputDescr, &inputEvent, sizeof(struct input_event))) {        
        const char *bytes = (char*)&inputEvent;

        if(virtualMode) {
            udpSocket.writeDatagram(QByteArray::fromRawData(bytes, sizeof(struct input_event)),QHostAddress(ip), udpPort);

            QMutexLocker lock(&mutex);
            offsetX += (int)bytes[1];
            offsetY += (int)bytes[2];
        }
    }

    close(inputDescr);
}

void UDPMouseDriver::receiveAndExecuteMouseInput(void) {
    QUdpSocket udpSocketMove, udpSocketScroll;
    QByteArray datagram;

    initRelInputDevice();

    datagram.resize(sizeof(struct input_event));
    if(!udpSocketMove.bind(udpPort)) {
        qDebug() << "Clould not bind to udpPort!";
        running = false;
        setUp = true;
        setUpTime.wakeAll();
        return;
    }

    if(!udpSocketScroll.bind(udpPort+1)) {
        qDebug() << "Clould not bind to udpPort!";
        running = false;
        setUp = true;
        setUpTime.wakeAll();
        return;
    }

    running = true;
    setUp = true;
    setUpTime.wakeAll();
    while(running) {
        if(udpSocketMove.hasPendingDatagrams()) {
            udpSocketMove.readDatagram(datagram.data(),sizeof(struct input_event));

            evRel[0].value = datagram[1];
            evRel[1].value = -datagram[2];

            write(fdRel, evRel, sizeof(evRel));
            write(fdRel, &evS, sizeof(struct input_event));

            executeButtonclick(BTN_LEFT, (datagram[0] & 0x01));
            executeButtonclick(BTN_MIDDLE, ((datagram[0] & 0x04) >> 2));
            executeButtonclick(BTN_RIGHT, ((datagram[0] & 0x02) >> 1));

            QMutexLocker lock(&mutex);           
            offsetX += (int)datagram[1];
            offsetY += (int)datagram[2];
        }
        if(udpSocketScroll.hasPendingDatagrams()) {
            udpSocketScroll.readDatagram(datagram.data(),sizeof(struct input_event));

            evScroll.value = datagram[20];
            write(fdRel, &evScroll, sizeof(evScroll));
            write(fdRel, &evS, sizeof(struct input_event));
        }
        QThread::usleep(5);
    }
    close(fdRel);
}

void UDPMouseDriver::executeButtonclick(Button btn, int val) {
    key.code = btn;
    key.value = val;

    write(fdRel, &key, sizeof(key));
    write(fdRel, &evS, sizeof(struct input_event));
}

void UDPMouseDriver::setToAbsolutePosition(const int x, const int y) {
    absInitialized = absInitialized ? absInitialized : initAbsInputDevice();

    evAbs[0].value = x;
    evAbs[1].value = y;

    write(fdAbs, evAbs, sizeof(evAbs));
    write(fdAbs, &evS, sizeof(struct input_event));
}

void UDPMouseDriver::initRelInputDevice(void) {
    fdRel = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    ioctl(fdRel, UI_SET_EVBIT, EV_KEY);
    ioctl(fdRel, UI_SET_KEYBIT, BTN_MOUSE);
    ioctl(fdRel, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fdRel, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fdRel, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(fdRel, UI_SET_EVBIT, EV_REL);
    ioctl(fdRel, UI_SET_RELBIT, REL_X);
    ioctl(fdRel, UI_SET_RELBIT, REL_Y);
    ioctl(fdRel, UI_SET_RELBIT, REL_WHEEL);

    struct uinput_user_dev uidev;
    memset(&uidev,0,sizeof(uidev));
    snprintf(uidev.name,UINPUT_MAX_NAME_SIZE,"RelativeMouse");
    uidev.id.bustype = BUS_USB;
    uidev.id.version = 1;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = 1080;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = 1080;
    write(fdRel, &uidev, sizeof(uidev));
    ioctl(fdRel, UI_DEV_CREATE);

    usleep(100000);

    memset(evRel, 0, sizeof(evRel));
    memset(&evScroll,0, sizeof(struct input_event));
    memset(&key, 0, sizeof(struct input_event));
    memset(&evS,0,sizeof(struct input_event));

    evS.type = EV_SYN;

    evRel[0].type = EV_REL;
    evRel[0].code = REL_X;

    evRel[1].type = EV_REL;
    evRel[1].code = REL_Y;

    evScroll.type = EV_REL;
    evScroll.code = REL_WHEEL;

    key.type = EV_KEY;
}

bool UDPMouseDriver::initAbsInputDevice(void) {
    fdAbs = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    ioctl(fdAbs, UI_SET_EVBIT, EV_KEY);
    ioctl(fdAbs, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fdAbs, UI_SET_EVBIT, EV_ABS);
    ioctl(fdAbs, UI_SET_ABSBIT, ABS_X);
    ioctl(fdAbs, UI_SET_ABSBIT, ABS_Y);

    struct uinput_user_dev uidev;
    memset(&uidev,0,sizeof(uidev));
    snprintf(uidev.name,UINPUT_MAX_NAME_SIZE,"AbsoluteMouse");
    uidev.id.bustype = BUS_USB;
    uidev.id.version = 1;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = 1920;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = 1080;
    write(fdAbs, &uidev, sizeof(uidev));
    ioctl(fdAbs, UI_DEV_CREATE);

    usleep(100000);

    memset(evAbs, 0, sizeof(evAbs));
    memset(&evS,0,sizeof(struct input_event));

    evS.type = EV_SYN;

    evAbs[0].type = EV_ABS;
    evAbs[0].code = ABS_X;

    evAbs[1].type = EV_ABS;
    evAbs[1].code = ABS_Y;

    return true;
}

void UDPMouseDriver::readAndSendScrollInput(void) {
    QUdpSocket udpSocket;

    if((inputDescr = open(mousePath.toLatin1().data(), O_RDONLY)) == -1) {
       qDebug() << "No access to path! (" << mousePath << ") Do you have the rights?";
       running = false;
       setUp = true;
       setUpTime.wakeAll();
       return;
    }

    running = true;
    setUp = true;
    setUpTime.wakeAll();

    while(running && read(inputDescr, &inputEvent, sizeof(struct input_event))) {
        const char *bytes = (char*)&inputEvent;
        if(virtualMode && bytes[16] == EV_REL && bytes[18] == REL_WHEEL) {
            //qDebug() << "scrolling " << (int)bytes[16] << " " << (int)bytes[18] << " " << (int)bytes[20];
            udpSocket.writeDatagram(QByteArray::fromRawData(bytes, sizeof(struct input_event)),QHostAddress(ip), udpPort+1);
        }
    }
}

void UDPMouseDriver::receiveClipBoardInput(void) {
    QTcpServer tcpServer;

    if(!tcpServer.listen(QHostAddress::Any,udpPort+2)) {
        running = false;
        setUp = true;
        setUpTime.wakeAll();
        return;
    }

    running = true;
    setUp = true;
    setUpTime.wakeAll();
    while(running) {
       if(tcpServer.waitForNewConnection(3)) {
           QByteArray nameSizeBytes, nameBytes;
           QByteArray fileSizeBytes, fileBytes;

           auto client = tcpServer.nextPendingConnection();

           client->waitForReadyRead();        
           nameSizeBytes.resize(4);
           client->read(nameSizeBytes.data(), 4);
           int nameSize = 0;

           nameSize |= ((0xFF & nameSizeBytes[0]) << 24);
           nameSize |= ((0xFF & nameSizeBytes[1]) << 16);
           nameSize |= ((0xFF & nameSizeBytes[2]) << 8);
           nameSize |= (0xFF & nameSizeBytes[3]);

           nameBytes.resize(nameSize);
           client->waitForReadyRead();

           client->read(nameBytes.data(),nameSize);
           QString name(nameBytes);

           fileSizeBytes.resize(4);
           client->read(fileSizeBytes.data(), 4);
           int fileSize = 0;

           fileSize |= ((0xFF & fileSizeBytes[0]) << 24);
           fileSize |= ((0xFF & fileSizeBytes[1]) << 16);
           fileSize |= ((0xFF & fileSizeBytes[2]) << 8);
           fileSize |= (0xFF & fileSizeBytes[3]);

           if(fileSize > 0) {
              fileBytes.resize(fileSize);
               client->read(fileBytes.data(),fileSize);

               QFile file("/tmp/" + name);
               if(file.open(QFile::WriteOnly)) {
                   file.write(fileBytes);
                   file.close();
               }
           }
           emit clipboardContentReceived(name);
       }else if(newClipboardContent != clipboardContent) {
           readAndSendClipBoardInput();
       }
       QThread::msleep(500);
   }
}

void UDPMouseDriver::readAndSendClipBoardInput(void) {
    QTcpSocket tcpSocket;

    tcpSocket.connectToHost(QHostAddress(ip), udpPort+2);
    if(!tcpSocket.waitForConnected()) {
        return;
    }

    if(clipboardContent != newClipboardContent) {
        clipboardContent = newClipboardContent;

        QString name = clipboardContent.split("/").last();

        QFile file(clipboardContent);

        QByteArray nameSizeBytes;
        nameSizeBytes.append(0xff & (name.size() >> 24));
        nameSizeBytes.append(0xff & (name.size() >> 16));
        nameSizeBytes.append(0xff & (name.size() >> 8));
        nameSizeBytes.append(0xff & (name.size()));

        tcpSocket.write(nameSizeBytes);
        tcpSocket.waitForBytesWritten(1000);

        QByteArray nameBytes;
        nameBytes.append(name);
        tcpSocket.write(nameBytes);
        tcpSocket.waitForBytesWritten(1000);

        QByteArray fileSizeBytes;
        if(QFileInfo(clipboardContent).exists() && file.size() <= 524288 && file.open(QFile::ReadOnly)) {
            fileSizeBytes.append(0xff & (file.size() >> 24));
            fileSizeBytes.append(0xff & (file.size() >> 16));
            fileSizeBytes.append(0xff & (file.size() >> 8));
            fileSizeBytes.append(0xff & (file.size()));

            tcpSocket.write(fileSizeBytes);
            tcpSocket.waitForBytesWritten(1000);

            QByteArray fileBytes;
            fileBytes.append(file.readAll());

            tcpSocket.write(fileBytes);
            tcpSocket.waitForBytesWritten(1000);

        } else {
            fileSizeBytes.append((char)0x00);
            fileSizeBytes.append((char)0x00);
            fileSizeBytes.append((char)0x00);
            fileSizeBytes.append((char)0x00);

            tcpSocket.write(fileSizeBytes);
            tcpSocket.waitForBytesWritten(1000);
        }
    }
}

