#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>
#include <fstream>

#include <regex>

#include <QBitArray>
#include <QFileDialog>

using namespace std;

#include "config.h"

#ifdef COMPILER_IS_GNU
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

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

    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &MainWindow::on_new_connection);
    connect(ui->save_vsd_button, &QPushButton::clicked, this, &MainWindow::update_xml_with_vsds_from_table);

    if(not mTcpServer->listen(QHostAddress::Any, 6000))
        cout << "tcp_server status=not_started " << endl;
    else
        cout << "tcp_server status=started " << endl;

    QStringList headers;
    headers << trUtf8("Продукция")
            << trUtf8("ВСД");

    ui->table->show();
    ui->table->setColumnCount(2); // Указываем число колонок
    ui->table->setShowGrid(true); // Включаем сетку
    // Разрешаем выделение только одного элемента
    ui->table->setSelectionMode(QAbstractItemView::SingleSelection);
    // Разрешаем выделение построчно
    ui->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Устанавливаем заголовки колонок
    ui->table->setHorizontalHeaderLabels(headers);
    // Растягиваем последнюю колонку на всё доступное пространство
    ui->table->horizontalHeader()->setStretchLastSection(true);
    // Скрываем колонку под номером 0
    //ui->tableWidget->hideColumn(0);

    auto vsd_per_names = fill_table();

    QStringList items;
    for(auto const& [name, vsd]: vsd_per_names) {
        items.append(QString::fromStdString(name));
        product_names.push_back(name);
    }

    ui->product_name_combobox->addItems(items);
}

map<std::string, std::string>  MainWindow::fill_table() {
    std::map<std::string, std::string> vsds;

    fs::current_path(save_folder);

    pugi::xml_document doc;
    if (!doc.load_file("positions.xml")) {
        cout << "Load XML positions.xml FAILED" << endl;
        return {};
    } else {
        cout << "Load XML positions.xml SUCCESS" << endl;
    }

    pugi::xml_node inn_xml = doc.child("resources").child("inn");
    pugi::xml_node version = doc.child("resources").child("version");

    pugi::xml_node positions_xml = doc.child("resources").child("input");

    if (positions_xml.children("position").begin() == positions_xml.children("position").end()) {
        cout << "Positions in positions.xml absent ERROR" << endl;
        return {};
    }

    int row = 0;
    for (pugi::xml_node position_xml: positions_xml.children("position")) {
        std::string position_name = position_xml.attribute("name_english").as_string();
        std::string vsd = position_xml.attribute("vsd").as_string();

        ui->table->insertRow(row);
        ui->table->setItem(row,0, new QTableWidgetItem(QString::fromStdString(position_name)));
        ui->table->setItem(row,1, new QTableWidgetItem(QString::fromStdString(vsd)));

        vsds.emplace(position_name,vsd);
    }

    ui->table->resizeColumnsToContents();

    return vsds;
}

