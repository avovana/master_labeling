#pragma once

#include <QObject>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

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

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_new_connection();
    void on_server_read();
    void on_client_disconnected();

private slots:
    void on_make_template_pushbutton_clicked();

    void on_line_1_start_pushbutton_clicked();

    std::map<std::string, std::string> get_vsds(const std::string & vsd_path) {
        std::ifstream vsd;

        vsd.open(vsd_path);
        if(not vsd.is_open()) {
            std::cout<<"Open vsd.csv ERROR"<<std::endl;
            return {};
        } else {
            std::cout<<"Open vsd.csv success"<<std::endl;
        }

        std::map<std::string, std::string> vsds;
        std::string line;
        for (int i = 0; std::getline(vsd, line); ++i) {
            int comma_pos = line.find(",");
            string name = line.substr(0, comma_pos);
            string vsd = line.substr(comma_pos + 1);

            cout << "name: " << name << ", vsd: " << vsd << endl;
            vsds.emplace(name,vsd);
        }

        return vsds;
    }

    // Почти всё можно сделать DEBUG LVL. Главное - сделать тест и чтобы он срабатывал
    void update_xml() {
        const string vsd_path = string("vsd.csv");

        const auto& vsd_per_names = get_vsds(vsd_path);
        if(vsd_per_names.empty()) {
            cout << "VSD not foungd ERROR" << endl;
            throw std::logic_error("error");
            close(); // не помню, что за close()
        }

        // XML open
        pugi::xml_document doc;
        if (doc.load_file("positions.xml")) {
            cout << "Load XML success" << endl;
        } else {
            cout << "Load XML ERROR" << endl;
            throw std::logic_error("error");
            close();
        }

        pugi::xml_node positions_xml = doc.child("resources").child("input");

        for (auto const& [name, vsd] : vsd_per_names) {
            std::cout << "  vsd name: " << name << std::endl;
            for (pugi::xml_node position_xml: positions_xml.children("position")) {
                std::string name_in_xml = position_xml.attribute("name_english").as_string();
                std::cout << "   name_in_xml: " << name_in_xml << std::endl;
                if(name == name_in_xml) {
                    position_xml.attribute("vsd").set_value(vsd.c_str());
                    cout << "    name: " << name << endl;
                    cout << "    set vsd to xml: " << vsd << endl;
                }
            }
        }

        if(not doc.save_file("positions.xml")) {
            cout << "Update XML ERROR" << endl;
            throw std::logic_error("error");
        } else {
            cout << "Update XML success" << endl;
        }
    }

private:
    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    int current = 0;
};
