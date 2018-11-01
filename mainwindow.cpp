#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"

#include <QGst/Parse>
#include <QGst/Element>
#include <QGst/Pad>
#include <QGst/Message>
#include <QGst/Bus>
#include <QGlib/Connect>
#include <QGst/ElementFactory>

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

    connection = new Connection();
    thread = new QThread(this);
    connection->moveToThread(thread);
    connect(thread, SIGNAL(finished()), connection, SLOT(deleteLater()));
    connect(this, SIGNAL(getDeviceList()), connection, SLOT(getDeviceList()));
    connect(connection, SIGNAL(opened()), this, SLOT(connectionOpened()));


    connect(this, SIGNAL(getStreamSettings()), connection, SLOT(getStreamSettings()));
    connect(connection, SIGNAL(receivedStreamSettings(unsigned short,uint,uint,float)),
            this, SLOT(receivedStreamSettings(unsigned short,uint,uint,float)));
    connect(this, SIGNAL(startStreaming()), connection, SLOT(startStreaming()));
    connect(connection, SIGNAL(closed()), this, SLOT(connectionClosed()));

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
    connect(overlayWidget, SIGNAL(setROI(QSize,QRect)), connection, SLOT(setROI(QSize,QRect)));

    overlayWidget->setMinimumSize(QSize(1280, 720));
    videoWidget->setMinimumSize(QSize(1280, 720));
    videoScene->setSceneRect(videoWidget->rect());
    ui->graphicsView->fitInView(videoWidget->rect(), Qt::KeepAspectRatio);
    this->showMaximized();

    connectDialog = new ConnectDialog(this);
    connect(connectDialog, SIGNAL(connectToServer(QString, short)), connection, SLOT(open(QString, short)));
    connect(connection, SIGNAL(timeout(int,int)), connectDialog, SLOT(connectionTimeout(int, int)));

    appSink = new AppSink(this);
    connect(appSink, SIGNAL(setRoi(QRect)), overlayWidget, SLOT(updateRoi(QRect)));

    thread->start();
    connectDialog->show();
    //createPipeline(5001);
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

void MainWindow::onPadAdded(const QGst::PadPtr &pad) {
    if (pad->name().startsWith(QLatin1String("video"))) {
        qDebug() << "Pad " << pad->name() << " added";

        QGst::ElementPtr queue = pipeline->getElementByName("queue_video");
        if (pad->link(queue->getStaticPad("sink")) != QGst::PadLinkOk) {
            qFatal("[GSTREAMER] Failed to link %s to %s",
                   pad->name().toStdString().c_str(),
                   queue->getStaticPad("sink")->name().toStdString().c_str());
        }
    } else {
        qDebug() << "Pad " << pad->name() << " added";
        QGst::ElementPtr queue = pipeline->getElementByName("queue_klv");
        if (pad->link(queue->getStaticPad("sink")) != QGst::PadLinkOk) {
            qFatal("[GSTREAMER] Failed to link %s to %s",
                   pad->name().toStdString().c_str(),
                   queue->name().toStdString().c_str());
        }
    }
}

