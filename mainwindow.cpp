#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"

#include <regex>

#include <QGst/Parse>
#include <QGst/Element>
#include <QGst/Pad>
#include <QGst/Message>
#include <QGst/Bus>
#include <QGlib/Connect>
#include <QGst/ElementFactory>
#include <QGst/GhostPad>

#include <QGLWidget>
#include <QGraphicsView>
#include <QDebug>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    pipeline.clear();

//    connection = new Connection();
//    thread = new QThread(this);
//    connection->moveToThread(thread);
//    connect(thread, SIGNAL(finished()), connection, SLOT(deleteLater()));
//    connect(this, SIGNAL(getDeviceList()), connection, SLOT(getDeviceList()));
//    connect(connection, SIGNAL(opened()), this, SLOT(connectionOpened()));


//    connect(this, SIGNAL(getStreamSettings()), connection, SLOT(getStreamSettings()));
//    connect(connection, SIGNAL(receivedStreamSettings(unsigned short,uint,uint,float)),
//            this, SLOT(receivedStreamSettings(unsigned short,uint,uint,float)));
//    connect(this, SIGNAL(startStreaming()), connection, SLOT(startStreaming()));
//    connect(connection, SIGNAL(closed()), this, SLOT(connectionClosed()));

    videoScene =  new QGraphicsScene(ui->graphicsView);
    ui->graphicsView->setScene(videoScene);
    videoSurface = new QGst::Ui::GraphicsVideoSurface(ui->graphicsView);
    videoWidget = new QGst::Ui::GraphicsVideoWidget();
    videoWidget->setSurface(videoSurface);
    videoScene->addItem(videoWidget);

    overlayWidget = new OverlayWidget();
    QGraphicsProxyWidget *proxyOverlayWidget = videoScene->addWidget(overlayWidget);
    proxyOverlayWidget->setParentItem(videoWidget);
    proxyOverlayWidget->setZValue(1);
    //connect(overlayWidget, SIGNAL(setROI(QSize,QRect)), connection, SLOT(setROI(QSize,QRect)));

    overlayWidget->setMinimumSize(QSize(1280, 720));
    videoWidget->setMinimumSize(QSize(1280, 720));
    videoScene->setSceneRect(videoWidget->rect());
    ui->graphicsView->fitInView(videoWidget->rect(), Qt::KeepAspectRatio);
    this->showMaximized();

//    connectDialog = new ConnectDialog(this);
//    connect(connectDialog, SIGNAL(connectToServer(QString, short)), connection, SLOT(open(QString, short)));
//    connect(connection, SIGNAL(timeout(int,int)), connectDialog, SLOT(connectionTimeout(int, int)));

    appSink = new AppSink(this);
    connect(appSink, SIGNAL(setRoi(QRect)), overlayWidget, SLOT(updateRoi(QRect)));

//    thread->start();
//    connectDialog->show();
    receivedStreamSettings(0, 1280, 720, 15.0f);
}

MainWindow::~MainWindow() {
    thread->exit();
    thread->wait();
    destroyPipeline();
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    videoScene->setSceneRect(videoScene->sceneRect());
    ui->graphicsView->fitInView(videoScene->sceneRect(), Qt::KeepAspectRatio);
}

QString MainWindow::toString(QGst::State state) {
    switch(state) {
    case QGst::StateNull:
        return "Null";
        break;
    case QGst::StatePaused:
        return "Paused";
        break;
    case QGst::StatePlaying:
        return "Playing";
        break;
    case QGst::StateReady:
        return "Ready";
        break;
    case QGst::StateVoidPending:
        return "VoidPending";
        break;
    }
}

