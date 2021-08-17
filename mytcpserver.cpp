#include "mytcpserver.h"
#include <QDebug>
#include <QDataStream>
#include <QCoreApplication>
#include <iostream>
#include <fstream>
#include <QBitArray>

MyTcpServer::MyTcpServer(QObject *parent) : QObject(parent)
{
    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &MyTcpServer::slotNewConnection);

    if(!mTcpServer->listen(QHostAddress::Any, 6000)){
        qDebug() << "server is not started";
    } else {
        qDebug() << "server is started";
    }
}

void MyTcpServer::slotNewConnection()
{
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
    connect(mTcpSocket, &QTcpSocket::readyRead, this, &MyTcpServer::slotServerRead);
    connect(mTcpSocket, &QTcpSocket::disconnected, this, &MyTcpServer::slotClientDisconnected);
}

void MyTcpServer::slotServerRead()
{
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

    uint plan = 10;
    QString name = "Sir Kavkazkiu";
    auto msg = QString("%1,%2").arg(name).arg(plan).toStdString();
    qDebug() << "msg.size(): " << msg.size();

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

void MyTcpServer::slotClientDisconnected()
{
    mTcpSocket->close();
}

