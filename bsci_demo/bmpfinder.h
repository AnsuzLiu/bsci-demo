#ifndef BMPFINDER_H
#define BMPFINDER_H

#include <QObject>
#include <QDir>
#include <QTimer>
#include <QtConcurrent>
#include <QSet>
#include <QStringList>
#include <QMetaObject>

#include <qcap.windef.h>

class BmpFinder : public QObject
{

    Q_OBJECT

public:

    explicit BmpFinder( const QString &path, int intervalMs = 1000, QObject *parent = nullptr );

signals:

    void Signal_Bmp_LatestFound( const QString &fullPath );

    void Signal_Bmp_Removed( const QString &fileName );

private slots:

    void Slot_Scan_Update();

private:

    void Func_Scan_InitFullScan();

private:

    QString         m_dirPath;

    QTimer *        m_ScanTimer = nullptr;

    QSet<QString>   m_knownFileSet;

};
#endif // BMPFINDER_H