void MainWindow::onBusMessage(const QGst::ObjectPtr object, const QGst::MessagePtr &message) {
    //qDebug() << message->typeName();
    switch (message->type()) {
    case QGst::MessageEos:
        exit(0);
        break;
    case QGst::MessageError:
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << message.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageStateChanged:
        qDebug() << "[GSTREAMER] Pipeline state changed to"
                 << toString(message.staticCast<QGst::StateChangedMessage>()->newState());
        if (message.staticCast<QGst::StateChangedMessage>()->newState() == QGst::StatePaused)
                pipeline->setState(QGst::StatePlaying);
        break;
    case QGst::MessageElement:
        qDebug() << "[GSTREAMER] Element message: " << object.staticCast<QGst::Element>()->name();
        for (uint index=0; index < pipeline->childrenCount(); ++index) {
            QGst::ElementPtr elem = pipeline->childByIndex(index).staticCast<QGst::Element>();
            QGst::State state, statePending;
            elem->getState(&state, &statePending, 0);
            qDebug() << elem->name() << ":" << toString(state);
        }
        break;
    default:
        break;
    }
}

void MainWindow::onNewSample(const QGst::ElementPtr appsink) {
    qDebug() << "new data";
}

std::unique_ptr<Session> MainWindow::makeVideoSession() {
    auto bin = QGst::Bin::create("video");

    auto depayloader = QGst::ElementFactory::make("rtph264depay");
    if (!depayloader) {
        qFatal("Failed to create element. Aborting...");
    }

    auto decoder = QGst::ElementFactory::make("avdec_h264");
    if (!decoder) {
        qFatal("Failed to create element. Aborting...");
    }

    auto converter = QGst::ElementFactory::make("videoconvert");
    if (!converter) {
        qFatal("Failed to create element. Aborting...");
    }

    auto videoSink = videoSurface->videoSink();
    videoSink->setProperty("sync", true);
    videoSink->setProperty("async", true);

    bin->add(depayloader, decoder, converter, videoSink);
    bin->linkMany(depayloader, decoder, converter, videoSink);

    auto caps = QGst::Caps::fromString("application/x-rtp,"
                                       "media=video,"
                                       "encoding-name=H264,"
                                       "payload=96,"
                                       "clock-rate=90000");

    bin->addPad(QGst::GhostPad::create(depayloader->getStaticPad("sink"), "sink"));

    return std::make_unique<Session>(1, caps, bin);
}

std::unique_ptr<Session> MainWindow::makeMetadataSession()
{
    auto bin = QGst::Bin::create("klv");

    auto depayloader = QGst::ElementFactory::make("rtpklvdepay");
    if (!depayloader) {
        qFatal("Failed to create element. Aborting...");
    }

    auto capsFilter = QGst::ElementFactory::make("capsfilter");
    if (!capsFilter) {
        qFatal("Failed to create element. Aborting...");
    }
    capsFilter->setProperty("caps", QGst::Caps::fromString("meta/x-klv"));

    appSink->setCaps(QGst::Caps::fromString("meta/x-klv"));
    appSink->element()->setProperty("sync", true);
    appSink->element()->setProperty("async", true);

    bin->add(depayloader, capsFilter, appSink->element());
    bin->linkMany(depayloader, capsFilter, appSink->element());

    auto caps = QGst::Caps::fromString("application/x-rtp,"
                                       "media=application,"
                                       "encoding-name=SMPTE336M,"
                                       "payload=96,"
                                       "clock-rate=90000");

    bin->addPad(QGst::GhostPad::create(depayloader->getStaticPad("sink"), "sink"));

    return std::make_unique<Session>(2, caps, bin);
}

void MainWindow::onPadAdded(const QGst::PadPtr &pad) {;
    qDebug() << "Pad added";
    std::regex re("recv_rtp_src_([0-9]+)");
    std::cmatch m;
    pad->name().sprintf("recv_rtp_src_");
    if (std::regex_search(pad->name().toStdString().c_str(), m, re)) {
        auto sessionId = std::stoi(m[1]);
        if (sessionId == videoSession->getId()) {
            qDebug() << "Video session";
            pipeline->add(videoSession->getBin());
            videoSession->getBin()->syncStateWithParent();
            auto videoPad =  videoSession->getBin()->getStaticPad("sink");
            if (!videoPad) {
                qDebug() << "Something is wrong with static pad";
            }
            pad->link(videoPad);
        } else if (sessionId == metadataSession->getId()) {
            qDebug() << "Metadata session";
            pipeline->add(metadataSession->getBin());
            metadataSession->getBin()->syncStateWithParent();
            pad->link(metadataSession->getBin()->getStaticPad("sink"));
        }
    }
}

