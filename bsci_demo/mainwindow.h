#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDir>
#include <QProcess>
#include <QtGui>

#include <cstdlib>

#include <qcap.h>
#include <qcap.linux.h>
#include <qcap.common.h>
#include <processinference.h>

struct SourceParam {

    ULONG   st_nVideoColorSpaceType     = QCAP_COLORSPACE_TYPE_NV12;

    ULONG   st_nVideoWidth              = 0;

    ULONG   st_nVideoHeight             = 0;

    BOOL    st_bVideoIsInterleaved      = FALSE;

    double  st_dVideoFrameRate          = 0.0;

    ULONG   st_nAudioChannels           = 0;

    ULONG   st_nAudioBitsPerSample      = 0;

    ULONG   st_nAudioSampleFrequency    = 0;

};

struct FunctionParam {

    QTimer *    st_pDiskUsageTimer;

    QTimer *    st_pSnapshotTimer;

    BOOL        st_bShareRecord_Live        = FALSE;

};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void checkUsb();
    QString detectUsbPath();
    bool copyRecursively(const QString &srcPath, const QString &dstPath);

    QTimer *timer;
    QString lastUsbPath;
    QString sourceDir;

    void HwInitialize();

    void HwUninitialize();

    void Func_DiskUsage_Update();

    void Func_Live_Preview();

    void Func_Live_Snapshot();

    //// DEVICE HANDLE

    PVOID                   m_hDevice;


    //// SOURCE PARAM

    SourceParam             m_stParam_Device;


    //// SOURCE FUNC

    FunctionParam           m_stFunc_Device;


    //// OTHER

    QString                 m_qszAppPath;

    QString                 m_qszOutputPath;

    QStorageInfo            m_qtStorage;

    QMap< QString, ULONG >  m_qMapVideoInput;

    QMap< QString, ULONG >  m_qMapAudioInput;




private slots:
    void on_btn_changepassword_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
