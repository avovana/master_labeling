#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>
#include <fstream>
#include <regex>

#include <QBitArray>
#include <QFileDialog>

using namespace std;

ostream& operator<<( ostream&  out, Position& pos ) {
    out << "code_tn_ved: " << pos.code_tn_ved << " " << endl
        << "document_type: " << pos.document_type << " " << endl
        << "document_number: " << pos.document_number << " " << endl
        << "document_date: " << pos.document_date << " " << endl
        << "vsd: " << pos.vsd << " " << endl
        << "name: " << pos.name << " " << endl
        << "inn:" << pos.inn << endl
        << "expected: " << pos.expected << endl
        << "current: " << pos.current << endl;

    return out;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    pugi::xml_document doc;
    if (!doc.load_file("vars.xml")) {
        cout << "Load vars XML FAILED" << endl;
        return;
    } else {
        cout << "Load vars XML SUCCESS" << endl;
    }

    pugi::xml_node save_folder_child = doc.child("vars").child("save_folder");
    save_folder = save_folder_child.text().get();
    cout << "save_folder: " << save_folder << endl;

    // Прочитать vsd
    update_xml();

    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &MainWindow::on_new_connection);

    if(not mTcpServer->listen(QHostAddress::Any, 6000))
        cout << "tcp_server status=not_started " << endl;
    else
        cout << "tcp_server status=started " << endl;
}


void MainWindow::on_new_connection() {
    QTcpSocket * new_socket = mTcpServer->nextPendingConnection();

    sockets.push_back(new_socket);
    qDebug() << "Local:";
    qDebug() << new_socket->localAddress();
    qDebug() << new_socket->localPort();
    qDebug() << "Peer:";
    qDebug() << new_socket->peerAddress();
    qDebug() << new_socket->peerPort();
    /* QByteArray ba;
      ba.resize(4);
      ba[0] = 0x04;
      ba[1] = 0x00;
      ba[2] = 0x00;
      ba[3] = 0x0;
    mTcpSocket->write(ba);
    -> client:
    Received! Yeah!
    header_data_str:
    header_data:  b'\x04\x00\x00\x00'
    Я внутри!
    msg_len:  4
    */

    /* mTcpSocket->write("4abc\r\n");
    -> client:
    Received! Yeah!
    header_data_str:  4abc
    header_data:  b'4abc'
    Я внутри!
    msg_len:  1667391796
    */

    /* mTcpSocket->write("4");
    -> client:
    Received! Yeah!
    */

    /* mTcpSocket->write("4000");
    -> client:
    Received! Yeah!
    header_data_str:  4000
    header_data:  b'4000'
    Я внутри!
    msg_len:  808464436

    т.е. пришло 4 байта. Ок. В этих байтах...
    */

    /*
    QByteArray ba;
          ba.resize(4);
          ba[0] = 0x04;
          ba[1] = 0x00;
          ba[2] = 0x00;
          ba[3] = 0x0;
        mTcpSocket->write(ba);
        mTcpSocket->write("1234");
    */
    connect(new_socket, &QTcpSocket::readyRead, this, &MainWindow::on_server_read);
    connect(new_socket, &QTcpSocket::disconnected, this, &MainWindow::on_client_disconnected);
}


