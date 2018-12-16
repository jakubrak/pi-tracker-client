#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGst/Ui/GraphicsVideoSurface>
#include <QGst/Ui/VideoWidget>
#include <QThread>

#include "connectdialog.h"
#include "overlaywidget.h"
#include "connection.h"

class Pipeline;

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

signals:
    void openConnection(QString address, short port);
    void getDeviceList();
    void getStreamSettings();
    void startStreaming();

private slots:
    void connectionOpened();
    void connectionClosed();
    void frameSizeChanged(QSize frameSize);
    void receivedStreamSettings(int videoPort,
        int metadataPort,
        int frameWidth,
        int frameHeight,
        double frameRate);

private:
    QGraphicsScene *videoScene;
    QGst::Ui::GraphicsVideoSurface *videoSurface;
    QGst::Ui::VideoWidget *videoWidget;

    Connection *connection;
    ConnectDialog *connectDialog;

    Ui::MainWindow *ui;

    QThread *thread;
    OverlayWidget *overlayWidget;

    Pipeline *pipeline;
};

#endif // MAINWINDOW_H