void MainWindow::on_new_connection() {
    QTcpSocket * new_socket = mTcpServer->nextPendingConnection();
    qDebug() << "on_new_connection " << new_socket;
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
    qDebug() << __PRETTY_FUNCTION__ << "================";
    QTcpSocket* socket = qobject_cast< QTcpSocket* >(sender());

    qDebug() << "bytes_available=" << socket->bytesAvailable();

    QByteArray array;
    QByteArray array_header;

    while(socket->bytesAvailable() < 4)
        if (!socket->waitForReadyRead(10))
            qDebug() << "waiting header bytes timed out INFO";

    array_header = socket->read(4);
    qDebug() << "array_header=" << array_header;

    uint32_t bytes_to_read;
    QDataStream ds_header(array_header);
    ds_header >> bytes_to_read;

    qDebug() << "bytes_to_read = " << bytes_to_read;

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
    uint8_t task_number;

    received_bytes >> msg_type;
    received_bytes >> line_number;
    received_bytes >> task_number;

    uint32_t body_size = array.size() - 3;

    qDebug() << "msg_type = " << msg_type << " line_number = " << line_number << " task_number = " << task_number << " INFO";

    qDebug() << "receive method socket " << socket;
    qDebug() << "receive method socket " << &*socket;
    auto& connector = create_connector_if_needed(line_number, socket);
    qDebug() << "connector " << &connector;
    bool res = connector.socket == nullptr;
    qDebug() << "here " << res ;

    switch(msg_type) {
        case 1: // Info request
        {
            qDebug() << "Info request--------";
            vector<string> tasks_info;

            for(auto& task: tasks) {
                if(task.line_number == line_number) {
                    auto number = task.number();
                    auto name = task.product_name();
                    auto plan = task.plan();
                    auto info = QString("%1,%2,%3;").arg(number).arg(name).arg(plan).toStdString();
                    task.set_status(TaskStatus::STARTED);
                    tasks_info.push_back(info);
                }
            }

            connector.send_tasks(tasks_info);

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
            qDebug() << "Scan receive--------";
            QByteArray buffer(body_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), body_size);
            QString scan_number(buffer);

            bool found_task = false;

            for(auto& task: tasks) {
                if(task.line_number == line_number && task.task_number == task_number) {
                    task.set_current(scan_number);
                    found_task = true;
                }
            }
            if(not found_task)
                qDebug() << "Task doesn't exist! ERROR" << msg_type;
        }
        break;
        case 3: // End receive
        {
            qDebug() << "File receive--------";
            QByteArray buffer(body_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), body_size);
            QString scans(buffer);

            qDebug() << "file save...";
            qDebug() << scans;
            auto pos = scans.lastIndexOf(QChar('\n'));
            if(pos != -1)
                scans.truncate(pos);

            time_t rawtime;
            struct tm * timeinfo;
            char date_buffer[80];
            char time_buffer [80];

            time (&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(date_buffer, sizeof(date_buffer), "%d-%m", timeinfo);
            strftime(time_buffer, sizeof(time_buffer), "%H-%M", timeinfo);

            std::string date(date_buffer);
            std::string time_ts(time_buffer);
            qDebug() << "date=" << date.c_str();
            qDebug() << "time_ts=" << time_ts.c_str();

            string working_dir = save_folder + "ki\\" + std::string(date_buffer);
            qDebug() << "working_dir=" << working_dir.c_str();
            fs::create_directories(working_dir);
            fs::current_path(working_dir);
            string scans_number = to_string(scans.count(QChar('\n')) + 1);
            qDebug() << "scans_number=" << scans_number.c_str();
            qDebug() << "fs::current_path=" << fs::current_path().c_str();

            QString product_name;
            //bool found_task = false;
            //
            //for(auto& task: tasks) {
            //    if(task.line_number == line_number && task.task_number == task_number) {
            //        product_name = task.product_name();
            //        task.set_status(TaskStatus::FINISHED);
            //        found_task = true;
            //        break;
            //    }
            //}
            //if(not found_task) {
            //    qDebug() << "Task doesn't exist! ERROR" << msg_type;
            //    return;
            //}
            //auto& task = get_task(line_number, line_number);
            //auto [exist, task] = get_task(line_number, line_number);

//            if(not exist) {
//                qDebug() << "task doesn't exist! ERROR";
//                return;
//            }

            for(auto& task: tasks) {
                if(task.line_number == line_number) {
                    if(task.number() == task_number) {
                        std::string filename = task.product_name().toStdString() + "_" + scans_number + "_" + time_ts + ".csv";
                        qDebug() << "filename=" << filename.c_str();

                        std::ofstream out(filename);
                        out << scans.toStdString();
                        out.close();

                        qDebug() << "file save done";
                        task.set_status(TaskStatus::FINISHED);
                        qDebug() << "111";


                    }
                }
            }








//            for(auto& task: tasks) {
//                qDebug() << "task.number() " << task.number();
//                qDebug() << "task.line_number " << task.line_number;
//                qDebug() << "task.product_name() " << task.product_name();
//            }

//            qDebug() << "look for line_number: " << line_number << " task: " << task_number;
//            auto task_itr = std::find_if(begin(tasks), end(tasks),[&](auto task) {
//                qDebug() << "task.line_number: " << task.line_number << " task.task_number: " << task.task_number;
//                return (line_number == task.line_number) && (task.task_number == task_number);});

//            if(task_itr == end(tasks)) {
//                qDebug() << "Task didnt found line_number: " << line_number << " task: " << task_number;
//                return;
//            }

//            std::string filename = task_itr->product_name().toStdString() + "_" + scans_number + "_" + time_ts + ".txt";
//            qDebug() << "filename=" << filename.c_str();

//            std::ofstream out(filename);
//            out << scans.toStdString();
//            out.close();

//            qDebug() << "file save done";
//            task_itr->set_status(TaskStatus::FINISHED);
//            qDebug() << "111";













//            auto it = descriptor_itr->tasks.find(task_number);
//            if (it != descriptor_itr->tasks.end()) {
//                delete it->second;
//                it->tasks.erase(it);
//            }

//            qDebug() << "task finished";
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

    if(ki_name.isEmpty())
        return;

    //------------------------Download config-----------------------------
    Position position;

    cout << "fs::current_path(): " << fs::current_path() << endl;
    for (const auto & entry : fs::directory_iterator(fs::current_path()))
        std::cout << entry.path() << std::endl;

    fs::current_path(save_folder);

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
    std::string d_m_pattern = std::string("%d-%m");
    std::string time_pattern = std::string("%H-%M");
    char date_buffer [80];
    char d_m_buffer [80];
    char time_buffer [80];

    time_t t = time(0);
    struct tm * now = localtime( & t );
    strftime (date_buffer,80,date_pattern.c_str(),now);
    strftime (time_buffer,80,time_pattern.c_str(),now);
    strftime (d_m_buffer,80,d_m_pattern.c_str(),now);

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

    string filename = product_name + "_" + to_string(sTotal) + "_" + std::string(time_buffer) + ".csv";

    string working_dir = save_folder + "/input/" + std::string(d_m_buffer);
    fs::create_directories(working_dir);
    fs::current_path(working_dir);
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
    auto name = senderButton->objectName();

    uint8_t line_number = name.mid(0,name.indexOf(';')).toUInt();

    auto connector_itr = std::find_if(begin(connectors), end(connectors),
                                    [&line_number](auto connector) { return line_number == connector.line_number;});

    if(connector_itr == end(connectors)) {
        qDebug() << "Connection wasn't established ERROR";
        return;
    }

    connector_itr->send_ready();

    for(auto& task : tasks) {
        if(task.line_number == line_number)
            task.start_button()->setStyleSheet("QPushButton{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: green;}");
    }
}

void MainWindow::on_add_line_pushbutton_clicked() {
    auto line_number = ui->line_number_choose_combobox->currentText().toInt();

    auto& inserted_task = tasks.emplace_back(line_number, ++task_counter, product_names); // implicitly uint -> uint8_t for line_number

    //inserted_task_it->add_task();
    connect(inserted_task.start_button(), &QPushButton::clicked, this, &MainWindow::send_ready_to_slave);

    ui->lines_layout->addLayout(inserted_task.layout());
}

map<string, string> MainWindow::get_vsds() {
    map<string, string> vsds;

    for (int i=0; i< ui->table->rowCount(); i++) {
        string name;
        string vsd;

        for (int j=0; j < ui->table->columnCount(); j++) {
            QTableWidgetItem *item =  ui->table->item(i,j);
            //item->setFlags(Qt::NoItemFlags);
            if(j == 0)
                name.assign(item->text().toStdString());
            else if(j == 1)
                vsd.assign(item->text().toStdString());
        }

        vsds[name] = vsd;
    }

    return vsds;
}

void MainWindow::update_xml_with_vsds_from_table() {
    const auto& vsd_per_names = get_vsds();
    if(vsd_per_names.empty()) {
        cout << "VSD not found ERROR" << endl;
        throw std::logic_error("error");
        close(); // не помню, что за close()
    }

    fs::current_path(save_folder);

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
