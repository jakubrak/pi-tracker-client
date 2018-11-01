#include "connectdialog.h"
#include "ui_connectdialog.h"

#include <QKeyEvent>
#include <QRegExp>
#include <QRegExpValidator>

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->lineEditAddress->setValidator(ipValidator);

    QIntValidator *portValidator = new QIntValidator(0, 65535, this);
    ui->lineEditPort->setValidator(portValidator);

    //this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    this->setFixedSize(this->width(), this->height());

    connect(ui->pushButtonConnect, SIGNAL(released()), this, SLOT(connectButtonReleased()));
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

void ConnectDialog::connectButtonReleased() {
    ui->pushButtonConnect->setText("Connecting ...");
    ui->pushButtonConnect->setDisabled(true);
    emit connectToServer(ui->lineEditAddress->text(), ui->lineEditPort->text().toShort());
}

void ConnectDialog::closeEvent(QCloseEvent *e) {
    exit(0);
}

void ConnectDialog::keyPressEvent(QKeyEvent *e) {
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
    else {/* minimize */}
}

void ConnectDialog::connectionTimeout(int tryCount, int maxTryCount) {
    if (tryCount < maxTryCount) {
        int tryLeft = maxTryCount - tryCount;
        ui->pushButtonConnect->setText("Connecting (" + QString::number(tryLeft) + ") ...");
        ui->pushButtonConnect->setDisabled(true);
    } else {
        ui->pushButtonConnect->setText("Connect");
        ui->pushButtonConnect->setDisabled(false);
    }
}
