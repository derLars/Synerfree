#include "seconddisplay.h"

#include <QTimer>

SecondDisplay::SecondDisplay(QString ip, int udpPort, QWidget *parent) :
    ip(ip),udpPort(udpPort),QWidget(parent,Qt::Dialog),
    ui(new Ui::SecondDisplay) {
    ui->setupUi(this);

    QRect rec = QApplication::desktop()->screenGeometry();
    height = rec.height();
    width = rec.width();

    setStyleSheet("background:transparent;");
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint|Qt::Dialog|Qt::ToolTip);

    this->setFixedHeight(1);
    this->setFixedWidth(1);
    this->move(0,0);
    this->setCursor(Qt::BlankCursor);

    cursorObserver = QSharedPointer<CursorObserver>::create(width,height,ip,udpPort,true);

    connect(cursorObserver.data(),SIGNAL(virtualModeOn()),this,SLOT(virtualModeOn()));
    connect(cursorObserver.data(),SIGNAL(virtualModeOff()),this,SLOT(virtualModeOff()));

    cursorObserver->start();
}

SecondDisplay::~SecondDisplay() {
    delete ui;
}

void SecondDisplay::virtualModeOn(void) {
    virtualMode = true;

    this->setFixedWidth(width);
    this->setFixedHeight(height);
    this->cursor().setPos(width/2,height/2);

    QTimer::singleShot(300,this,SLOT(fixCursor()));
}

void SecondDisplay::virtualModeOff(void) {
    virtualMode = false;

    this->setFixedWidth(1);
    this->setFixedHeight(1);
    this->cursor().setPos(width-15,height/2);
}

void SecondDisplay::fixCursor(void) {
    if(virtualMode) {        
        this->cursor().setPos(width/2,height/2);
        QTimer::singleShot(300,this,SLOT(fixCursor()));
    }
}
