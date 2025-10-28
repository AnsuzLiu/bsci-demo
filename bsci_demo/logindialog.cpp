#include "logindialog.h"
#include "ui_logindialog.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <QRect>
#include <QDesktopWidget>
#include <QShowEvent>
#include <QWidget>
#include <QApplication>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle("Login");

    this->resize(1920, 1080);
    this->setFixedSize(1920, 1080);
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint );
    this->showFullScreen();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_onLogin_clicked()
{
    QString user = ui->username->text();
    QString pass = ui->password->text();

    QFile file("config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        reject();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    file.close();

    QString savedUser = obj["username"].toString();
    QString savedHash = obj["password_hash"].toString();

    QByteArray inputHash = QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex();

    if (user == savedUser && inputHash == savedHash)
        accept();
    else
        ui->statusLabel->setText("Username or Password Error !!");
}
