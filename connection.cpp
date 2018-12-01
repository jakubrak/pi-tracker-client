#include "connection.h"
#include <QByteArray>
#include <QStack>
#include <QJsonDocument>
#include <QThread>
#include "common.h"

Connection::Connection() : tryCount(0) {
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
}

Connection::~Connection() {
    if(tcpSocket)
    {
        if(tcpSocket->isOpen())
        {
            tcpSocket->close();
        }
        delete tcpSocket;
    }
}

void Connection::open(QString address, short port) {
    this->address = address;
    this->port = port;

    tcpSocket->connectToHost(address, port);
    qDebug() << "Tryng to connect " << tryCount << "(ip =" << address << ", port =" << port << ")";
    if (tcpSocket->waitForConnected(1000) == false) {
        QTimer::singleShot(1000, this, SLOT(timeout()));
    } else {
        tryCount = 0;
        emit opened();
        LOG("Connection established")
    }
}

void Connection::timeout() {
    if (tryCount < 10){
        open(address, port);
        emit timeout(tryCount, 10);
        ++tryCount;
    } else {
        emit timeout(tryCount, 10);
        tryCount = 0;
    }
}

void Connection::send(QJsonObject json) {
    QJsonDocument jsonDoc(json);
    QString strJson(jsonDoc.toJson(QJsonDocument::Compact));
    tcpSocket->write(strJson.toUtf8());
    QTime timestamp = QTime::currentTime();
    qDebug() << timestamp.hour() << timestamp.minute() << timestamp.second() << "SENT:" << strJson.toUtf8();
}

void Connection::readyRead() {
    //qDebug() << __FILE__ << ":" << __FUNCTION__ << "received " << tcpSocket->bytesAvailable() << " bytes";
    static QStack<char> stack;
    static QString data;
    do
    {
        char c;
        tcpSocket->read(&c, 1);
        switch (c) {
        case '{':
            stack.push(c);
            data.append(c);
            break;
        case '}':
            stack.pop();
            data.append(c);
            if(stack.isEmpty())
            {
                //got complete json document
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8());
                //QTime timestamp = QTime::currentTime();
                //qDebug() << timestamp.hour() << timestamp.minute() << timestamp.second() <<"RECEIVED:" << data.toUtf8();
                LOG(data.toUtf8());
                parse(jsonDoc.object());
                data.clear();
            }
            break;
        default:
            if(!data.isEmpty())
            {
                data.append(c);
            }
            break;
        }
    }
    while(tcpSocket->bytesAvailable() > 0);
}

void Connection::parse(const QJsonObject &json) {
    QString msgid = json["msgid"].toString();
    if(msgid.compare(msgid, "GET_VIDEO_CAP_DEV_LIST") == 0) {

    } else if(msgid.compare(msgid, "STREAM_SETTINGS") == 0) {
        const unsigned short port = (const unsigned short)json["port"].toInt();
        auto frame = json["frame"].toObject();
        const unsigned int frameWidth = frame["width"].toInt();
        const unsigned int frameHeight = frame["height"].toInt();
        const float frameRate = (const float)frame["rate"].toDouble();
        emit receivedStreamSettings(port, frameWidth, frameHeight, frameRate);
    }
}

void Connection::getDeviceList() {
    QJsonObject json;
    json.insert("msgid", "GET_VIDEO_CAP_DEV_LIST");
    send(json);
}

void Connection::getStreamSettings() {
    QJsonObject json;
    json.insert("msgid", "GET_STREAM_SETTINGS");
    send(json);
}

void Connection::startStreaming() {
    QJsonObject json;
    json.insert("msgid", "START_STREAMING");
    send(json);
}

void Connection::setROI(QSize windowSize, QRect roi) {
    QJsonObject json;
    json.insert("msgid", "SET_ROI");
    json.insert("windW", windowSize.width());
    json.insert("windH", windowSize.height());
    json.insert("roiX", roi.topLeft().x());
    json.insert("roiY", roi.topLeft().y());
    json.insert("roiW", roi.width());
    json.insert("roiH", roi.height());
    send(json);
}

void Connection::socketError(const QAbstractSocket::SocketError &e) {
    qDebug() << e;
    if(e == QAbstractSocket::RemoteHostClosedError) {
        emit closed();
    }
}

