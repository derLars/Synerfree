/* Author: Lars Schwensen
 * Project: Synerfree
 * Date: 28/05/17
 *
 * Synerfree allows the use of the mouse & keyboard of the server computer
 * on the client computer.
 */

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

    void displayEvents(void);
private slots:
    void start(void);

public slots:
    void stop(void);
    void receiveClipboardContent(QString clipBoardText);
};

#endif // MAINWINDOW_H
