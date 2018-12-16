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
    explicit Connection(QObject *parent = nullptr);
    ~Connection();

signals:
    void opened();
    void timeout(int tryCount, int maxTry);
    void closed();
    void receivedStreamSettings(int videoPort,
        int metadataPort,
        int frameWidth,
        int frameHeight,
        double frameRate);

public slots:
    void open(QString address, short port);
    void getDeviceList();
    void getStreamSettings();
    void startStreaming();
    void setRoi(QRect roi);

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