void MainWindow::createPipeline(const unsigned short port)
{
    destroyPipeline();

    pipeline = QGst::Pipeline::create();
    if (QGlib::connect(pipeline->bus(), "message", this, &MainWindow::onBusMessage, QGlib::PassSender) == false) {
         qDebug() << "Failed to connect message::error signal from " << pipeline->name();
    }
    pipeline->bus()->addSignalWatch();

    QGst::ElementPtr udpsrc = QGst::ElementFactory::make("udpsrc", "source");
    if (!udpsrc) {
        qFatal("Failed to create udpsrc. Aborting...");
    }
    udpsrc->setProperty("port", (int)port);
    udpsrc->setProperty("caps", QGst::Caps::fromString("application/x-rtp,"
                                                          "media=(string)video,"
                                                          "encoding-name=(string)MP2T"));

    QGst::ElementPtr queue_udp = QGst::ElementFactory::make("queue", "queue_udp");
    if (!queue_udp) {
        qFatal("Failed to create queue. Aborting...");
    }

    QGst::ElementPtr rtpmp2tdepay = QGst::ElementFactory::make("rtpmp2tdepay", "rtpdepay");
    if (!rtpmp2tdepay) {
        qFatal("Failed to create rtph264depay. Aborting...");
    }

    QGst::ElementPtr tsdemux = QGst::ElementFactory::make("tsdemux", "demuxer");
    if (!tsdemux) {
        qFatal("Failed to create tsdemux. Aborting...");
    }

    QGst::ElementPtr queue_video = QGst::ElementFactory::make("queue", "queue_video");
    if (!queue_video) {
        qFatal("Failed to create queue. Aborting...");
    }

    QGst::ElementPtr h264parse = QGst::ElementFactory::make("h264parse", "parser");
    if (!h264parse) {
        qFatal("Failed to create h264parse. Aborting...");
    }

    QGst::ElementPtr avdec_h264 = QGst::ElementFactory::make("avdec_h264", "decoder");
    if (!avdec_h264) {
        qFatal("Failed to create avdec_h264. Aborting...");
    }

    QGst::ElementPtr videoconvert = QGst::ElementFactory::make("videoconvert", "converter");
    if (!videoconvert) {
        qFatal("Failed to create videoconvert. Aborting...");
    }

    QGst::ElementPtr appsink_video = videoSurface->videoSink();
    appsink_video->setProperty("sync", false);
    appsink_video->setProperty("async", false);
    appsink_video->setProperty("drop", true);

    QGst::ElementPtr queue_klv = QGst::ElementFactory::make("queue", "queue_klv");
    if (!queue_klv) {
        qFatal("Failed to create queue. Aborting...");
    }

    QGst::ElementPtr capsfilter_klv = QGst::ElementFactory::make("capsfilter", "capsfilter_klv");
    if (!capsfilter_klv) {
        qFatal("Failed to create capsfilter_klv. Aborting...");
    }

    capsfilter_klv->setProperty("caps", QGst::Caps::fromString("meta/x-klv"));

    appSink->setCaps(QGst::Caps::fromString("meta/x-klv"));
    QGst::ElementPtr appsink_klv = appSink->element();
    appsink_klv->setProperty("sync", false);
    appsink_klv->setProperty("async", false);

    pipeline->add(udpsrc);
    pipeline->add(queue_udp);
    pipeline->add(rtpmp2tdepay);
    pipeline->add(tsdemux);
    pipeline->add(queue_video);
    pipeline->add(h264parse);
    pipeline->add(avdec_h264);
    pipeline->add(videoconvert);
    pipeline->add(appsink_video);
    pipeline->add(queue_klv);
    pipeline->add(capsfilter_klv);
    pipeline->add(appsink_klv);

    if (udpsrc->link(queue_udp) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               udpsrc->name().toStdString().c_str(),
               queue_udp->name().toStdString().c_str());
    }

    if (queue_udp->link(rtpmp2tdepay) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               queue_udp->name().toStdString().c_str(),
               rtpmp2tdepay->name().toStdString().c_str());
    }

    if (rtpmp2tdepay->link(tsdemux) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               rtpmp2tdepay->name().toStdString().c_str(),
               tsdemux->name().toStdString().c_str());
    }

    if (queue_video->link(h264parse) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               queue_video->name().toStdString().c_str(),
               h264parse->name().toStdString().c_str());
    }

    if (h264parse->link(avdec_h264) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               h264parse->name().toStdString().c_str(),
               avdec_h264->name().toStdString().c_str());
    }

    if (avdec_h264->link(videoconvert) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               avdec_h264->name().toStdString().c_str(),
               videoconvert->name().toStdString().c_str());
    }

    if (videoconvert->link(appsink_video) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               videoconvert->name().toStdString().c_str(),
               appsink_video->name().toStdString().c_str());
    }

    if (QGlib::connect(tsdemux, "pad-added", this, &MainWindow::onPadAdded) == false) {
        qDebug() << "Failed to connect pad-added signal from " << tsdemux->name();
    }

    if (queue_klv->link(capsfilter_klv) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               queue_klv->name().toStdString().c_str(),
               capsfilter_klv->name().toStdString().c_str());
    }

    if (capsfilter_klv->link(appsink_klv) == false) {
        qFatal("[GSTREAMER] Failed to link %s to %s",
               capsfilter_klv->name().toStdString().c_str(),
               appsink_klv->name().toStdString().c_str());
    }

    pipeline->setProperty("latency", 500000000);

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
