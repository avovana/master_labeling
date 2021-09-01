#pragma once

#include <QObject>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

#include <fstream>

namespace Ui {
    class MainWindow;
}

struct Position
{
    std::string code_tn_ved;
    std::string document_type;
    std::string document_number;
    std::string document_date;

    std::string vsd;

    std::string name;
    std::string inn;
    std::string name_english;
    int expected;
    int current;
};

std::ostream& operator<<(std::ostream&  out, Position& pos);

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_new_connection();
    void on_server_read();
    void on_client_disconnected();

private slots:
    void on_make_template_pushbutton_clicked();

private:
    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    int current = 0;
};
