/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */
#include <QDir>
#include <QDirIterator>
#include <QClipboard>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);    

    ui->startPushButton->setEnabled(true);
    ui->stopPushButton->setEnabled(false);
    ui->stackedWidget->setEnabled(true);

    connect(ui->startPushButton,SIGNAL(clicked(bool)),this,SLOT(start(void)));
    connect(ui->stopPushButton,SIGNAL(clicked(bool)),this,SLOT(stop(void)));

    displayEvents();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::start(void) {
    ui->startPushButton->setEnabled(false);
    ui->stopPushButton->setEnabled(true);
    ui->stackedWidget->setEnabled(false);
    ui->serverRadioButton->setEnabled(false);
    ui->clientRadioButton->setEnabled(false);

    auto ip = ui->ipLineEdit->text();
    auto port = ui->portSpinBox->value();
    auto scrollEvent = ui->eventComboBox->currentText();

    if(ui->clientRadioButton->isChecked()) {
        QRect rec = QApplication::desktop()->screenGeometry();

        mouseMover = QSharedPointer<CursorObserver>::create(rec.width(),rec.height(),ip,port,scrollEvent,false);
        mouseMover->start();
    } else {
        secondDisplay = QSharedPointer<SecondDisplay>::create(ip,port,scrollEvent);
        secondDisplay->show();
    }
    this->setWindowState(Qt::WindowMinimized);
}

void MainWindow::stop(void) {
    ui->startPushButton->setEnabled(true);
    ui->stopPushButton->setEnabled(false);
    ui->stackedWidget->setEnabled(true);
    ui->serverRadioButton->setEnabled(true);
    ui->clientRadioButton->setEnabled(true);

    if(!secondDisplay.isNull()) {
        secondDisplay.reset();
    }
    if(!mouseMover.isNull()) {
        mouseMover->running = false;
        mouseMover->wait();
    }
}

void MainWindow::displayEvents(void) {
    for(int i=0; i<100; i++) {
        QString file = QString("/dev/input/event") + QString::number(i);
        if(QFileInfo(file).exists()) {
            ui->eventComboBox->addItem(file);
        }
    }
}

