#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>
#include <fstream>

#include <regex>

#include <QBitArray>
#include <QFileDialog>
#include <QRegExpValidator>

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

    ui->date_line_edit->setValidator(new QRegExpValidator(QRegExp("[0-3]{1}[0-9]{1}-[0-1]{1}[1-9]{1}")));
//    ui->date_make_template_line_edit->setValidator(new QRegExpValidator(QRegExp("[0-3]{1}[0-9]{1}-[0-1]{1}[1-9]{1}")));

    ui->date_line_edit->setText(QString::fromStdString(date_d_m()));
//    ui->date_make_template_line_edit->setText(QString::fromStdString(date_m_d()));

    pugi::xml_document doc;
    if (!doc.load_file("vars.xml")) {
        spdlog::info("Load vars XML FAILED");
        return;
    } else {
        spdlog::info("Load vars XML SUCCESS");
    }

    pugi::xml_node save_folder_child = doc.child("vars").child("save_folder");
    pugi::xml_node save_remote_vsd_child = doc.child("vars").child("save_remote_vsd");
    pugi::xml_node need_vsd_child = doc.child("vars").child("need_vsd");
    pugi::xml_node company_name_child = doc.child("vars").child("company_name");
    pugi::xml_node max_scans_for_template_child = doc.child("vars").child("max_scans_for_template");
    max_scans_for_template = max_scans_for_template_child.text().as_int(2500) ;
    company_name = company_name_child.text().get();
    save_folder = save_folder_child.text().get();
    save_remote_vsd = save_remote_vsd_child.text().get();
    std::string need_vsd_str = need_vsd_child.text().get();
    cout << "max_scans_for_template: " << max_scans_for_template << endl;
    spdlog::info("max_scans_for_template: {} ", max_scans_for_template);
    spdlog::info("company_name: {} ", company_name);
    spdlog::info("need_vsd_str: {} ", need_vsd_str);
    need_vsd = need_vsd_str == "no" ? false : true;
    spdlog::info("save_folder: {} ", save_folder);
    spdlog::info("save_remote_vsd: {} ", save_remote_vsd);
    spdlog::info("need_vsd: {} ", need_vsd);

    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &MainWindow::on_new_connection);
    connect(ui->save_vsd_button, &QPushButton::clicked, this, &MainWindow::update_xml_with_vsds_from_table);

    if(not mTcpServer->listen(QHostAddress::Any, 6000))
        spdlog::info("tcp_server status=not_started");
    else
        spdlog::info("tcp_server status=started");

    QStringList headers;
    headers << trUtf8("Продукция")
            << trUtf8("ВСД");
    ui->table->show();
    //ui->company_label->setStyleSheet("QLabel { background-color : red; color : blue; }");
    ui->company_label->setText(company_name.c_str());
    ui->company_label->setStyleSheet("QLabel{font-size: 25px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
    std::string sss = "НПП СИСТЕМА\nпромышленная автоматизация +79962009007";
    ui->dev_company_label->setText(sss.c_str());
    ui->dev_company_label->setStyleSheet("QLabel{font-size: 22px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
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

    if(company_name == std::string("ООО \"ИМПЕРИЯ СОКОВ\"")) {
        //ui->groupBox->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->create_line_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        //ui->product_name_for_task_combobox->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->product_name_for_task_combobox->setStyleSheet("QComboBox{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->date_line_edit->setStyleSheet("QLineEdit{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->create_plan_line_edit->setStyleSheet("QLineEdit{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->create_plan_line_edit->setToolTip("План задать");
        ui->create_plan_line_edit->setPlaceholderText("План задать");
        ui->line_number_choose_combobox->setStyleSheet("QComboBox{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->add_line_pushbutton->setStyleSheet("QPushButton{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->create_template_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->product_name_combobox->setStyleSheet("QComboBox{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->make_template_pushbutton->setStyleSheet("QPushButton{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->line_number_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->scaner_status_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->product_name_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->date_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->plan_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->current_label->setStyleSheet("QLabel{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
        ui->copy_button->setStyleSheet("QPushButton{font-size: 18px;font-family: Arial;color: rgb(255, 255, 255);background-color: rgb(92, 99, 118);}");
    }

    auto rus_eng_names = fill_table();
    QStringList items;
    for(auto const& [name_rus, name_eng]: rus_eng_names) {
        items.append(QString::fromStdString(name_rus));
        product_names[name_rus] = name_eng;
    }

    ui->product_name_combobox->addItems(items);
    ui->product_name_for_task_combobox->addItems(items);

//    ui->product_name_for_task_combobox->setStyleSheet("QLabel{background-color: rgb(158, 99, 118);}");
    if(not need_vsd) {
        ui->table->hide();
        ui->save_vsd_button->hide();
    } else {
        ui->verticalSpacer->changeSize(1,1);
    }
}

map<std::string, std::string>  MainWindow::fill_table() {
    std::map<std::string, std::string> rus_eng_names;

    fs::current_path(save_folder);

    pugi::xml_document doc;
    if (!doc.load_file("positions.xml")) {
        spdlog::info("Load XML positions.xml FAILED");
        return {};
    } else {
        spdlog::info("Load XML positions.xml SUCCESS");
    }

    pugi::xml_node inn_xml = doc.child("resources").child("inn");
    pugi::xml_node version = doc.child("resources").child("version");

    pugi::xml_node positions_xml = doc.child("resources").child("input");

    if (positions_xml.children("position").begin() == positions_xml.children("position").end()) {
        spdlog::info("Positions in positions.xml absent ERROR");
        return {};
    }

    int row = 0;
    for (pugi::xml_node position_xml: positions_xml.children("position")) {
        std::string name_eng = position_xml.attribute("name_english").as_string();
        std::string name_rus = position_xml.attribute("name").as_string();
        std::string vsd = position_xml.attribute("vsd").as_string();
        spdlog::info("{}", position_xml.attribute("vsd").as_string());
        spdlog::info("{}", position_xml.attribute("name").as_string());

        if(need_vsd) {
            ui->table->insertRow(row);
            ui->table->setItem(row,0, new QTableWidgetItem(QString::fromStdString(name_rus)));
            ui->table->setItem(row,1, new QTableWidgetItem(QString::fromStdString(vsd)));
        }

        rus_eng_names.emplace(name_rus, name_eng);
    }

    if(need_vsd)
        ui->table->resizeColumnsToContents();

    return rus_eng_names;
}

void MainWindow::on_new_connection() {
    QTcpSocket * new_socket = mTcpServer->nextPendingConnection();
    sockets.push_back(new_socket);
    spdlog::info("new connection, local {} {}, peer {} {}", new_socket->localAddress().toString().toStdString(), new_socket->localPort(), new_socket->peerAddress().toString().toStdString(), new_socket->peerPort());

    connect(new_socket, &QTcpSocket::readyRead, this, &MainWindow::on_server_read);
    connect(new_socket, &QTcpSocket::disconnected, this, &MainWindow::on_client_disconnected);
}

void MainWindow::on_server_read() {
    QTcpSocket* socket = qobject_cast< QTcpSocket* >(sender());

    spdlog::info("Received bytes: {}", socket->bytesAvailable());

    QByteArray array;
    QByteArray array_header;

    while(socket->bytesAvailable() < 4)
        if (!socket->waitForReadyRead(10))
            spdlog::info("waiting header bytes timed out");

    array_header = socket->read(4);

    uint32_t bytes_to_read;
    QDataStream ds_header(array_header);
    ds_header >> bytes_to_read;

    spdlog::info("header : {}, bytes to read for this message: {}", array_header.toHex().toStdString(), bytes_to_read);

    int attempts = 50;
    if(bytes_to_read > 0) {
        if (!socket->waitForReadyRead(100)) {
            spdlog::info("waiting header bytes timed out. attempts: {}", attempts);
            if(not attempts--) {
                socket->close();
                spdlog::info("socket closed");
                return;
            }
        }

        QByteArray read_bytes_chunk = socket->read(bytes_to_read);

        array.append(read_bytes_chunk);
        bytes_to_read -= read_bytes_chunk.size();

        spdlog::info("read bytes already: {}, remain bytes: {}", read_bytes_chunk.size(), bytes_to_read);
    }

    QDataStream received_bytes(array);

    uint8_t msg_type;
    uint8_t line_number;
    uint8_t task_number;

    received_bytes >> msg_type;
    received_bytes >> line_number;
    received_bytes >> task_number;

    uint32_t body_size = array.size() - 3;

    spdlog::info("msg type: {}, line number: {}, task number: {}", msg_type, line_number, task_number);

//    qDebug() << "receive method socket " << socket;
//    qDebug() << "receive method socket " << &*socket;
    auto& connector = create_connector_if_needed(line_number, socket);
//    qDebug() << "connector " << &connector;
//    bool res = connector.socket == nullptr;
//    qDebug() << "here " << res ;

    switch(msg_type) {
        case 1:
        {
//            qDebug() << "--------Info request--------";
            const auto & tasks_for_line = get_tasks_for_line(line_number);

            std::vector<string> tasks_info;

            TaskStatus task_status;

            for_each(begin(tasks_for_line), end(tasks_for_line),[&](auto & task) {
//                if(task->line_number == line_number) {
                auto number = task->task_number;
                auto name = task->product_name_eng();
                auto plan = task->plan();
                auto date = task->date;
                auto info = QString("%1:%2:%3:%4;").arg(number).arg(QString::fromStdString(name)).arg(plan).arg(QString::fromStdString(date)).toStdString();
                task_status = task->status();
                tasks_info.push_back(info);
                if(task_status == TaskStatus::IN_PROGRESS) {
                    task->state_button()->setStyleSheet("QPushButton{font-size: 25px;font-family: Arial;color: rgb(255, 255, 255);background-color: grey;}");
                    task->state_button()->setText("В работе");
                } else {
                    task->set_status(TaskStatus::TASK_SEND);
                    task->state_button()->setText("Задача передана");
                }
//                }
            });

            if(not tasks_info.empty())
                tasks_info.back().pop_back(); // rm last ;

            connector.send_tasks(tasks_info);
//            for_each(begin(tasks_for_line), end(tasks_for_line),[&](auto & task) {
//                if(task->line_number == line_number)
//                    if(task_status == TaskStatus::IN_PROGRESS)
//                       connector.send_ready(task->task_number);
//            });
        }
        break;

        case 2:
        {

            auto task_it = std::find_if(begin(tasks), end(tasks),
                                                    [&](auto &task) { return line_number == task->line_number && task_number == task->task_number;});

            if(task_it == end(tasks)) {
                spdlog::error("Task doesn't exist");
                return;
            }

            auto & task = *task_it;

            QByteArray buffer(body_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), body_size);
            QString scan_number(buffer);
            spdlog::info("Received scan number: {}", scan_number.toLong());

            task->set_current(scan_number);
        }
        break;
        case 3:
        {
//            qDebug() << "--------File receive--------";

            auto task_it = std::find_if(begin(tasks), end(tasks),
                                                    [&](auto &task) { return line_number == task->line_number && task_number == task->task_number;});

            if(task_it == end(tasks)) {
                spdlog::error("Task doesn't exist");
                return;
            }

            auto & task = *task_it;

            if(task->status() == TaskStatus::FINISHED)
                spdlog::error("Task FINISHED already");

            QByteArray buffer(body_size, Qt::Uninitialized);

            received_bytes.readRawData(buffer.data(), body_size);
            QString scans(buffer);

            spdlog::info("Start to save file with scans: {}", scans.toStdString());
            auto pos = scans.lastIndexOf(QChar('\n'));
            if(pos != -1)
                scans.truncate(pos);

            fs::path work_path = save_folder;
            work_path /= "ki";
            work_path /= task->date;
//            qDebug() << "work_path: " << work_path.c_str();
//            qDebug() << "work_path: " << QString::fromStdString(work_path.string());
            fs::create_directories(work_path);
            fs::current_path(work_path);
            string scans_number = to_string(scans.count(QChar('\n')) + 1);
            spdlog::info("scans: {}", scans_number);
//            qDebug() << "fs::current_path=" << fs::current_path().c_str();

            QString product_name;

            std::string filename = task->product_name_eng() + "__" + scans_number + " __" + time_h_m() + ".csv";
            task->current_label->setText(QString::fromStdString(scans_number));
            spdlog::info("filename: {}", filename.c_str());
            work_path /= filename;
//            qDebug() << "work_path: " << work_path.c_str();
            task->file_name = work_path.string();
            std::ofstream out(work_path.string());
            out << scans.toStdString();
            out.close();

            spdlog::info("Finished file save");
            task->set_status(TaskStatus::FINISHED);
            task->state_button()->setText("Завершено. Сделать шаблон");
            task->state_button()->setStyleSheet("QPushButton{font-size: 25px;font-family: Arial;color: rgb(255, 255, 255);background-color: green;}");
//            qDebug() << "111";
            connector.send_file_received();
        }
        break;
        case 4:
        {
//            qDebug() << "--------Task started--------";
            const auto & tasks_for_line = get_tasks_for_line(line_number);

            for_each(begin(tasks_for_line), end(tasks_for_line),[&](auto & task) {
                if(task_number == task->task_number) {
                    task->set_status(TaskStatus::IN_PROGRESS);
                }
            });
        }
        break;
    }

    spdlog::info("Received bytes at the end: {}", socket->bytesAvailable());
    if(socket->bytesAvailable())
        on_server_read();
}

void MainWindow::on_client_disconnected() {
    QTcpSocket* socket = qobject_cast< QTcpSocket* >(sender());
    socket->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    auto task_it = std::find_if(begin(tasks), end(tasks), [](auto &task) { return task->status() != TaskStatus::FINISHED;});

    QString msg = "Вы уверены, что хотите выйти?\n";
    if(task_it != end(tasks))

        msg.append("Существуют не завершенные задачи.");

    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Завершение", msg,
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes)
        event->ignore();
    else
        event->accept();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_make_template_pushbutton_clicked() {
    //------------------------File choose-----------------------------
    QString ki_name = QFileDialog::getOpenFileName(this, "Ki", QString::fromStdString(save_folder));
    qDebug() << "Filename ki: " << ki_name;
    string product_name = product_names[ui->product_name_combobox->currentText().toStdString()];
//    string date = ui->date_make_template_line_edit->text().toStdString();

    make_template(ki_name, product_name);
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

//    qDebug() << "connector_itr: " << &connector_itr;
    if(connector_itr == end(connectors)) {
        qDebug() << "Connection wasn't established ERROR";
        return;
    }

    auto task_it = std::find_if(begin(tasks), end(tasks),
                                            [&](auto &task) { return line_number == task->line_number && task_number == task->task_number;});

    if(task_it == end(tasks)) {
        qDebug() << "Task doesn't exist! ERROR";
        return;
    }

    auto & task = *task_it;

    switch (task->status()) {
//    case TaskStatus::INIT:
//        break;
//    case TaskStatus::TASK_SEND:
//        task->state_button()->setStyleSheet("QPushButton{font-size: 25px;font-family: Arial;color: rgb(255, 255, 255);background-color: grey;}");
//        task->state_button()->setText("В работе");
//        connector_itr->send_ready(task->task_number);
//        task->set_status(TaskStatus::IN_PROGRESS);
//        break;
    case TaskStatus::FINISHED:
        {
        //------------------------File choose-----------------------------
        QString ki_name = QString::fromStdString(task->file_name);
        make_template(ki_name, task->product_name_eng());
        qDebug() << "tasks.size(): " << tasks.size();
        //auto it = find_if(tasks.begin(), tasks.end(),[&](auto & task) {return task->line_number == line_number;});
        tasks.erase(task_it);
        qDebug() << "tasks.size(): " << tasks.size();
        }
        break;
    default:
        break;
    }
}

void MainWindow::on_add_line_pushbutton_clicked() {
    qDebug() << "===============" << __PRETTY_FUNCTION__ << "===============";
    auto line_number = ui->line_number_choose_combobox->currentText().toInt();

    auto product_name_rus = ui->product_name_for_task_combobox->currentText().toStdString();
    auto date = ui->date_line_edit->text().toStdString();
    int plan_remains = ui->create_plan_line_edit->text().toInt();
    int current_plan = plan_remains;

    while(plan_remains) {
        ++task_counter;
        if(plan_remains / max_scans_for_template) {
            plan_remains -= max_scans_for_template;
            current_plan = max_scans_for_template;
        }
        else {
            current_plan = plan_remains;
            plan_remains = 0;
        }

        auto task = make_shared<TaskInfo>(line_number, task_counter, product_names, product_name_rus, date, current_plan, company_name);

        auto inserted_task = tasks.emplace_back(task); // implicitly uint -> uint8_t for line_number

        connect(inserted_task->state_button(), &QPushButton::clicked, this, &MainWindow::make_next_action);
        connect(inserted_task->delete_task_button, &QPushButton::clicked, this, &MainWindow::remove_task);

        ui->lines_layout->addLayout(inserted_task->layout());
    }
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
// make ki file devision also
void MainWindow::make_template(QString ki_name, std::string product_name) {
    qDebug() << "Filename ki: " << ki_name;
    qDebug() << "product_name: " << QString::fromStdString(product_name);
    if(ki_name.isEmpty())
        return;

    //------------------------Download config-----------------------------
    Position position;

    spdlog::info("current_path: {} ", fs::current_path().string());
    for (const auto & entry : fs::directory_iterator(fs::current_path()))
        std::cout << entry.path() << std::endl;

    fs::current_path(save_folder);
    spdlog::info("current_path: {} ", fs::current_path().string());

    pugi::xml_document doc;
    if (!doc.load_file("positions.xml")) {
        spdlog::info("Load XML positions.xml FAILED");
        return;
    } else {
        spdlog::info("Load XML positions.xml SUCCESS");
    }

    pugi::xml_node inn_xml = doc.child("resources").child("inn");
    pugi::xml_node version = doc.child("resources").child("version");

    pugi::xml_node positions_xml = doc.child("resources").child("input");

    if (positions_xml.children("position").begin() == positions_xml.children("position").end()) {
        spdlog::info("Positions in positions.xml absent ERROR");
        return;
    }

    for (pugi::xml_node position_xml: positions_xml.children("position")) {
        std::string position_name = position_xml.attribute("name_english").as_string();
        spdlog::info("position_name= {} ", position_name);

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

            std::stringstream ss;
            ss << position;

            spdlog::info("position= {} ", ss.str());
        }
    }
    //------------------------Make file-----------------------------

    // Подсчитать кол-во сканов
    string s;
    int sTotal = 0;

    std::ifstream in(ki_name.toStdString());

    sTotal = std::count(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), '\n') + 1;
    qDebug() << "sTotal=" << sTotal;

    spdlog::info("sTotal: {} ", sTotal);
    in.close();
    // Создать файл

    string filename = product_name + "__" + to_string(sTotal) + "__" + time_h_m() + ".csv";

    fs::path work_path = save_folder;
    work_path /= "input";
    work_path /= date_d_m();
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
    template_file << position.inn << "," << position.inn << "," << position.inn << "," << date_y_m_d() << ",Собственное производство,4" << endl;
    if(need_vsd)
        template_file << "КИ,КИТУ,Дата производства,Код ТН ВЭД ЕАС товара,Вид документа подтверждающего соответствие,Номер документа подтверждающего соответствие,Дата документа подтверждающего соответствие,Идентификатор ВСД" << endl;
    else
        template_file << "КИ,КИТУ,Дата производства,Код ТН ВЭД ЕАС товара,Вид документа подтверждающего соответствие,Номер документа подтверждающего соответствие,Дата документа подтверждающего соответствие,Номер скважины,Идентификатор ВСД" << endl;
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
           << date_y_m_d() << ","
           << position.code_tn_ved << ","
           << position.document_type << ","
           << position.document_number << ","
           << position.document_date;

        if(need_vsd)
            template_file << "," << position.vsd;
        else {
            template_file << "," << "\"1\"" << ",";
        }

        if(--sTotal > 0)
            template_file << endl;
    }
}

void MainWindow::update_xml_with_vsds_from_table() {
    const auto& vsd_per_names = get_vsds();
    if(vsd_per_names.empty()) {
        spdlog::info("VSD not found ERROR");
        throw std::logic_error("error");
        close(); // не помню, что за close()
    }

    fs::current_path(save_folder);

    qDebug() << "save_folder=" << QString::fromStdString(save_folder);
    qDebug() << "fs::current_path=" << QString::fromStdString(fs::current_path().string());

    // XML open
    pugi::xml_document doc;
    if (doc.load_file("positions.xml")) {
        spdlog::info("Load XML success");
    } else {
        spdlog::info("Load XML ERROR");
        throw std::logic_error("error");
        close();
    }

    pugi::xml_node positions_xml = doc.child("resources").child("input");

    for (auto const& [name, vsd] : vsd_per_names) {
        qDebug() << "  vsd name: " << QString::fromStdString(name);

        for (pugi::xml_node position_xml: positions_xml.children("position")) {
            std::string name_in_xml = position_xml.attribute("name").as_string();
//            qDebug() << "   name_in_xml: " << QString::fromStdString(name_in_xml);
            if(name == name_in_xml) {
                position_xml.attribute("vsd").set_value(vsd.c_str());
                qDebug() << "    name: " << QString::fromStdString(name);
                qDebug() << "    set vsd to xml: " << QString::fromStdString(vsd);
            }
        }
    }

    if(not doc.save_file("positions.xml")) {
        spdlog::info("ERROR VSD safe local===============================================");
        throw std::logic_error("error");
    } else {
        spdlog::info("VSD safe SUCCES local==============================================");
    }

    if(not doc.save_file(save_remote_vsd.c_str())) {
        spdlog::info("ERROR VSD safe remote===============================================");
    } else {
        spdlog::info("VSD safe SUCCES remote==============================================");
    }
}

void MainWindow::on_copy_button_clicked()
{
    QString ki_name = QFileDialog::getOpenFileName(this, "Ki", QString::fromStdString(save_folder));
    qDebug() << "Filename ki: " << ki_name;
    string product_name = product_names[ui->product_name_combobox->currentText().toStdString()];

    QString dbase_str;

    QFile file(ki_name);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()){
                dbase_str += stream.readLine()+"\n";
            }
    }

//    qDebug() << dbase_str;

    QClipboard* c = QApplication::clipboard();
    c->setText(dbase_str);
}
