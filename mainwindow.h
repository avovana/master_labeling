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
            std::cout<<"Ошибка открытия vsd.csv"<<std::endl;
            return {};
        } else {
            std::cout<<"Открытие vsd.csv успешно"<<std::endl;
        }

        std::map<std::string, std::string> vsds;
        std::string line;
        for (int i = 0; std::getline(vsd, line); ++i) {
            cout << line;
            int pos = line.size() - 12;
            cout << "line: " << line;
            cout << "line.size()=" << line.size()  << " line.size() - 12=" << pos << endl;
            string name = line.substr(0, line.find(","));
            int comma_pos = line.find(",");
            int size_of_vsd = line.size() - 1 - (comma_pos + 1);
            cout << "comma_pos: " << comma_pos << " size_of_vsd: " << size_of_vsd << endl;
            string vsd = line.substr(comma_pos + 1, size_of_vsd);

            cout << "name: " << name << ", vsd: " << vsd << ", vsd.size()=" << vsd.size() << endl;
            cout << "==============" << endl;
            vsds.emplace(name,vsd);
        }

        std::cout<<"Считанные пары: "<<std::endl;

        for (auto const& [name, vsd] : vsds)
            std::cout << "name: " << name << ", vsd: " << vsd << std::endl;

        return vsds;
    }

    // Почти всё можно сделать DEBUG LVL. Главное - сделать тест и чтобы он срабатывал
    void update_xml() {
        const string vsd_path = string("/mnt/hgfs/shared_folder/") + string("vsd.csv");

        const auto& vsd_per_names = get_vsds(vsd_path);
        if(vsd_per_names.empty()) {
            cout << "ВСД не найдены" << endl;
            throw std::logic_error("error");
            close();
        }

        // XML open
        pugi::xml_document doc;
        if (doc.load_file("positions.xml")) {
            cout << "Удалось загрузить XML документ" << endl;
        } else {
            cout << "Не удалось загрузить XML документ" << endl;
            throw std::logic_error("error");
            close();
        }

        pugi::xml_node positions_xml = doc.child("resources").child("input");

        for (auto const& [name, vsd] : vsd_per_names) {
            std::cout << "  vsd name: " << name << std::endl;
            for (pugi::xml_node position_xml: positions_xml.children("position")) {
                std::string name_in_xml = position_xml.attribute("name_english").as_string();
                std::cout << "  name_in_xml: " << name_in_xml << std::endl;
                if(name == name_in_xml) {
                    position_xml.attribute("vsd").set_value(vsd.c_str());
                    cout << "name: " << name << endl;
                    cout << "set vsd to xml: " << vsd << endl;
                }
            }
        }

        if(not doc.save_file("positions.xml")) {
            cout << "Не удалось обновить XML документ" << endl;
            throw std::logic_error("error");
        } else {
            cout << "Удалось обновить XML документ" << endl;
        }
    }

private:
    Ui::MainWindow *ui;
    QTcpServer * mTcpServer;
    QTcpSocket * mTcpSocket;

    int current = 0;
};
