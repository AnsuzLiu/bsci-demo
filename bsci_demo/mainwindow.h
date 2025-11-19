#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDir>
#include <QProcess>
#include <QtGui>
#include <QFrame>

#include <cstdlib>
#include <stack>

#include <qcap.h>
#include <qcap.linux.h>
#include <qcap.common.h>
#include <qcap2.h>
#include "qcap2.nvbuf.h"
#include "qcap2.cuda.h"
#include "qcap2.gst.h"
#include "qcap2.user.h"

#include <processinference.h>
#include <aspectratioframe.h>
#include <bmpfinder.h>

////// TIME INTERVAL

#define DISKCHECK_INTERVAL 2000 //ms

#define BMP_SCAN_INTERVAL 500 //ms

////// SOURCE

#define SOURCE_WIDTH 1920

#define SOURCE_HEIGHT 1080

#define MAX_CUDA_BUFFER_NUM 10

////// FRAME ASPECT RATIO

#define LIVE_FRAME_WIDTH 1324

#define LIVE_FRAME_HEIGHT 1026

#define INFER_FRAME_WIDTH 556

#define INFER_FRAME_HEIGHT 508

////// CROP SIZE

#define CROP_WIDTH 556

#define CROP_HEIGHT 508

class processinference;


struct free_stack_t : protected std::stack< std::function< void () >> {

    typedef free_stack_t self_t;

    typedef std::stack< std::function< void () > > parent_t;

    free_stack_t() {
    }

    ~free_stack_t() {

        if( ! empty() ) {

            printf( "%s(%d): unexpected value, size()=%ld \n", __FUNCTION__, __LINE__, size() );

        }

    }

    template<class FUNC>

    free_stack_t& operator +=( const FUNC& func ) {

        push( func );

        return *this;

    }

    void flush() {

        while( ! empty() ) {

            top()();

            pop();

        }

    }

};

struct callback_t {

    typedef callback_t self_t;

    typedef std::function<QRETURN ()> cb_func_t;

    cb_func_t func;

    template< class FUNC >

    callback_t( FUNC func ) : func( func ) {
    }

    static QRETURN _func( PVOID pUserData ) {

        self_t * pThis = ( self_t * )pUserData;

        return pThis->func();

    }

};

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

    QTimer *                st_pDiskUsageTimer;

    free_stack_t            st_oFreeStack;

    BYTE *                  st_pCUDABuffer_S[ MAX_CUDA_BUFFER_NUM ];

    qcap2_video_scaler_t *  st_pScaler_Live         = nullptr;

    qcap2_video_sink_t *    st_pSink_Live           = nullptr;

    BOOL                    st_bSinkState           = FALSE;

    qcap2_video_scaler_t *  st_pScaler_Crop         = nullptr;

    BOOL                    st_bStorageCropRaw      = FALSE;

    BOOL                    st_bDiskOverwrite       = FALSE;

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

    void Func_OutputBmp_Update( const QString &path );

    QRESULT Func_Live_Scaler_Init( free_stack_t& _FreeStack_, ULONG nCropX, ULONG nCropY, ULONG nCropW, ULONG nCropH, qcap2_video_scaler_t** ppVsca );

    QRESULT Func_Live_Sink_Init( free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nVideoFrameWidth, ULONG nVideoFrameHeight, QFrame *pFrame, qcap2_video_sink_t** ppVsink );

    QRESULT Func_Crop_Scaler_Init( free_stack_t& _FreeStack_, ULONG nCropX, ULONG nCropY, ULONG nCropW, ULONG nCropH, qcap2_video_scaler_t** ppVsca );

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

    void on_BTN_StorgeCropData_clicked();

private:

    Ui::MainWindow *ui;
    processinference* m_infer = nullptr;   // 加這一行成員

};

#endif // MAINWINDOW_H
