#include "udpmousedriver.h"

#include <QUdpSocket>

UDPMouseDriver::UDPMouseDriver(QString mousePath, QString ip, int udpPort, int service):service(service),mousePath(mousePath),ip(ip),udpPort(udpPort) {
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
    }
}

void UDPMouseDriver::readAndSendMouseInput(void) {
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
    QUdpSocket udpSocket;
    QByteArray datagram;

    initRelInputDevice();

    datagram.resize(sizeof(struct input_event));
    if(!udpSocket.bind(udpPort)) {
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
        if(udpSocket.hasPendingDatagrams()) {
            udpSocket.readDatagram(datagram.data(),sizeof(struct input_event));

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
        }else{
            QThread::usleep(3);
        }
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
    memset(&key, 0, sizeof(struct input_event));
    memset(&evS,0,sizeof(struct input_event));

    evS.type = EV_SYN;

    evRel[0].type = EV_REL;
    evRel[0].code = REL_X;

    evRel[1].type = EV_REL;
    evRel[1].code = REL_Y;

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
