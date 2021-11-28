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

ostream& operator<<( ostream&  out, TaskStatus& state ) {
    switch (state) {
    case TaskStatus::INIT:
        out << "INIT";
        break;
    case TaskStatus::TASK_SEND:
        out << "TASK_SEND";
        break;
    case TaskStatus::IN_PROGRESS:
        out << "IN_PROGRESS";
        break;
    case TaskStatus::FINISHED:
        out << "FINISHED";
        break;
    }

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

    auto rus_eng_names = fill_table();

    QStringList items;
    for(auto const& [name_rus, name_eng]: rus_eng_names) {
        items.append(QString::fromStdString(name_rus));
        product_names[name_rus] = name_eng;
    }

    ui->product_name_combobox->addItems(items);
}

map<std::string, std::string>  MainWindow::fill_table() {
    std::map<std::string, std::string> rus_eng_names;

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
        std::string name_eng = position_xml.attribute("name_english").as_string();
        std::string name_rus = position_xml.attribute("name").as_string();
        std::string vsd = position_xml.attribute("vsd").as_string();
        qDebug() << position_xml.attribute("name").as_string();
        qDebug() << position_xml.attribute("vsd").as_string();

        ui->table->insertRow(row);
        ui->table->setItem(row,0, new QTableWidgetItem(QString::fromStdString(name_rus)));
        ui->table->setItem(row,1, new QTableWidgetItem(QString::fromStdString(vsd)));

        rus_eng_names.emplace(name_rus, name_eng);
    }

    ui->table->resizeColumnsToContents();

    return rus_eng_names;
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

    connect(new_socket, &QTcpSocket::readyRead, this, &MainWindow::on_server_read);
    connect(new_socket, &QTcpSocket::disconnected, this, &MainWindow::on_client_disconnected);
}

