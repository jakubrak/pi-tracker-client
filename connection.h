#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QStringList>
#include <QTimer>
#include <QRect>
#include <QSize>

typedef QList<int> IdList;

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection();
    ~Connection();

signals:
    void opened();
    void timeout(int tryCount, int maxTry);
    void closed();
    void receivedStreamSettings(unsigned short port,
        unsigned int frameWidth,
        unsigned int frameHeight,
        float frameRate);

public slots:
    void open(QString address, short port);
    void getDeviceList();
    void getStreamSettings();
    void startStreaming();
    void setROI(QSize windowSize, QRect roi);

private slots:
    void readyRead();
    void socketError(const QAbstractSocket::SocketError &e);
    void timeout();

private:
    void send(QJsonObject json);
    void parse(const QJsonObject &json);
    QTcpSocket *tcpSocket;
    QString address;
    short port;
    unsigned int tryCount;
};

#endif // CONNECTION_H
