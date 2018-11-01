#include <QApplication>
#include <QGst/Init>

#include "mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QGst::init(&argc, &argv);
    MainWindow w;
    w.show();
    return app.exec();
}
