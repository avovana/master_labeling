#pragma once

#include <QObject>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_new_connection();
    void on_server_read();
    void on_client_disconnected();

private:
    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    int current = 0;
};
