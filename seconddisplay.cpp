/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

#include "seconddisplay.h"

#include <QTimer>
#include <QClipboard>

SecondDisplay::SecondDisplay(QString ip, int udpPort, QString scrollEvent, QWidget *parent) :
    ip(ip),udpPort(udpPort),scrollEvent(scrollEvent),QWidget(parent,Qt::Dialog),
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

    cursorObserver = QSharedPointer<CursorObserver>::create(width,height,ip,udpPort,scrollEvent,true);
    connect(this,SIGNAL(passClipboardContent(QString)),cursorObserver.data(),SLOT(passClipboardContentSlot(QString)));

    connect(cursorObserver.data(),SIGNAL(virtualModeOn()),this,SLOT(virtualModeOn()));
    connect(cursorObserver.data(),SIGNAL(virtualModeOff()),this,SLOT(virtualModeOff()));

    cursorObserver->start();

    clipBoard = QApplication::clipboard();
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
    QString clipBoardText = clipBoard->text();
    if(clipBoardContent != clipBoardText) {
        clipBoardContent = clipBoardText;
        emit passClipboardContent(clipBoardText);
    }

}

