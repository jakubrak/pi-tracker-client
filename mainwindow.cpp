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


    videoWidget = new QGst::Ui::VideoWidget();
    overlayWidget = new OverlayWidget();
    videoScene =  new QGraphicsScene(ui->graphicsView);
    videoScene->addWidget(videoWidget);
    videoScene->addWidget(overlayWidget);
    videoScene->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    ui->graphicsView->setScene(videoScene);

    //proxyOverlayWidget->setParentItem(videoWidget->graphicsProxyWidget());
    //proxyOverlayWidget->setZValue(1);
    //connect(overlayWidget, SIGNAL(setROI(QSize,QRect)), connection, SLOT(setROI(QSize,QRect)));

    //overlayWidget->setMinimumSize(QSize(1280, 720));
    //videoWidget->setMinimumSize(QSize(1280, 720));
    //videoScene->setSceneRect(videoWidget->rect());
    this->showMaximized();

//    connectDialog = new ConnectDialog(this);
//    connect(connectDialog, SIGNAL(connectToServer(QString, short)), connection, SLOT(open(QString, short)));
//    connect(connection, SIGNAL(timeout(int,int)), connectDialog, SLOT(connectionTimeout(int, int)));

    //connect(appSink, SIGNAL(setRoi(QRect)), overlayWidget, SLOT(updateRoi(QRect)));

//    thread->start();
//    connectDialog->show();
    pipeline = new Pipeline(this);
    connect(pipeline, SIGNAL(frameSizeChanged(QSize)), this, SLOT(frameSizeChanged(QSize)));

    videoWidget->setVideoSink(pipeline->getVideoSink());

    pipeline->play();

    emit startStreaming();
}

MainWindow::~MainWindow() {
    thread->exit();
    thread->wait();
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    qDebug() << videoScene->sceneRect();
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

void MainWindow::frameSizeChanged(QSize frameSize) {
    qDebug() << "New frame size: " << frameSize;
    overlayWidget->setMinimumSize(frameSize);
    videoWidget->setMinimumSize(frameSize);
    videoScene->setSceneRect(0, 0, frameSize.width(), frameSize.height());
    ui->graphicsView->fitInView(videoScene->sceneRect(), Qt::KeepAspectRatio);

}
