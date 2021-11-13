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

class MainWindow : public QMainWindow {
    Q_OBJECT

enum class TaskStatus {
    INIT,
    STARTED,
    FINISHED
};

struct TaskInfo {
    TaskInfo(Ui::MainWindow *ui_, uint line_number, vector<string> vsds_names):
        ui(ui_)
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
        for(auto vsd_name : vsds_names)
            product_name_combobox->addItem(QString::fromStdString(vsd_name));

        plan_lineedit = new QLineEdit();
        plan_lineedit->setStyleSheet("QLineEdit{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        plan_lineedit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        current_label = new QLineEdit();
        current_label->setStyleSheet("QLineEdit{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        current_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        line_start_pushbutton = new QPushButton();
        line_start_pushbutton->setText("Старт");
        line_start_pushbutton->setStyleSheet("QPushButton{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
        line_start_pushbutton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        //static int line_position = 2;
        //line_position++;
        new_layout->addWidget(line_number_label);
        new_layout->addWidget(line_status_label);
        new_layout->addWidget(product_name_combobox);
        new_layout->addWidget(plan_lineedit);
        new_layout->addWidget(current_label);
        new_layout->addWidget(line_start_pushbutton);

        //ui->lines_layout->addLayout(new_layout);
    }

    auto number() {
        return task_number;
    }

    auto product_name() {
        return product_name_combobox->currentText();
    }

    auto plan() {
        return plan_lineedit->text().toInt();
    }

    auto set_current(QString scan_number) {
        current_label->setText(scan_number);
    }

    void set_status(TaskStatus status_) {
        status = status_;
        switch (status) {
        case TaskStatus::INIT:
            ;
            break;
        case TaskStatus::STARTED:
            line_status_label->setStyleSheet("QLabel{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: green;}");
            break;
        case TaskStatus::FINISHED:
            line_start_pushbutton->setStyleSheet("QPushButton{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: red;}");
            line_start_pushbutton->setText("Файл получен");
            break;
        default:
            break;
        }
    }

    Ui::MainWindow *ui;

    QHBoxLayout *new_layout;
    QLabel *line_number_label;
    QLabel *line_status_label;
    QComboBox *product_name_combobox;
    QLineEdit *plan_lineedit;
    QLineEdit *current_label;
    QPushButton *line_start_pushbutton;
    uint8_t task_number = 0;
    TaskStatus status = TaskStatus::INIT;
};

struct LineDescriptor {
    //Q_OBJECT
    LineDescriptor(Ui::MainWindow *ui_, uint line_number_, vector<string> vsds_names_) :
        ui(ui_),
        line_number(line_number_),
        vsds_names(vsds_names_)
    {
        add_task();
    }

    void add_task() {
        ++task_number;
        TaskInfo ti = TaskInfo(ui, line_number, vsds_names);
        //tasks[task_number] = TaskInfo(ui, line_number, vsds_names);
        //tasks.emplace(line_number, ti);
        tasks.try_emplace(line_number, ui, line_number, vsds_names);
    }

    const TaskInfo& get_last_task() {
        return tasks.rbegin()->second;
    }

    Ui::MainWindow *ui;
    uint8_t line_number;
    uint8_t task_number = 0;

    std::vector<string> vsds_names;

    QTcpSocket *socket;
    std::map<uint8_t, TaskInfo> tasks;
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
            string msg;
            for(auto &task: tasks)
                msg.append(task);

            qDebug() << "msg_size = " << msg.size();

            QByteArray out_array;
            QDataStream stream(&out_array, QIODevice::WriteOnly);
            unsigned int out_msg_size = msg.size() + sizeof(unsigned int);
            unsigned int type = 4;
            stream << out_msg_size;
            stream << type;
            //stream.writeRawData(msg.c_str(), msg.size());

            for(int i = 0; i < out_array.count(); ++i)
              qDebug() << out_array.count() << QString::number(out_array[i], 16);

            qDebug() << "out_array=" << out_array;
            for(int i = 0; i < out_array.count(); ++i)
              qDebug() << "out_array[" << i << "] " << out_array[i];

            qDebug() << "out_array_size=" << out_array.size();

            socket->write(out_array);
        }
    }

    QTcpSocket* socket;
    uint8_t line_number;
};

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_new_connection();
    void on_server_read();
    void on_client_disconnected();
    void send_ready_to_slave();
private slots:
    void on_make_template_pushbutton_clicked();

    std::map<std::string, std::string> get_vsds(const std::string & vsd_path);
    void update_xml(); // Почти всё можно сделать DEBUG LVL. Главное - сделать тест и чтобы он срабатывал

    void on_add_line_pushbutton_clicked();

    std::vector<TaskInfo> get_tasks_for_line(uint line_number) {
        auto tasks_it = tasks_per_line.find(line_number);
        if(tasks_it == tasks_per_line.end()) {
            qDebug() << "Tasks doesn't exist! ERROR";
            return {};
        }

        return tasks_it->second;
    }

private:
    std::string save_folder;

    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    //LineDescriptor *line_descriptor;
    std::vector<LineConnector> connectors;
    std::map<uint8_t, std::vector<TaskInfo>> tasks_per_line;
    std::vector<std::string> vsds_names;
    std::vector<QTcpSocket *> sockets;

    int current = 0;
    int lines = 0;
};