void MainWindow::joinSession(QGst::ElementPtr rtpBin, int rtpPort, int rtcpPort, const Session& session) {
    auto rtpSrc = QGst::ElementFactory::make("udpsrc");
    if (!rtpSrc) {
        qFatal("Failed to create udpsrc. Aborting...");
    }
    rtpSrc->setProperty("port", rtpPort);
    rtpSrc->setProperty("caps", session.getCaps());
    pipeline->add(rtpSrc);
    rtpSrc->link(rtpBin, std::string("recv_rtp_sink_" + std::to_string(session.getId())).c_str());

    // recv sink will be added when the pad becomes available

    // video rtcp connection
    auto rtcpSrc = QGst::ElementFactory::make("udpsrc");
    rtcpSrc->setProperty("port", rtcpPort);
    pipeline->add(rtcpSrc);
    rtcpSrc->link(rtpBin, std::string("recv_rtcp_sink_" + std::to_string(session.getId())).c_str());
}

void MainWindow::createPipeline(const unsigned short port)
{
    destroyPipeline();

    pipeline = QGst::Pipeline::create();

    auto rtpBin = QGst::ElementFactory::make("rtpbin");
    if (!rtpBin) {
        qFatal("Failed to create rtpbin. Aborting...");
    }

    // watch the bus
    if (QGlib::connect(pipeline->bus(), "message", this, &MainWindow::onBusMessage, QGlib::PassSender) == false) {
         qDebug() << "Failed to connect message::error signal from " << pipeline->name();
    }
    pipeline->bus()->addSignalWatch();

    pipeline->add(rtpBin);
    rtpBin->setProperty("latency", 50);
    rtpBin->setProperty("do-retransmission", true);

    QGlib::connect(rtpBin, "pad-added", this, &MainWindow::onPadAdded);

    videoSession = makeVideoSession();
    metadataSession = makeMetadataSession();

    joinSession(rtpBin, 5000, 5001, *videoSession);
    joinSession(rtpBin, 5006, 5007, *metadataSession);

    // play
    pipeline->setState(QGst::StatePlaying);
}

void MainWindow::destroyPipeline() {
    if (pipeline.isNull() == false) {
        pipeline->setState(QGst::StateNull);
        pipeline.clear();
    }
}

void MainWindow::connectionOpened() {
    delete connectDialog;
    emit getStreamSettings();
}

void MainWindow::connectionClosed() {
    connectDialog = new ConnectDialog(this);
    connect(connectDialog, SIGNAL(connectToServer(QString, short)), connection, SLOT(open(QString, short)));
    connect(connection, SIGNAL(timeout(int,int)), connectDialog, SLOT(connectionTimeout(int, int)));
    connectDialog->show();
}

void MainWindow::receivedStreamSettings(unsigned short port,
    unsigned int frameWidth,
    unsigned int frameHeight,
    float frameRate) {

    /*videoScene =  new QGraphicsScene(ui->graphicsView);
    ui->graphicsView->setScene(videoScene);
    videoSurface = new QGst::Ui::GraphicsVideoSurface(ui->graphicsView);
    videoWidget = new QGst::Ui::GraphicsVideoWidget();
    videoWidget->setSurface(videoSurface);
    videoWidget->setPreferredSize(frameWidth, frameHeight);
    videoScene->addItem(videoWidget);*/
    //videoScene->addWidget(new OverlayWidget(ui->graphicsView));

    overlayWidget->setMinimumSize(QSize(frameWidth, frameHeight));
    videoWidget->setMinimumSize(QSize(frameWidth, frameHeight));
    videoScene->setSceneRect(videoWidget->rect());
    ui->graphicsView->fitInView(videoWidget->rect(), Qt::KeepAspectRatio);
    createPipeline(port);

    emit startStreaming();
}
