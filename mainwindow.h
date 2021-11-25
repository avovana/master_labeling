#pragma once

#include <QObject>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

#include <QtWidgets>
#include <QHBoxLayout>

#include <fstream>
#include <iostream>
#include <string>
#include <map>

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

enum class TaskStatus {
    INIT,
    TASK_SEND,
    IN_PROGRESS,
    FINISHED
};

std::ostream& operator<< (std::ostream& os, TaskStatus state);

struct TaskInfo {
    TaskInfo(uint8_t line_number_, uint8_t task_number_, const map<string, string>& product_names_):
        line_number(line_number_),
        task_number(task_number_),
        product_names(product_names_)
    {
        new_layout = new QHBoxLayout();

        line_number_label = new QLabel();
        line_number_label->setStyleSheet("QLabel{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        line_number_label->setText(QString::number(line_number));
        line_number_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        line_number_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        line_status_label = new QLabel();
        line_status_label->setStyleSheet("QLabel{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        line_status_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        product_name_combobox = new QComboBox();
        product_name_combobox->setStyleSheet("QLineEdit{font-size: 24px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        product_name_combobox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        product_name_combobox->setStyleSheet("QComboBox{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        for(auto& [name_rus, name_eng] : product_names)
            product_name_combobox->addItem(QString::fromStdString(name_rus));

        plan_lineedit = new QLineEdit();
        plan_lineedit->setStyleSheet("QLineEdit{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        plan_lineedit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        current_label = new QLineEdit();
        current_label->setStyleSheet("QLineEdit{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        current_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        line_state_pushbutton = new QPushButton();
        line_state_pushbutton->setText("Ожидание подключения линии");
        line_state_pushbutton->setStyleSheet("QPushButton{font-size: 25px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        line_state_pushbutton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        line_state_pushbutton->setObjectName(QString("%1;%2").arg(line_number).arg(task_number));

        //static int line_position = 2;
        //line_position++;
        new_layout->addWidget(line_number_label);
        new_layout->addWidget(line_status_label);
        new_layout->addWidget(product_name_combobox);
        new_layout->addWidget(plan_lineedit);
        new_layout->addWidget(current_label);
        new_layout->addWidget(line_state_pushbutton);
    }

    ~TaskInfo() {
        delete new_layout;
        delete line_number_label;
        delete line_status_label;
        delete product_name_combobox;
        delete plan_lineedit;
        delete current_label;
        delete line_state_pushbutton;
    }

    auto& layout() {
        return new_layout;
    }

    auto& state_button() {
        return line_state_pushbutton;
    }

    auto number() {
        return task_number;
    }

    auto product_name_eng() {
        return product_names[product_name_combobox->currentText().toStdString()];
    }

    auto product_name_rus() {
        return product_name_combobox->currentText().toStdString();
    }

    auto plan() {
        return plan_lineedit->text().toInt();
    }

    auto set_current(QString scan_number) {
        current_label->setText(scan_number);
    }

    void set_status(TaskStatus status_) {
        m_status = status_;
        switch (m_status) {
        case TaskStatus::INIT:
            ;
            break;
        case TaskStatus::IN_PROGRESS:
            line_status_label->setStyleSheet("QLabel{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: green;}");
            break;
        case TaskStatus::FINISHED:
            line_state_pushbutton->setStyleSheet("QPushButton{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: red;}");
            line_state_pushbutton->setText("Файл получен");
            break;
        default:
            break;
        }
    }

    auto status() {
        return m_status;
    }

    Ui::MainWindow *ui;

    QHBoxLayout *new_layout;
    QLabel *line_number_label;
    QLabel *line_status_label;
    QComboBox *product_name_combobox;
    QLineEdit *plan_lineedit;
    QLineEdit *current_label;
    QPushButton *line_state_pushbutton;
    uint8_t task_number = 0;
    uint8_t line_number = 0;
    TaskStatus m_status = TaskStatus::INIT;
    map<string, string> product_names;
    string file_name;
};

struct LineConnector {
    LineConnector(QTcpSocket* socket_, uint line_number_) :
        socket(socket_),
        line_number(line_number_)
    { }

    void send_tasks(std::vector<std::string> tasks) {
        if(tasks.empty()) {
            QByteArray out_array;
            QDataStream stream(&out_array, QIODevice::WriteOnly);
            unsigned int type = 8;
            unsigned int out_msg_size = sizeof(type);
            stream << out_msg_size;
            stream << type;

            qDebug() << "out_array_size=" << out_array.size();

            socket->write(out_array);
        } else {
            std::string msg;
            for(auto &task: tasks)
                msg.append(task);

            qDebug() << "msg = " << QString::fromStdString(msg);
            qDebug() << "msg_size = " << msg.size();

            QByteArray out_array;
            QDataStream stream(&out_array, QIODevice::WriteOnly);
            unsigned int out_msg_size = msg.size() + sizeof(unsigned int);
            unsigned int type = 4;
            stream << out_msg_size;
            stream << type;
            stream.writeRawData(msg.c_str(), msg.size());

            for(int i = 0; i < out_array.count(); ++i)
              qDebug() << out_array.count() << QString::number(out_array[i], 16);

            qDebug() << "out_array=" << out_array;
            for(int i = 0; i < out_array.count(); ++i)
              qDebug() << "out_array[" << i << "] " << out_array[i];

            qDebug() << "out_array_size=" << out_array.size();

            qDebug() << "socket " << socket;
            socket->write(out_array);
            qDebug() << "here";
        }
    }

    void send_ready() {
        QByteArray out_array;
        QDataStream stream(&out_array, QIODevice::WriteOnly);
        unsigned int out_msg_size = sizeof(unsigned int);
        unsigned int type = 6;
        stream << out_msg_size;
        stream << type;

        qDebug() << " on pushbutton_clicked size: " << out_array.size();

       socket->write(out_array);
    }

    QTcpSocket* socket = nullptr;
    uint8_t line_number = 0;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_new_connection();
    void on_server_read();
    void on_client_disconnected();
    void make_next_action();
private slots:
    void on_make_template_pushbutton_clicked();

    std::map<std::string, std::string> get_vsds();

    void on_add_line_pushbutton_clicked();

    std::vector<TaskInfo> get_tasks_for_line(uint line_number) {
        //std::vector<int> matches;
        //std::copy_if(tasks.begin(), tasks.end(), std::back_inserter(matches), [&](int v) {
        //    return v == findValue;
        //});
        //
        //auto tasks_it = tasks.find(line_number);
        //if(tasks_it == tasks.end()) {
        //    qDebug() << "Tasks doesn't exist! ERROR";
        //    return {};
        //}
        //
        //return tasks_it->second;
    }

    std::map<std::string, std::string> fill_table();
    void update_xml_with_vsds_from_table();

    pair<bool, TaskInfo&> get_task(uint8_t line_number, uint8_t task_number) {
        qDebug() << "look for line_number: " << line_number << " task: " << task_number;
        auto task_itr = std::find_if(begin(tasks), end(tasks),[&](auto task) {
            qDebug() << "task.line_number: " << task.line_number << " task.task_number: " << task.task_number;
            return (line_number == task.line_number) && (task.task_number == task_number);});

        if(task_itr == end(tasks)) {
            qDebug() << "Task didnt found line_number: " << line_number << " task: " << task_number;
        }

        return {task_itr != end(tasks), *task_itr};
    }

    LineConnector& create_connector_if_needed(uint line_number, QTcpSocket * socket) {
        qDebug() << "1";
        auto connector_itr = std::find_if(begin(connectors), end(connectors),
                                        [&line_number](auto connector) { return line_number == connector.line_number;});

        qDebug() << "2";
        if(connector_itr == end(connectors)) {
            connectors.emplace_back(socket, line_number);
            qDebug() << "connector_itr == end(connectors)";
            qDebug() << "create_connector_if_needed " << socket; //qDebug() << "create_connector_if_needed " << &*socket;

            qDebug() << "3";
            return connectors.back();
        }
        else if(connector_itr->socket != socket) {
            qDebug() << "connector_itr->socket != socket";
            connector_itr->socket->close();
            connector_itr->socket = socket;
            qDebug() << "4";
        }

        qDebug() << "5";

        return *connector_itr;
    }

private:
    std::string save_folder;

    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    std::vector<LineConnector> connectors;
    std::list<TaskInfo> tasks;
    map<string, string> product_names;
    std::vector<QTcpSocket *> sockets;

    uint8_t task_counter = 0;

    int current = 0;
    int lines = 0;
};
