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
    mTcpSocket = mTcpServer->nextPendingConnection();

    qDebug() << "Local:";
    qDebug() << mTcpSocket->localAddress();
    qDebug() << mTcpSocket->localPort();
    qDebug() << "Peer:";
    qDebug() << mTcpSocket->peerAddress();
    qDebug() << mTcpSocket->peerPort();
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
    connect(mTcpSocket, &QTcpSocket::readyRead, this, &MainWindow::on_server_read);
    connect(mTcpSocket, &QTcpSocket::disconnected, this, &MainWindow::on_client_disconnected);
}


void MainWindow::on_server_read() {
    qDebug() << "slotServerRead start";
    qDebug() << "mTcpSocket->bytesAvailable(): " << mTcpSocket->bytesAvailable();

    QByteArray array;
    QBitArray bitarray;
    while(mTcpSocket->bytesAvailable()>0) // считаю, что где-то вглубине сообщение, которое могло бы быть разбито собирается, и здесь сигнал выставляется, когда оно уже полное
    {
        //QByteArray array = mTcpSocket->readAll();

        //qDebug() << "array: " << array;
        //qDebug() << "array.size(): " << array.size();
        //std::cout << "array.toStdString: " << array.toStdString();
        //qDebug() << "array.toStdString.size(): " << array.toStdString().size();
        //mTcpSocket->write(array);
        array = mTcpSocket->readAll();
//        qDebug() << "array: " << array;
        qDebug() << "array: " << bitarray ;
    }

    for(int i = 0; i < array.count(); ++i) {
      qDebug() << QString::number(array[i], 16);
    }

    //Сработало норм:
    //QString DataAsString = QString(array);
    //qDebug() << "str: " << DataAsString;
    //qDebug() << "str.size(): " << DataAsString.size();

    //auto scans = DataAsString.split('\n');
    //qDebug() << "scans: " << scans;

    //std::ofstream out("output.txt");
    //for ( const auto& scan : scans ) {
    //qDebug() << "   scan:   " << scan;
    //    out << scan.toStdString();
    //}
    //out.close();

   // QDataStream ds(array);
   // QString str;
   // ds >> str;
   // qDebug() << "str: " << str;
   // Считать в большую строку. Пробегаться по строке до конца встречая \n - деля на под строки и записывая в файл

    QDataStream ds(array);

    uint32_t header;
    uint8_t msg_type;
    uint8_t line_number;

    ds >> header;
    ds >> msg_type;
    ds >> line_number;

    qDebug() << "header: " << header;
    qDebug() << "msg_type: " << msg_type;
    qDebug() << "line_number: " << line_number;
    qDebug() << "============";

    switch(msg_type) {
        case 1:
        {
        switch(line_number) {
            case 1:
            {
            ui->line_1_status_checkbox->setChecked(true);
            ui->line_1_status_checkbox->setEnabled(false);

            auto palette = QPalette();
            palette.setColor(QPalette().Base, QColor("#23F617"));
            ui->line_1_status_checkbox->setPalette(palette);
            /*
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
            */
            uint plan = 10;
            QString name = "Sir Kavkazkiu";
            auto msg = QString("%1,%2").arg(name).arg(plan).toStdString();
            qDebug() << "msg.size(): " << msg.size();
            /*
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
        //    mTcpSocket->write(out_msg_buf);*/

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

            qDebug() << "out_array: " << out_array;
            for(int i = 0; i < out_array.count(); ++i) {
              qDebug() << out_array[i];
            }
            qDebug() << "out_array.size(): " << out_array.size();

            mTcpSocket->write(out_array);

            //mTcpSocket->write(msg.c_str(), msg.size());

        /*
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
            */

            }
            break;
        }
        }
        break;

        case 2:
        {
            QByteArray buffer(header - 2, Qt::Uninitialized);

            ds.readRawData(buffer.data(), header - 2);
            QString scan(buffer);

            qDebug() << "scan: " << scan;
            qDebug() << "scan.size(): " << scan.size();
            std::cout << "scan.toStdString(): " << scan.toStdString();
            qDebug() << "scan.toStdString(): " << scan.toStdString().size();

            std::ofstream out("scans_.txt");
            out << scan.toStdString();
            out.close();

            ui->current_1_label->setText(QString::number(++current));
        }
        break;
        case 3:
        {
            QByteArray buffer(header - 2, Qt::Uninitialized);

            ds.readRawData(buffer.data(), header - 2);
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

            qDebug() << "date: " << date.c_str();

            std::ofstream out(date + "_ki_line_№" + to_string(line_number) + ".txt");
            out << scan.toStdString();
            out.close();

            auto palette = QPalette();
            palette.setColor(QPalette().Base, QColor("#23F617"));
            ui->line_1_status_finish_checkbox->setPalette(palette);

            qDebug() << "Статус проставлен";
        }
        break;
    }

}

void MainWindow::on_client_disconnected() {
    mTcpSocket->close();
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::on_make_template_pushbutton_clicked() {
    //------------------------File choose-----------------------------
    QString ki_name = QFileDialog::getOpenFileName();
    qDebug() << "Filename: " << ki_name;
    qDebug() << "product_name: " << ui->product_1_name_lineedit->text();
    string product_name = ui->product_1_name_lineedit->text().toStdString();

    //------------------------Download config-----------------------------
    Position position;

    pugi::xml_document doc;
    if (!doc.load_file("positions.xml")) {
        cout << "Не удалось загрузить XML документ" << endl;
        return;
    } else {
        cout << "Удалось загрузить XML документ" << endl;
    }

    pugi::xml_node inn_xml = doc.child("resources").child("inn");
    pugi::xml_node version = doc.child("resources").child("version");

    pugi::xml_node positions_xml = doc.child("resources").child("input");

    if (positions_xml.children("position").begin() == positions_xml.children("position").end()) {
        cout << "ОШИБКА! В документе нет позиций!" << endl;
        return;
    }

    for (pugi::xml_node position_xml: positions_xml.children("position")) {
        std::string position_name = position_xml.attribute("name_english").as_string();
        std::cout << "position_name: " << position_name << endl;

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

            std::cout << "Считана позиция: " << endl << position << endl;
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

void MainWindow::on_line_1_start_pushbutton_clicked() {
    QByteArray out_array;
    QDataStream stream(&out_array, QIODevice::WriteOnly);
    unsigned int out_msg_size = sizeof(unsigned int);
    unsigned int type = 6;
    stream << out_msg_size;
    stream << type;

    qDebug() << " on_line_1_start_pushbutton_clicked out_array.size(): " << out_array.size();

    mTcpSocket->write(out_array);
}
