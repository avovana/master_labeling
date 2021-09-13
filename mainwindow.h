#pragma once

#include <QObject>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

#include <QtWidgets>

#include <fstream>
#include <iostream>
#include <string>

#include "pugixml.hpp"

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

using namespace std;

std::ostream& operator<<(std::ostream&  out, Position& pos);

class MainWindow : public QMainWindow {
    Q_OBJECT

struct LineDescriptor {
    //Q_OBJECT
    LineDescriptor(Ui::MainWindow *ui_, uint line_number_);
    void add_to_ui();

    Ui::MainWindow *ui;
    uint8_t line_number;
    QLabel *line_number_label;
    QCheckBox *line_status_checkbox;
    QLineEdit *product_name_lineedit;
    QLineEdit *plan_lineedit;
    QLineEdit *current_label;
    QPushButton *line_start_pushbutton;
    QCheckBox *line_status_finish_checkbox;
    QTcpSocket *socket;
};

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_new_connection();
    void on_server_read();
    void on_client_disconnected();

    void on_line_1_start_pushbutton_clicked();

    void send_ready_to_slave();
private slots:
    void on_make_template_pushbutton_clicked();

    std::map<std::string, std::string> get_vsds(const std::string & vsd_path);
    void update_xml(); // Почти всё можно сделать DEBUG LVL. Главное - сделать тест и чтобы он срабатывал

    void on_add_line_pushbutton_clicked();

private:


    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    //LineDescriptor *line_descriptor;
    std::vector<LineDescriptor> lines_descriptors;
    std::vector<QTcpSocket *> sockets;

    int current = 0;
    int lines = 0;
};
