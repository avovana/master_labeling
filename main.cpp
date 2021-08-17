#include "mainwindow.h"
#include "mytcpserver.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    MyTcpServer server;

    w.show();
    return a.exec();
}
