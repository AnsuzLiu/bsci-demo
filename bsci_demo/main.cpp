#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include "logindialog.h"
#include "setpassworddialog.h"
#include "screenwatcher.h"

bool hasConfig() {
    QFile file("config.json");
    return file.exists();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    screenwatcher watcher;

    if (!hasConfig()) {
        SetPasswordDialog setDialog;
        if (setDialog.exec() != QDialog::Accepted)
        return 0;
    }

    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        MainWindow w;
        w.show();
        return a.exec();
    }

    return 0;
}
