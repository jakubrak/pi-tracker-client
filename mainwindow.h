#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGst/Ui/GraphicsVideoSurface>
#include <QGst/Ui/GraphicsVideoWidget>
#include <QGst/Pipeline>
#include <QThread>

#include "connectdialog.h"
#include "connection.h"
#include "appsink.h"
#include "overlaywidget.h"
#include "session.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void resizeEvent(QResizeEvent *event);

    std::unique_ptr<Session> makeVideoSession();

    std::unique_ptr<Session> makeMetadataSession();

    void joinSession(QGst::ElementPtr rtpBin, int rtpPort, int rtcpPort, const Session& session);

signals:
    void openConnection(QString address, short port);
    void getDeviceList();
    void getStreamSettings();
    void startStreaming();

private slots:
    void connectionOpened();
    void connectionClosed();
    void receivedStreamSettings(unsigned short port,
        unsigned int frameWidth,
        unsigned int frameHeight,
        float frameRate);

private:
    QGraphicsScene *videoScene;
    QGst::Ui::GraphicsVideoSurface *videoSurface;
    QGst::Ui::GraphicsVideoWidget *videoWidget;

    ConnectDialog *connectDialog;
    Connection *connection;
    QGst::PipelinePtr pipeline;
    std::unique_ptr<Session> videoSession;
    std::unique_ptr<Session> metadataSession;

    Ui::MainWindow *ui;

    QThread *thread;
    QThread *gstThread;
    OverlayWidget *overlayWidget;

    AppSink *appSink;

    QString toString(QGst::State state);
    void createPipeline(const unsigned short port);
    void destroyPipeline();
    void onBusMessage(const QGst::ObjectPtr object, const QGst::MessagePtr &message);
    void onPadAdded(const QGst::PadPtr &pad);
    void onNewSample(const QGst::ElementPtr appsink);
};

#endif // MAINWINDOW_H
