#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <regex>

#include <QGLWidget>
#include <QGraphicsView>
#include <QDebug>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>

#include "pipeline.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);



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

    //connect(appSink, SIGNAL(setRoi(QRect)), overlayWidget, SLOT(updateRoi(QRect)));

//    thread->start();
//    connectDialog->show();
    receivedStreamSettings(0, 1280, 720, 15.0f);
}

MainWindow::~MainWindow() {
    thread->exit();
    thread->wait();
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    videoScene->setSceneRect(videoScene->sceneRect());
    ui->graphicsView->fitInView(videoScene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::connectionOpened() {
    delete connectDialog;
    emit getStreamSettings();
}

void MainWindow::connectionClosed() {
    connectDialog = new ConnectDialog(this);
    //connect(connectDialog, SIGNAL(connectToServer(QString, short)), connection, SLOT(open(QString, short)));
    //connect(connection, SIGNAL(timeout(int,int)), connectDialog, SLOT(connectionTimeout(int, int)));
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

    pipeline = std::make_unique<Pipeline>(videoSurface->videoSink());
    pipeline->play();

    emit startStreaming();
}
