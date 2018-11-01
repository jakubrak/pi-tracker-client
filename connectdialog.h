#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>

namespace Ui {
class ConnectDialog;
}

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = 0);
    ~ConnectDialog();

    void closeEvent(QCloseEvent *e);
    void keyPressEvent(QKeyEvent *e);

private slots:
    void connectButtonReleased();

public slots:
    void connectionTimeout(int tryCount, int maxTryCount);

signals:
    void connectToServer(QString address, short port);

private:
    Ui::ConnectDialog *ui;
};

#endif // CONNECTDIALOG_H