void MainWindow::on_server_read() {
    QTcpSocket* socket = qobject_cast< QTcpSocket* >(sender());

    qDebug() << "bytes_available=" << socket->bytesAvailable();

    QByteArray array;
    QByteArray array_header;

    while(socket->bytesAvailable() < 4)
        if (!socket->waitForReadyRead(10))
            qDebug() << "waiting header bytes timed out INFO";

    array_header = socket->read(4);
    qDebug() << "array_header=" << array_header;

    uint32_t header;
    QDataStream ds_header(array_header);
    ds_header >> header;

    qDebug() << "header=" << header;
    uint32_t bytes_to_read = header;
    uint32_t data_size = header - 2;

    while(bytes_to_read > 0) {
        if (!socket->waitForReadyRead(100))
            qDebug() << "waiting bytes timed out INFO";

        QByteArray read_bytes_chunk = socket->read(bytes_to_read);
        qDebug() << "read_bytes_chunk=" << read_bytes_chunk.size();

        array.append(read_bytes_chunk);
        bytes_to_read -= read_bytes_chunk.size();

        qDebug() << "bytes_to_read=" << bytes_to_read;
    }

    QDataStream received_bytes(array);

    uint8_t msg_type;
    uint8_t line_number;

    received_bytes >> msg_type;
    received_bytes >> line_number;

    qDebug() << "msg_type=" << msg_type << " line_number=" << line_number << " INFO";

    auto descriptor_itr = std::find_if(begin(lines_descriptors), end(lines_descriptors),
                                    [&line_number](auto descriptor) { return line_number == descriptor.line_number; });

    bool not_exist_description = (descriptor_itr == end(lines_descriptors));

    switch(msg_type) {
        case 1: // Info request
        {
            if(not_exist_description) {
                qDebug() << "Line doesn't exist! ERROR" << msg_type;

                QByteArray out_array;
                QDataStream stream(&out_array, QIODevice::WriteOnly);
                unsigned int type = 8;
                unsigned int out_msg_size = sizeof(type);
                stream << out_msg_size;
                stream << type;

                qDebug() << "out_array_size=" << out_array.size();

                socket->write(out_array);

                return;
            }

            descriptor_itr->socket = socket;

            descriptor_itr->line_status_checkbox->setChecked(true);
            descriptor_itr->line_status_checkbox->setEnabled(false);

            auto palette = QPalette();
            palette.setColor(QPalette().Base, QColor("#23F617"));
            descriptor_itr->line_status_checkbox->setPalette(palette);

            uint plan = descriptor_itr->plan_lineedit->text().toInt();
            QString name = descriptor_itr->product_name_lineedit->text();
            auto msg = QString("%1,%2").arg(name).arg(plan).toStdString();
            qDebug() << "msg_size=" << msg.size();

            QByteArray out_array;
            QDataStream stream(&out_array, QIODevice::WriteOnly);
            unsigned int out_msg_size = msg.size() + sizeof(unsigned int);
            unsigned int type = 4;
            stream << out_msg_size;
            stream << type;
            stream.writeRawData(msg.c_str(), msg.size());

            for(int i = 0; i < out_array.count(); ++i) {
              qDebug() << QString::number(out_array[i], 16);
            }

            qDebug() << "out_array=" << out_array;
            for(int i = 0; i < out_array.count(); ++i) {
              qDebug() << out_array[i];
            }
            qDebug() << "out_array_size=" << out_array.size();

            descriptor_itr->socket->write(out_array);
        /*
        switch(line_number) {
            case 1:
            {


            //QByteArray header;
            //header.resize(4);
            //header[0] = 0x01;
            //header[1] = 0x00;
            //header[2] = 0x00;
            //header[3] = 0x00;

            //QByteArray out_array;
            //QDataStream out_stream(out_array);
            //out_stream << 5;
            //unsigned int ll = 1234;
            //out_array.setNum(ll);

            //QString msg;
            //msg.append("Sir Kavkazkiu").append(",").append(QString::number(plan));
            //qDebug() << "msg" << msg;
            //qDebug() << "msg.size(): " << msg.size();
            //qDebug() << "msg.count(): " << msg.count();
            //mTcpSocket->write(msg.toStdString().c_str(), msg.count());



            //QByteArray out_msg_buf;
            //QDataStream outStream(&out_msg_buf, QIODevice::WriteOnly);
            //outStream<<msg;
            //
            //qDebug() << "msg.size(): " << msg.size();
            //qDebug() << "out_msg_buf: " << out_msg_buf.size();
            //for(int i = 0; i < out_msg_buf.count(); ++i) {
            //  qDebug() << "\ x" << QString::number(out_msg_buf[i], 16);
            //}
            //
            //qDebug() << "============";
        //    mTcpSocket->write(out_msg_buf);


            //mTcpSocket->write(msg.c_str(), msg.size());


            //mTcpSocket->write("1"); // 1 = OK

        //    std::cout << "DataAsString.toStdString(): " << str;
        //    std::cout << "DataAsString.toStdString().size(): " << str.size();
        //    //ds.setByteOrder(QDataStream::LittleEndian);
        //    uint32_t header1; // Since the size you're trying to read appears to be 2 bytes
        //    uint8_t msg_type;
        //    uint8_t line_number;
        //    //std::string str;
        //    ds >> header1;
        //    ds >> msg_type;
        //    ds >> line_number;
        //
        //    qDebug() << "header1: " << header1;
        //    qDebug() << "msg_type: " << msg_type;
        //    qDebug() << "line_number: " << line_number;
        //
        //    qDebug() << "array.mid(0,4): " << array.mid(0,4);
        //    qDebug() << "array.mid(4,1): " << array.mid(4,1);
        //    qDebug() << "array.mid(5,1): " << array.mid(5,1);
        //    qDebug() << "array.mid(6,array.end()): " << array.mid(6,array.size() - 1);
        //    uint32_t header = array.mid(0,4).toUInt();
        //    qDebug() << "array.mid(0,4).toUInt(): " << array.mid(0,4).toUInt();
        //    qDebug() << "array.mid(0,4).toULong(): " << array.mid(0,4).toULong();
        //    qDebug() << "sizeof(uint): " << sizeof(uint);
        //    qDebug() << "sizeof(ulong): " << sizeof(ulong);
        //
        //
            //QByteArray header;
            //header.resize(4);
            //header[0] = 0x01;
            //header[1] = 0x00;
            //header[2] = 0x00;
            //header[3] = 0x00;
            //
            //mTcpSocket->write(header);
            //
            //mTcpSocket->write("1"); // 1 = OK
            //
            //qDebug() << "slotServerRead end";


            }
            break;*/
        }
        break;

        case 2: // Scan receive
        {
            if(not_exist_description) {
                qDebug() << "Scan receive Line doesn't exist! ERROR" << msg_type;
                return;
            }
            /*
            QByteArray buffer(data_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), data_size);
            QString scan(buffer);

            qDebug() << "    scan_size=" << scan.size() << " scan=" << scan;
            std::cout << "std scan_scan_size=" << scan.toStdString().size() << " scan=" << scan.toStdString();

            std::ofstream out("scans_current_" + descriptor_itr->product_name_lineedit->text().toStdString() + ".txt", std::ios_base::app);
            out << scan.toStdString() << endl;
            out.close();

            descriptor_itr->current_label->setText(QString::number(++current));
            */
            QByteArray buffer(data_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), data_size);
            QString scan_number(buffer);
            descriptor_itr->current_label->setText(scan_number);
        }
        break;
        case 3:
        {
            if(not_exist_description) {
                qDebug() << "File receive Line doesn't exist! ERROR" << msg_type;
                return;
            }

            QByteArray buffer(data_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), data_size);
            QString scan(buffer);

            qDebug() << "file save...";
            qDebug() << scan;
            auto pos = scan.lastIndexOf(QChar('\n'));
            if(pos != -1)
                scan.truncate(pos);

            time_t rawtime;
            struct tm * timeinfo;
            char bufferDate[80];

            time (&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(bufferDate, sizeof(bufferDate), "%d-%m-%Y", timeinfo);
            std::string date(bufferDate);

            qDebug() << "date=" << date.c_str();

            std::string filename = save_folder + "input\\" + date + "_ki_line_number_" + to_string(line_number) + ".txt";

            std::ofstream out(filename);
            out << scan.toStdString();
            out.close();

            auto palette = QPalette();
            palette.setColor(QPalette().Base, QColor("#23F617"));
            descriptor_itr->line_status_finish_checkbox->setPalette(palette);

            qDebug() << "file save done";
        }
        break;
    }

}

