#include "setpassworddialog.h"
#include "ui_setpassworddialog.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

SetPasswordDialog::SetPasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetPasswordDialog)
{
    ui->setupUi(this);
    setWindowTitle("SetUsername&Password");

    this->resize(1920, 1080);
    this->setFixedSize(1920, 1080);
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint );
    this->showFullScreen();
}

SetPasswordDialog::~SetPasswordDialog()
{
    delete ui;
}

void SetPasswordDialog::on_Save_clicked()
{
    QString user = ui->username->text();
    QString pass = ui->password->text();
    QString confirm = ui->confirm->text();

    if (pass != confirm) {
        ui->statusLabel->setText("Password Not Same !!");
        return;
    }

    QByteArray hash = QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex();

    QJsonObject obj;
    obj["username"] = user;
    obj["password_hash"] = QString(hash);

    QFile file("config.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
        accept();
    }
}