void MainWindow::on_server_read() {
    qDebug() << "===============" << __PRETTY_FUNCTION__ << "================";
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
        case 1:
        {
            qDebug() << "--------Info request--------";
            const auto & tasks_for_line = get_tasks_for_line(line_number);

            std::vector<string> tasks_info;

            for_each(begin(tasks_for_line), end(tasks_for_line),[&](auto & task) {
                if(task->line_number == line_number) {
                    auto number = task->task_number;
                    auto name = task->product_name_eng();
                    auto plan = task->plan();
                    auto info = QString("%1:%2:%3;").arg(number).arg(QString::fromStdString(name)).arg(plan).toStdString();
                    task->set_status(TaskStatus::TASK_SEND);
                    task->state_button()->setText("Задача передана. Разрешить старт");
                    tasks_info.push_back(info);
                }
            });

            connector.send_tasks(tasks_info);
        }
        break;

        case 2:
        {
            qDebug() << "--------Scan receive--------";
            auto task_it = std::find_if(begin(tasks), end(tasks),
                                                    [&](auto &task) { return line_number == task->line_number && task_number == task->task_number;});

            if(task_it == end(tasks)) {
                qDebug() << "Task doesn't exist! ERROR" << msg_type;
                return;
            }

            auto & task = *task_it;

            QByteArray buffer(body_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), body_size);
            QString scan_number(buffer);

            task->set_current(scan_number);
        }
        break;
        case 3:
        {
            qDebug() << "--------File receive--------";

            auto task_it = std::find_if(begin(tasks), end(tasks),
                                                    [&](auto &task) { return line_number == task->line_number && task_number == task->task_number;});

            if(task_it == end(tasks)) {
                qDebug() << "Task doesn't exist! ERROR";
                return;
            }

            auto & task = *task_it;

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

            fs::path work_path = save_folder;
            work_path /= "ki";
            work_path /= std::string(date_buffer);
            qDebug() << "work_path: " << work_path.c_str();
            qDebug() << "work_path: " << QString::fromStdString(work_path.string());
            fs::create_directories(work_path);
            fs::current_path(work_path);
            string scans_number = to_string(scans.count(QChar('\n')) + 1);
            qDebug() << "scans_number=" << scans_number.c_str();
            qDebug() << "fs::current_path=" << fs::current_path().c_str();

            QString product_name;

            std::string filename = task->product_name_rus() + "__" + scans_number + " штук__" + time_ts + ".csv";
            qDebug() << "filename=" << filename.c_str();
            work_path /= filename;
            qDebug() << "work_path: " << work_path.c_str();
            task->file_name = work_path.string();
            std::ofstream out(work_path.string());
            out << scans.toStdString();
            out.close();

            qDebug() << "file save done";
            task->set_status(TaskStatus::FINISHED);
            task->state_button()->setText("Завершено. Сделать шаблон");
            task->state_button()->setStyleSheet("QPushButton{font-size: 25px;font-family: Arial;color: rgb(255, 255, 255);background-color: green;}");
            qDebug() << "111";
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
    cout << "fs::current_path(): " << fs::current_path() << endl;

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

    string filename = product_name + "__" + to_string(sTotal) + "штук__" + std::string(time_buffer) + ".csv";

    fs::path work_path = save_folder;
    work_path /= "input";
    work_path /= std::string(d_m_buffer);
    qDebug() << "work_path: " << work_path.c_str();

    fs::create_directories(work_path);
    fs::current_path(work_path);
    qDebug() << "fs::current_path=" << fs::current_path().c_str();

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

void MainWindow::make_next_action() {
    qDebug() << "===============" << __PRETTY_FUNCTION__ << "===============";
    QPushButton * senderButton = qobject_cast<QPushButton *>(this->sender());
    auto name = senderButton->objectName();

    auto line_task = name.split(';');
    uint8_t line_number = line_task[0].toUInt();
    uint8_t task_number = line_task[1].toUInt();
    qDebug() << "line_number: " << line_number;
    qDebug() << "task_number: " << task_number;

    auto connector_itr = std::find_if(begin(connectors), end(connectors),
                                    [&line_number](auto connector) { return line_number == connector.line_number;});

    qDebug() << "connector_itr: " << &connector_itr;
    if(connector_itr == end(connectors)) {
        qDebug() << "Connection wasn't established ERROR";
        return;
    }

    bool need_to_delete = false;

    auto task_it = std::find_if(begin(tasks), end(tasks),
                                            [&](auto &task) { return line_number == task->line_number && task_number == task->task_number;});

    if(task_it == end(tasks)) {
        qDebug() << "Task doesn't exist! ERROR";
        return;
    }

    auto & task = *task_it;

    switch (task->status()) {
    case TaskStatus::INIT:
        break;
    case TaskStatus::TASK_SEND:
        task->state_button()->setStyleSheet("QPushButton{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: grey;}");
        task->state_button()->setText("В работе");
        connector_itr->send_ready();
        task->set_status(TaskStatus::IN_PROGRESS);
        break;
    case TaskStatus::FINISHED:
        {
        //------------------------File choose-----------------------------
        QString ki_name = QString::fromStdString(task->file_name);
        qDebug() << "Filename ki: " << ki_name;
        qDebug() << "product_name: " << QString::fromStdString(task->product_name_eng());
        string product_name = task->product_name_eng();
        if(ki_name.isEmpty())
            return;

        //------------------------Download config-----------------------------
        Position position;

        cout << "fs::current_path(): " << fs::current_path() << endl;
        for (const auto & entry : fs::directory_iterator(fs::current_path()))
            std::cout << entry.path() << std::endl;

        fs::current_path(save_folder);
        cout << "fs::current_path(): " << fs::current_path() << endl;

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

        string filename = task->product_name_rus() + "__" + to_string(sTotal) + "штук__" + std::string(time_buffer) + ".csv";

        fs::path work_path = save_folder;
        work_path /= "input";
        work_path /= std::string(d_m_buffer);
        qDebug() << "work_path: " << work_path.c_str();

        fs::create_directories(work_path);
        fs::current_path(work_path);
        qDebug() << "fs::current_path=" << fs::current_path().c_str();

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

            need_to_delete = true;
        }
        }
        break;
    default:
        break;
    }

/*
    for(auto& task : tasks) {
        qDebug() << "task.line_number: " << task.line_number;
        if(task.line_number == line_number) {
            //cout << "task.status(): " << task.status();
            switch (task.status()) {
            case TaskStatus::INIT:
                break;
            case TaskStatus::TASK_SEND:
                task.state_button()->setStyleSheet("QPushButton{font-size: 60px;font-family: Arial;color: rgb(255, 255, 255);background-color: grey;}");
                task.state_button()->setText("В работе");
                connector_itr->send_ready();
                task.set_status(TaskStatus::IN_PROGRESS);
                break;
            case TaskStatus::FINISHED:
                {
                //------------------------File choose-----------------------------
//                QString ki_name = QFileDialog::getOpenFileName(this, "Ki", QString::fromStdString(save_folder));
                QString ki_name = QString::fromStdString(task.file_name);
                qDebug() << "Filename ki: " << ki_name;
//                qDebug() << "product_name: " << ui->product_name_combobox->currentText();
//                string product_name = ui->product_name_combobox->currentText().toStdString();
                qDebug() << "product_name: " << QString::fromStdString(task.product_name_eng());
                string product_name = task.product_name_eng();
                if(ki_name.isEmpty())
                    return;

                //------------------------Download config-----------------------------
                Position position;

                cout << "fs::current_path(): " << fs::current_path() << endl;
                for (const auto & entry : fs::directory_iterator(fs::current_path()))
                    std::cout << entry.path() << std::endl;

                fs::current_path(save_folder);
                cout << "fs::current_path(): " << fs::current_path() << endl;

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

//                string filename = task.product_name_rus() + "__" + to_string(sTotal) + "штук__" + std::string(time_buffer) + ".csv";

                fs::path work_path = save_folder;
                work_path /= "input";
                work_path /= std::string(d_m_buffer);
                qDebug() << "work_path: " << work_path.c_str();

                fs::create_directories(work_path);
                fs::current_path(work_path);
                qDebug() << "fs::current_path=" << fs::current_path().c_str();

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

                    need_to_delete = true;
                }
                }
                break;
            default:
                break;
            }
        }
    }
*/
    qDebug() << "tasks.size(): " << tasks.size();
    qDebug() << "need_to_delete: " << need_to_delete;
    if(need_to_delete) {
        qDebug() << "tasks.size(): " << tasks.size();
        auto it = find_if(tasks.begin(), tasks.end(),[&](auto & task) {return task->line_number == line_number;});
        tasks.erase(it);
        qDebug() << "tasks.size(): " << tasks.size();
    }
}

void MainWindow::on_add_line_pushbutton_clicked() {
    auto line_number = ui->line_number_choose_combobox->currentText().toInt();
    ++task_counter;
    auto task = make_shared<TaskInfo>(line_number, task_counter, product_names);

    auto inserted_task = tasks.emplace_back(task); // implicitly uint -> uint8_t for line_number

    connect(inserted_task->state_button(), &QPushButton::clicked, this, &MainWindow::make_next_action);

    ui->lines_layout->addLayout(inserted_task->layout());
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
