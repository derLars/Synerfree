#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "seconddisplay.h"

//class SecondDisplay;
class MouseMoverThread;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QSharedPointer<SecondDisplay> secondDisplay;
    QSharedPointer<CursorObserver> mouseMover;
private slots:
    void start(void);

public slots:
    void stop(void);
};

#endif // MAINWINDOW_H