void MainWindow::on_client_disconnected() {
    QTcpSocket* socket = qobject_cast< QTcpSocket* >(sender());
    socket->close();
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::on_make_template_pushbutton_clicked() {
    //------------------------File choose-----------------------------
    QString ki_name = QFileDialog::getOpenFileName(this, "Ki", QString::fromStdString(save_folder));
    qDebug() << "Filename ki: " << ki_name;
    qDebug() << "product_name: " << ui->product_name_combobox->currentText();
    string product_name = ui->product_name_combobox->currentText().toStdString();

    //------------------------Download config-----------------------------
    Position position;

    pugi::xml_document doc;
    if (!doc.load_file("positions.xml")) {
        cout << "Load XML positions.xml FAILED" << endl;
        return;
    } else {
        cout << "Load XML positions.xml SUCCESS" << endl;
    }

    pugi::xml_node inn_xml = doc.child("resources").child("inn");
    pugi::xml_node version = doc.child("resources").child("version");

    pugi::xml_node positions_xml = doc.child("resources").child("input");

    if (positions_xml.children("position").begin() == positions_xml.children("position").end()) {
        cout << "Positions in positions.xml absent ERROR" << endl;
        return;
    }

    for (pugi::xml_node position_xml: positions_xml.children("position")) {
        std::string position_name = position_xml.attribute("name_english").as_string();
        std::cout << "position_name=" << position_name << endl;

        if(product_name == position_name) {
            position = Position{position_xml.attribute("code_tn_ved").as_string(),
                                         position_xml.attribute("document_type").as_string(),
                                         position_xml.attribute("document_number").as_string(),
                                         position_xml.attribute("document_date").as_string(),
                                         position_xml.attribute("vsd").as_string(),
                                         position_name,
                                         inn_xml.text().get(),
                                         position_xml.attribute("name_english").as_string(),
                                         position_xml.attribute("expected").as_int(),
                                         position_xml.attribute("current").as_int()};

            std::cout << "position=" << endl << position << endl;
        }
    }
    //------------------------Make file-----------------------------

    std::string date_pattern = std::string("%Y-%m-%d");
    char date_buffer [80];

    time_t t = time(0);
    struct tm * now = localtime( & t );
    strftime (date_buffer,80,date_pattern.c_str(),now);

    // Подсчитать кол-во сканов
    string s;
    int sTotal = 0;

    ifstream in;
    in.open(ki_name.toStdString());

    while(!in.eof()) {
        getline(in, s);
        sTotal ++;
    }

    cout << "sTotal: " << sTotal << endl;
    in.close();
    // Создать файл

    string filename = std::string(date_buffer) + " " + product_name + " " + to_string(sTotal)+ ".csv";

    std::ofstream template_file;

    template_file.open(filename, std::ios_base::app);
    if(not template_file.is_open()) {
        std::cout<<"Ошибка создания файла шаблона"<<std::endl;
        return;
    }
    //------------------------Header-----------------------------

    template_file << "ИНН участника оборота,ИНН производителя,ИНН собственника,Дата производства,Тип производственного заказа,Версия" << endl;
    template_file << position.inn << "," << position.inn << "," << position.inn << "," << date_buffer << ",Собственное производство,4" << endl;
    template_file << "КИ,КИТУ,Дата производства,Код ТН ВЭД ЕАС товара,Вид документа подтверждающего соответствие,Номер документа подтверждающего соответствие,Дата документа подтверждающего соответствие,Идентификатор ВСД" << endl;

    //------------------------Scans-----------------------------



    //Открыть ки файл. Считывать строку за строкой. Дописывать в результирующий файл

    std::ifstream infile;

    infile.open(ki_name.toStdString(), std::ios_base::app);
    if(not infile.is_open()) {
        std::cout<<"Ошибка открытия ки файла"<<std::endl;
        return;
    }

    //auto lines = std::count(std::istreambuf_iterator<char>(infile),
    //             std::istreambuf_iterator<char>(), '\n');
    //
    //cout << "lines: " << lines << endl;

    std::string scan;

    while (std::getline(infile, scan)) {
        char gs = 29;
        auto pos = scan.find(gs);

        scan = scan.substr(0,pos);

        scan = std::regex_replace(scan, std::regex("\""), "\"\"");
        scan.insert(0, 1, '"');
        scan.push_back('"');

        template_file << scan << ","
           << ","
           << date_buffer << ","
           << position.code_tn_ved << ","
           << position.document_type << ","
           << position.document_number << ","
           << position.document_date << ","
           << position.vsd;

        if(--sTotal > 0)
            template_file << endl;
    }
}

void MainWindow::send_ready_to_slave() {
    QPushButton * senderButton = qobject_cast<QPushButton *>(this->sender());
    senderButton->setStyleSheet("background-color: green");

    QByteArray out_array;
    QDataStream stream(&out_array, QIODevice::WriteOnly);
    unsigned int out_msg_size = sizeof(unsigned int);
    unsigned int type = 6;
    stream << out_msg_size;
    stream << type;

    qDebug() << " on pushbutton_clicked size: " << out_array.size();

    auto descriptor_itr = std::find_if(begin(lines_descriptors), end(lines_descriptors),
                                    [senderButton](auto descriptor) { return senderButton == descriptor.line_start_pushbutton; });

    if(descriptor_itr == end(lines_descriptors)) {
        qDebug() << "Line doesn't exist! ERROR";
        return;
    }

    qDebug() << "send to line=" << descriptor_itr->line_number;

    if(descriptor_itr->socket == nullptr) {
        qDebug() << "Connection wasn't established ERROR";
        return;
    }

    descriptor_itr->socket->write(out_array);
}

void MainWindow::on_add_line_pushbutton_clicked() {
    lines++;

    lines_descriptors.emplace_back(ui, lines);
    connect(lines_descriptors.back().line_start_pushbutton, &QPushButton::clicked, this, &MainWindow::send_ready_to_slave);
}

MainWindow::LineDescriptor::LineDescriptor(Ui::MainWindow *ui_, uint line_number_) : ui(ui_), line_number(line_number_) {
    line_number_label = new QLabel();
    line_number_label->setStyleSheet("QLabel{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
    line_number_label->setText(QString::number(line_number_));
    line_number_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    line_number_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    line_status_checkbox = new QCheckBox();
    line_status_checkbox->setStyleSheet("QCheckBox{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);}");
    line_status_checkbox->setEnabled(false);
    line_status_checkbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    line_status_checkbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    product_name_lineedit = new QLineEdit();
    product_name_lineedit->setText("maslo");
    product_name_lineedit->setStyleSheet("QLineEdit{font-size: 24px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
    product_name_lineedit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    plan_lineedit = new QLineEdit();
    plan_lineedit->setStyleSheet("QLineEdit{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
    plan_lineedit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    current_label = new QLineEdit();
    current_label->setStyleSheet("QLineEdit{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
    current_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    line_start_pushbutton = new QPushButton();
    line_start_pushbutton->setText("Старт");
    line_start_pushbutton->setStyleSheet("QPushButton{font-size: 14px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(141, 255, 255);}");
    line_start_pushbutton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    line_status_finish_checkbox = new QCheckBox();
    line_status_finish_checkbox->setEnabled(false);
    line_status_finish_checkbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    socket = nullptr;

    add_to_ui();
}

void MainWindow::LineDescriptor::add_to_ui() {
    static int line_position = 2;
    line_position++;
    ui->gridLayout->addWidget(line_number_label, line_position, 0);
    ui->gridLayout->addWidget(line_status_checkbox, line_position, 1);
    ui->gridLayout->addWidget(product_name_lineedit, line_position, 3);
    ui->gridLayout->addWidget(plan_lineedit, line_position, 5);
    ui->gridLayout->addWidget(current_label, line_position, 6);
    ui->gridLayout->addWidget(line_start_pushbutton, line_position, 7);
    ui->gridLayout->addWidget(line_status_finish_checkbox, line_position, 8);
}

std::map<std::string, std::string> MainWindow::get_vsds(const std::string & vsd_path) {
    std::ifstream vsd;

    std::cout<<"vsd_path: " << vsd_path <<std::endl;

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
        ui->product_name_combobox->addItem(QString::fromStdString(name));
    }

    return vsds;
}

void MainWindow::update_xml() {
    const string vsd_path = string(save_folder + "\\vsd.csv");

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
