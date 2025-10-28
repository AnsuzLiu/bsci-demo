#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include "logindialog.h"
#include "setpassworddialog.h"

bool hasConfig() {
    QFile file("config.json");
    return file.exists();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (!hasConfig()) {
        SetPasswordDialog setDialog;
        if (setDialog.exec() != QDialog::Accepted)
        return 0; // 未設定就結束
    }

    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        MainWindow w;
        w.show();
        return a.exec();
    }

    return 0;
}
