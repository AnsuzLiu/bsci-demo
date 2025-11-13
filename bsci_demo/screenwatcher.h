#ifndef SCREENWATCHER_H
#define SCREENWATCHER_H

#include <QObject>
#include <QScreen>

#include <QGuiApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

class screenwatcher : public QObject
{
    Q_OBJECT
public:
    explicit screenwatcher(QObject *parent = nullptr);

signals:

public slots:
    void onScreenAdded(QScreen* screen);
    void onScreenRemoved(QScreen* screen);
    void onPollTimer();

private:
    void applyPolicyForScreen(QScreen* screen);
    bool setBestModeForOutput(const QString& outputName);
    QTimer m_timer;

};

#endif // SCREENWATCHER_H
