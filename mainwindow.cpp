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

    connection = new Connection();
    thread = new QThread(this);
    connection->moveToThread(thread);
    thread->start();

    videoWidget = new QGst::Ui::VideoWidget();
    overlayWidget = new OverlayWidget();
    videoScene =  new QGraphicsScene(ui->graphicsView);
    videoScene->addWidget(videoWidget);
    videoScene->addWidget(overlayWidget);
    videoScene->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    ui->graphicsView->setScene(videoScene);

    connectDialog = new ConnectDialog(this);

    // delete connection when thread finished
    connect(thread, SIGNAL(finished()), connection, SLOT(deleteLater()));

    // connection signals
    connect(connection, SIGNAL(opened()), this, SLOT(connectionOpened()));
    connect(connection, SIGNAL(closed()), this, SLOT(connectionClosed()));
    connect(connectDialog, SIGNAL(connectToServer(QString, short)), connection, SLOT(open(QString, short)));
    connect(connection, SIGNAL(timeout(int,int)), connectDialog, SLOT(connectionTimeout(int, int)));
    connect(this, SIGNAL(getStreamSettings()), connection, SLOT(getStreamSettings()));
    connect(connection, SIGNAL(receivedStreamSettings(int, int, int, int, double)),
            this, SLOT(receivedStreamSettings(int, int, int, int, double)));
    connect(this, SIGNAL(startStreaming()), connection, SLOT(startStreaming()));

    // ROI
    connect(overlayWidget, SIGNAL(setRoi(QRect)), connection, SLOT(setRoi(QRect)));

    this->showMaximized();
    connectDialog->show();
}

MainWindow::~MainWindow() {
    thread->exit();
    thread->wait();
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    ui->graphicsView->fitInView(videoScene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::connectionOpened() {
    connectDialog->hide();
    emit getStreamSettings();
}

void MainWindow::connectionClosed() {
    connectDialog->show();
}

void MainWindow::frameSizeChanged(QSize frameSize) {
    qDebug() << "New frame size: " << frameSize;
    overlayWidget->setMinimumSize(frameSize);
    videoWidget->setMinimumSize(frameSize);
    ui->graphicsView->fitInView(videoScene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::receivedStreamSettings(int videoPort,
    int metadataPort,
    int frameWidth,
    int frameHeight,
    double frameRate) {
    qDebug("Received stream settings:\n"
           "\tvideo port: %d\n"
           "\tmetadata port %d\n"
           "\tframe width: %d\n"
           "\tframe height: %d\n"
           "\tframe rate: %f\n",
           videoPort, metadataPort, frameWidth, frameHeight, frameRate);

    pipeline = new Pipeline("192.168.0.108", videoPort, metadataPort, this);
    videoWidget->setVideoSink(pipeline->getVideoSink());
    connect(pipeline, SIGNAL(frameSizeChanged(QSize)), this, SLOT(frameSizeChanged(QSize)));
    connect(pipeline, SIGNAL(updateRoi(QRect)), overlayWidget, SLOT(updateRoi(QRect)));
    pipeline->play();
    emit startStreaming();
}
