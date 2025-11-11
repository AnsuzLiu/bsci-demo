#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "setpassworddialog.h"
#include <QInputDialog>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

MainWindow * g_pMain = nullptr;

void Param_VA_Init( )
{

    struct InputOption {

        const char *    pszInputName;

        ULONG           nEnumIndex;

    };

    static const InputOption VideoOption_S[ ] = {

        { "COMPOSITE"          , QCAP_INPUT_TYPE_COMPOSITE },

        { "S_VIDEO"            , QCAP_INPUT_TYPE_SVIDEO },

        { "HDMI"               , QCAP_INPUT_TYPE_HDMI },

        { "DVI_D"              , QCAP_INPUT_TYPE_DVI_D },

        { "COMPONENTS (YCBCR)" , QCAP_INPUT_TYPE_COMPONENTS },

        { "DVI_A (RGB / VGA)"  , QCAP_INPUT_TYPE_DVI_A },

        { "SDI"                , QCAP_INPUT_TYPE_SDI },

        { "AUTO"               , QCAP_INPUT_TYPE_AUTO }

    };

    g_pMain->m_qMapVideoInput.clear();

    for( const auto& opt : VideoOption_S ) {

        g_pMain->m_qMapVideoInput.insert( opt.pszInputName, opt.nEnumIndex );

    }

    static const InputOption AudioOption_S[ ] = {

        { "EMBEDDED_AUDIO"          , QCAP_INPUT_TYPE_EMBEDDED_AUDIO },

        { "LINE_IN"                 , QCAP_INPUT_TYPE_LINE_IN },

        { "SOUNDCARD_MICROPHONE"    , QCAP_INPUT_TYPE_SOUNDCARD_MICROPHONE },

        { "SOUNDCARD_LINE_IN"       , QCAP_INPUT_TYPE_SOUNDCARD_LINE_IN }

    };

    g_pMain->m_qMapAudioInput.clear();

    for( const auto& opt : AudioOption_S ) {

        g_pMain->m_qMapAudioInput.insert( opt.pszInputName, opt.nEnumIndex );

    }

}


void Func_OutputFolder_Check( const QString &qszPath )
{

    QDir qDir( qszPath );

    if( qDir.exists() == FALSE ) {

        if( qDir.mkpath( "." ) == TRUE ) {

            printf( "[QCAP DEBUG] Output folder does not exist, creation successful\n" );

        } else {

            printf( "[QCAP DEBUG] Output folder does not exist, creation failed, close programe\n" );

            printf( "[QCAP DEBUG] Output folder Check - Fail\n" );

            std::exit(EXIT_FAILURE);

        }

    } else {

        printf( "[QCAP DEBUG] Output folder path exists\n" );

    }

    printf( "[QCAP DEBUG] Output folder Check - Pass\n" );

}


QRETURN on_process_signal_removed(PVOID pDevice, ULONG nVideoInput, ULONG nAudioInput, PVOID pUserData )
{

    Q_UNUSED( pDevice );

    ULONG nDeviceIndex = ( uintptr_t ) pUserData ;

    QString qszVideoName = g_pMain->m_qMapVideoInput.key( nVideoInput, "UNKNOWN" );

    printf( "[QCAP DEBUG] <CallBack> DEVICE %ld ( %s ) SIGNAL REMOVED \n", nDeviceIndex, qszVideoName.toUtf8().data() );

    QString qszAudioName = g_pMain->m_qMapAudioInput.key( nAudioInput, "UNKNOWN" );

    Q_UNUSED( qszAudioName );

    g_pMain->m_stParam_Device.st_nVideoWidth             = 0;

    g_pMain->m_stParam_Device.st_nVideoHeight            = 0;

    g_pMain->m_stParam_Device.st_bVideoIsInterleaved     = FALSE;

    g_pMain->m_stParam_Device.st_dVideoFrameRate         = 0.0;

    g_pMain->m_stParam_Device.st_nAudioChannels          = 0;

    g_pMain->m_stParam_Device.st_nAudioBitsPerSample     = 0;

    g_pMain->m_stParam_Device.st_nAudioSampleFrequency   = 0;

    return QCAP_RT_OK;

}


QRETURN on_process_no_signal_detected( PVOID pDevice, ULONG nVideoInput, ULONG nAudioInput, PVOID pUserData )
{

    Q_UNUSED( pDevice );

    ULONG nDeviceIndex = ( uintptr_t ) pUserData ;

    QString qszVideoName = g_pMain->m_qMapVideoInput.key( nVideoInput, "UNKNOWN" );

    printf( "[QCAP DEBUG] <CallBack> DEVICE %ld ( %s ) NO SIGNAL DETECTED \n", nDeviceIndex, qszVideoName.toUtf8().data()  );

    QString qszAudioName = g_pMain->m_qMapAudioInput.key( nAudioInput, "UNKNOWN" );

    Q_UNUSED( qszAudioName );

    return QCAP_RT_OK;

}


QRETURN on_process_format_changed( PVOID pDevice, ULONG nVideoInput, ULONG nAudioInput, ULONG nVideoWidth, ULONG nVideoHeight, BOOL bVideoIsInterleaved, double dVideoFrameRate, ULONG nAudioChannels, ULONG nAudioBitsPerSample,  ULONG nAudioSampleFrequency, PVOID pUserData )
{

    Q_UNUSED( pDevice );

    ULONG nDeviceIndex = ( uintptr_t ) pUserData ;

    QString qszVideoName = g_pMain->m_qMapVideoInput.key( nVideoInput, "UNKNOWN" );

    printf( "[QCAP DEBUG] <CallBack> DEVICE %ld ( %s ) FORMAT CHANGED DETECTED \n", nDeviceIndex, qszVideoName.toUtf8().data() );

    QString qszAudioName = g_pMain->m_qMapAudioInput.key( nAudioInput, "UNKNOWN" );

    Q_UNUSED( qszAudioName );

    g_pMain->m_stParam_Device.st_nVideoWidth            = nVideoWidth;

    g_pMain->m_stParam_Device.st_nVideoHeight           = nVideoHeight;

    g_pMain->m_stParam_Device.st_bVideoIsInterleaved    = bVideoIsInterleaved;

    g_pMain->m_stParam_Device.st_dVideoFrameRate        = dVideoFrameRate;

    UINT iVH = 0;

    CHAR strVideoFrameType[ 64 ]    = { 0 };

    if( bVideoIsInterleaved == TRUE ) {

        iVH = nVideoHeight / 2;

        sprintf( strVideoFrameType, "I" );

    } else {

        iVH = nVideoHeight;

        sprintf( strVideoFrameType, "P" );

    }

    g_pMain->m_stParam_Device.st_nAudioChannels         = nAudioChannels;

    g_pMain->m_stParam_Device.st_nAudioBitsPerSample    = nAudioBitsPerSample;

    g_pMain->m_stParam_Device.st_nAudioSampleFrequency  = nAudioSampleFrequency;

    char qszSourceInfo[ 256 ];

    snprintf( qszSourceInfo, sizeof( qszSourceInfo )
              , "INFO : %lu x %d %s %2.3f FPS , %lu CH x %lu BITS x %lu HZ"
              , nVideoWidth, iVH, strVideoFrameType, dVideoFrameRate
              , nAudioChannels, nAudioBitsPerSample, nAudioSampleFrequency );

    printf( "[QCAP DEBUG] <CallBack> %s\n", qszSourceInfo );

    return QCAP_RT_OK;

}


QRETURN on_process_video_preview( PVOID pDevice, double dSampleTime, BYTE* pFrameBuffer, ULONG nFrameBufferLen, PVOID pUserData)
{

    Q_UNUSED( pDevice );

    Q_UNUSED( dSampleTime );

    ULONG nDeviceIndex = ( uintptr_t ) pUserData ;

    Q_UNUSED( nDeviceIndex );

    if( g_pMain->m_stFunc_Device.st_bSinkState == TRUE
            &&g_pMain->m_stFunc_Device.st_pSink != nullptr ) {

        QRESULT QR = QCAP_RS_SUCCESSFUL;

        qcap2_rcbuffer_t* pSrcRCBuffer = qcap2_rcbuffer_cast( pFrameBuffer, nFrameBufferLen );

        std::shared_ptr<qcap2_rcbuffer_t> pDstRCBuffer = nullptr;

        if( g_pMain->m_stFunc_Device.st_bSinkState == TRUE
                &&g_pMain->m_stFunc_Device.st_pScaler != nullptr ) {

            qcap2_video_scaler_push( g_pMain->m_stFunc_Device.st_pScaler, pSrcRCBuffer );

            qcap2_rcbuffer_t * pTempBuffer = nullptr;

            qcap2_video_scaler_pop( g_pMain->m_stFunc_Device.st_pScaler, &pTempBuffer );

            pDstRCBuffer.reset( pTempBuffer, qcap2_rcbuffer_release );

        } else {

            pDstRCBuffer.reset( pSrcRCBuffer, qcap2_rcbuffer_release );

        }

        QR = qcap2_video_sink_push( g_pMain->m_stFunc_Device.st_pSink, pDstRCBuffer.get() );

        if( QR != QCAP_RS_SUCCESSFUL ) printf("[QCAP DEBUG] %s(%d): qcap2_video_sink_push ( Video Preview callback ) Failed ( %d )!!! \n", __FUNCTION__, __LINE__, QR );

    }

    return QCAP_RT_OK;

}


QRETURN on_process_audio_preview( PVOID pDevice, double dSampleTime, BYTE* pFrameBuffer, ULONG nFrameBufferLen, PVOID pUserData)
{

    Q_UNUSED( pDevice );

    Q_UNUSED( dSampleTime );

    Q_UNUSED( pFrameBuffer );

    Q_UNUSED( nFrameBufferLen );

    ULONG nDeviceIndex = ( uintptr_t ) pUserData ;

    Q_UNUSED( nDeviceIndex );

    return QCAP_RT_OK;

}


QRESULT new_video_cudahostbuf( free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nWidth, ULONG nHeight, unsigned int nFlags, qcap2_rcbuffer_t** ppRCBuffer ) {

    QRESULT qres = QCAP_RS_SUCCESSFUL;

    switch(1) { case 1:
        qcap2_rcbuffer_t* pRCBuffer = qcap2_rcbuffer_new_av_frame();
        _FreeStack_ += [pRCBuffer]() {
            qcap2_rcbuffer_delete(pRCBuffer);
        };

        qcap2_av_frame_t* pAVFrame = (qcap2_av_frame_t*)qcap2_rcbuffer_get_data(pRCBuffer);
        qcap2_av_frame_set_video_property(pAVFrame, nColorSpaceType, nWidth, nHeight);

        if(! qcap2_av_frame_alloc_cuda_host_buffer(pAVFrame, nFlags, 32, 1)) {
            qres = QCAP_RS_ERROR_OUT_OF_MEMORY;
            printf("[QCAP DEBUG] %s(%d): qcap2_av_frame_alloc_cuda_host_buffer() failed", __FUNCTION__, __LINE__);
            break;
        }
        _FreeStack_ += [pAVFrame]() {
            qcap2_av_frame_free_cuda_host_buffer(pAVFrame);
        };

        *ppRCBuffer = pRCBuffer;
    }

    return qres;

}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    m_infer = new processinference(ui->Frame_CropBMP);

    g_pMain = this;

    ////// Check Output Folder

    m_qszAppPath = QCoreApplication::applicationDirPath();

    m_qszOutputPath = m_qszAppPath + "/data/frames/";

    Func_OutputFolder_Check( m_qszOutputPath );


    ////// Auto Detect Disk Usage ( Per 2 Sec Snapshot )

    m_qtStorage = QStorageInfo( QDir( QCoreApplication::applicationFilePath() ) );

    m_stFunc_Device.st_pDiskUsageTimer = new QTimer( this );

    connect( m_stFunc_Device.st_pDiskUsageTimer, &QTimer::timeout, this, &MainWindow::Func_DiskUsage_Update );

    m_stFunc_Device.st_pDiskUsageTimer->start( DISKCHECK_INTERVAL );


    ////// Param List Init

    Param_VA_Init( );


    ////// Set QFrame Aspect Ratio

    ui->Frame_CropBMP->setAspectRatio( 508.0 / 556.0 );

    ui->Frame_Live->setAspectRatio( 1324.0 / 1025.0 );


    this->resize(1920, 1080);
    this->setFixedSize(1920, 1080);
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint );
    this->showFullScreen();

    sourceDir = "/home/rd/Documents/backup/";
    ui->progressBar->setValue(0);
    ui->labelStatus->setText("Waiting for USB Plugin...");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::checkUsb);
    timer->start(2000);

    // Check if the USB is already plugged in when the app starts
    QString usbPath = detectUsbPath();
    if (!usbPath.isEmpty()) {
        // USB detected, immediately start copying files
        ui->labelStatus->setText("USB already plugged in: " + usbPath);
        printf("Detected USB at startup: %s\n", usbPath.toStdString().c_str());
        lastUsbPath = usbPath;  // Set it as the last detected USB
        copyRecursively(sourceDir, usbPath); // Start copying files immediately
    }

    HwInitialize();

    Func_Live_Scaler_Init( m_stFunc_Device.st_oFreeStack, 0, 0, 1324, 1026, &m_stFunc_Device.st_pScaler );

    Func_Live_Sink_Init( m_stFunc_Device.st_oFreeStack, QCAP_COLORSPACE_TYPE_I420, 1324, 1026, ui->Frame_Live, &m_stFunc_Device.st_pSink );

}


void MainWindow::HwInitialize()
{

    //CREATE CAPTURE DEVICE

    char CapDevName[ ] = "SC0710 PCI";

    QCAP_CREATE( CapDevName, 0, NULL, &m_hDevice, TRUE, FALSE );

    if( m_hDevice != nullptr ) {

        ULONG nVcheck = QCAP_INPUT_TYPE_HDMI;

        ULONG nVcurrent = 0;

        QCAP_GET_VIDEO_INPUT( m_hDevice, &( nVcurrent ) );

        if( nVcurrent != nVcheck ) QCAP_SET_VIDEO_INPUT( m_hDevice, nVcheck );

        ULONG nAcheck = QCAP_INPUT_TYPE_EMBEDDED_AUDIO;

        ULONG nAcurrent = 0;

        QCAP_GET_AUDIO_INPUT( m_hDevice, &( nAcurrent ) );

        if( nAcurrent != nAcheck ) QCAP_SET_AUDIO_INPUT( m_hDevice, nAcheck );

        QCAP_REGISTER_NO_SIGNAL_DETECTED_CALLBACK( m_hDevice, on_process_no_signal_detected, ( PVOID ) ( ULONG ) 0 );

        QCAP_REGISTER_SIGNAL_REMOVED_CALLBACK( m_hDevice, on_process_signal_removed, ( PVOID ) ( ULONG ) 0 );

        QCAP_REGISTER_FORMAT_CHANGED_CALLBACK( m_hDevice, on_process_format_changed, ( PVOID ) ( ULONG ) 0 );

        QCAP_REGISTER_VIDEO_PREVIEW_CALLBACK( m_hDevice, on_process_video_preview, ( PVOID ) ( ULONG ) 0 );

        QCAP_REGISTER_AUDIO_PREVIEW_CALLBACK( m_hDevice, on_process_audio_preview, ( PVOID ) ( ULONG ) 0 );

        QCAP_SET_VIDEO_DEFAULT_OUTPUT_FORMAT( m_hDevice, m_stParam_Device.st_nVideoColorSpaceType, 0, 0, 0, 0 );

        QCAP_SET_DEVICE_CUSTOM_PROPERTY( m_hDevice, QCAP_DEVPROP_IO_METHOD, 1 );

        for( INT iBufferCount = 0; iBufferCount < MAX_CUDA_BUFFER_NUM; iBufferCount++ ) {

            QCAP_ALLOC_VIDEO_GPUDIRECT_PREVIEW_BUFFER( m_hDevice, &m_stFunc_Device.st_pCUDABuffer_S[ iBufferCount ], SOURCE_WIDTH * SOURCE_HEIGHT * 2 );

            QCAP_BIND_VIDEO_GPUDIRECT_PREVIEW_BUFFER( m_hDevice, iBufferCount, m_stFunc_Device.st_pCUDABuffer_S[ iBufferCount ], SOURCE_WIDTH * SOURCE_HEIGHT * 2 );

            printf( "[QCAP DEBUG] CUDA buffer id:%d, pointer:%p \n", iBufferCount, m_stFunc_Device.st_pCUDABuffer_S[ iBufferCount ] );

        }

        QCAP_RUN( m_hDevice );

    }

}


QString MainWindow::detectUsbPath()
{
    QStringList mountRoots = {"/media", "/run/media"};
    QString user = qgetenv("USER");

    for (const QString &root : mountRoots) {
        QDir base(root + "/" + user);
        if (!base.exists())
            continue;

        QStringList disks = base.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &d : disks) {
            QString full = base.absoluteFilePath(d);
            QFileInfo fi(full);
            if (fi.isDir() && fi.isWritable()) {
                return full;
            }
        }
    }
    return "";
}

bool MainWindow::copyRecursively(const QString &srcPath, const QString &dstPath)
{
//    printf("copy image\n");

    QDir srcDir(srcPath);
        QStringList files = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        if (files.isEmpty()) {
            return false; // nothing to move
        }

        ui->labelStatus->setText("Copying files...");
        ui->progressBar->setRange(0, 0);

        QProcess process;
        QString cmd = QString("bash -c \"mv '%1'/* '%2'/\"").arg(srcPath, dstPath);
        int ret = process.execute(cmd);

        ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(100);

        return (ret == 0);
}

void MainWindow::checkUsb()
{
    QString usbPath = detectUsbPath();

    if (!usbPath.isEmpty()) {
        // USB detected
        if (usbPath != lastUsbPath) {
            ui->labelStatus->setText("Detected USB: " + usbPath);
            printf("Detected USB: %s\n", usbPath.toStdString().c_str());
            lastUsbPath = usbPath;
        }

        // Continuously move files while USB is plugged in
        bool ok = copyRecursively(sourceDir, usbPath);
        if (ok) {
            ui->labelStatus->setText("✅ Files moved to: " + usbPath);
        } else {
            ui->labelStatus->setText("⚠️ No new files to move or copy failed");
        }
    }
    else if (usbPath.isEmpty() && !lastUsbPath.isEmpty()) {
        // USB removed
        ui->labelStatus->setText("USB Removed");
        ui->progressBar->setValue(0);
        lastUsbPath.clear();
    }
}

void MainWindow::HwUninitialize()
{

    //DESTROY CAPTURE DEVICE

    m_stFunc_Device.st_bSinkState = FALSE;

    m_stFunc_Device.st_oFreeStack.flush();

    if( m_hDevice != nullptr ) {

        QCAP_STOP( m_hDevice );

        QCAP_DESTROY( m_hDevice );

        m_hDevice = nullptr;

    }

}


MainWindow::~MainWindow()
{
    if (m_infer) {
        delete m_infer;
        m_infer = nullptr;
    }

    HwUninitialize();

    m_stParam_Device.st_nVideoWidth              = 0;

    m_stParam_Device.st_nVideoHeight             = 0;

    m_stParam_Device.st_bVideoIsInterleaved      = FALSE;

    m_stParam_Device.st_dVideoFrameRate          = 0.0;

    m_stParam_Device.st_nAudioChannels           = 0;

    m_stParam_Device.st_nAudioBitsPerSample      = 0;

    m_stParam_Device.st_nAudioSampleFrequency    = 0;

    delete ui;

}

void MainWindow::on_btn_changepassword_clicked()
{
    QFile file("config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    file.close();

    QString savedUser = obj["username"].toString();
    QString savedHash = obj["password_hash"].toString();

    bool okUser;
    QString oldUser = QInputDialog::getText(this, "Verify User", "Username:", QLineEdit::Normal, "", &okUser );

    bool okPass;
    QString oldPass = QInputDialog::getText(this, "Verify Password", "Password:", QLineEdit::Password, "", &okPass);

    if(!okUser || !okPass || oldPass.isEmpty() || oldUser.isEmpty()) {
        QMessageBox::critical(this, "Username or Password empty!!", "Please Try Again!!");
        return;
    }

    QByteArray oldHash = QCryptographicHash::hash(oldPass.toUtf8(), QCryptographicHash::Sha256).toHex();

    if(oldUser != savedUser || oldHash != savedHash) {
        QMessageBox::critical(this, "Username or Password Error!!", "Please Try Again!!");
        return;
    }

    SetPasswordDialog dlg(this);

    if(dlg.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "Success!!", "Username & Password has been changed!!");
    }
}


void MainWindow::Func_DiskUsage_Update()
{

    m_qtStorage.refresh();

    qint64 total = m_qtStorage.bytesTotal();

    qint64 free  = m_qtStorage.bytesAvailable();

    qint64 used  = total - free;

    double dUsedPercent = ( total > 0 ) ? ( double )used / total * 100.0 : 0.0;

    QString qszDiskInfoLabel = QString( "Disk info :   total: %1 GB, used: %2 GB" )
            .arg( total / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2 )
            .arg( used / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2 );

    ui->Label_DiskUsage->setText( qszDiskInfoLabel );

    ui->ProgressBar_DiskUsage->setValue( ( int )dUsedPercent );

    if( dUsedPercent >= 95 ) {

        ui->ProgressBar_DiskUsage->setStyleSheet( "QProgressBar::chunk {background-color: red;} QProgressBar{text-align: center;}" );

        printf( "[QCAP DEBUG] Disk usage exceeds 95%. Local storage saving shall continue, overwriting oldest data using FIFO buffer\n" );

        ui->Label_DiskInfo->setText( "Local storage saving shall continue, overwriting oldest data using FIFO buffer" );

    } else {

        ui->ProgressBar_DiskUsage->setStyleSheet( "" );

    }

}


QRESULT MainWindow::Func_Live_Scaler_Init( free_stack_t& _FreeStack_, ULONG nCropX, ULONG nCropY, ULONG nCropW, ULONG nCropH, qcap2_video_scaler_t** ppVsca )
{

    QRESULT qres = QCAP_RS_SUCCESSFUL;
    switch(1) { case 1:
        const int nBuffers = 4;
        const ULONG nColorSpaceType = QCAP_COLORSPACE_TYPE_I420;
        qcap2_video_scaler_t* pVsca = qcap2_video_scaler_new();
        _FreeStack_ += [&pVsca]() {
            qcap2_video_scaler_delete(pVsca);
            pVsca = nullptr;
        };

        qcap2_rcbuffer_t** pRCBuffers = new qcap2_rcbuffer_t*[nBuffers];
        _FreeStack_ += [pRCBuffers]() {
            delete[] pRCBuffers;
        };
        for(int i = 0;i < nBuffers;i++) {
            qcap2_rcbuffer_t* pRCBuffer;
            qres = new_video_cudahostbuf(_FreeStack_,
                                         nColorSpaceType, nCropW, nCropH, cudaHostAllocMapped, &pRCBuffer);
            if(qres != QCAP_RS_SUCCESSFUL) {
                printf("[QCAP DEBUG] %s(%d): __testkit__::new_video_cudahostbuf() failed, qres=%d", __FUNCTION__, __LINE__, qres);
                break;
            }
            pRCBuffers[i] = pRCBuffer;
        }
        qcap2_video_scaler_set_backend_type(pVsca, QCAP2_VIDEO_SCALER_BACKEND_TYPE_NPP);
        qcap2_video_scaler_set_multithread(pVsca, false);
        qcap2_video_scaler_set_frame_count(pVsca, nBuffers);
        qcap2_video_scaler_set_buffers(pVsca, &pRCBuffers[0]);
        qcap2_video_scaler_set_src_buffer_hint(pVsca, QCAP2_BUFFER_HINT_CUDAHOST);
        qcap2_video_scaler_set_dst_buffer_hint(pVsca, QCAP2_BUFFER_HINT_CUDAHOST);
        qcap2_video_scaler_set_crop(pVsca, nCropX, nCropY, nCropW, nCropH);
    {
        std::shared_ptr<qcap2_video_format_t> pVideoFormat(
                    qcap2_video_format_new(), qcap2_video_format_delete);

        qcap2_video_format_set_property(pVideoFormat.get(),
                                        nColorSpaceType, nCropW, nCropH, FALSE, 60.0);

        qcap2_video_scaler_set_video_format(pVsca, pVideoFormat.get());
    }
        qres = qcap2_video_scaler_start(pVsca);
        if(qres != QCAP_RS_SUCCESSFUL) {
            printf("[QCAP DEBUG] %s(%d): qcap2_video_scaler_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            break;
        }
        _FreeStack_ += [pVsca]() {
            QRESULT qres;
            qres = qcap2_video_scaler_stop(pVsca);
            if(qres != QCAP_RS_SUCCESSFUL) {
                printf("[QCAP DEBUG] %s(%d): qcap2_video_scaler_stop() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            }
        };
        *ppVsca = pVsca;
    }
    return qres;
}

QRESULT MainWindow::Func_Live_Sink_Init( free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nVideoFrameWidth, ULONG nVideoFrameHeight, QFrame *pFrame, qcap2_video_sink_t** ppVsink )
{

    QRESULT qres = QCAP_RS_SUCCESSFUL;

    switch(1) { case 1:
        qcap2_video_sink_t* pVsink = qcap2_video_sink_new();
        _FreeStack_ += [pVsink]() {
            qcap2_video_sink_delete(pVsink);
        };

        qcap2_video_sink_set_backend_type(pVsink, QCAP2_VIDEO_SINK_BACKEND_TYPE_GSTREAMER);

        qcap2_video_sink_set_gst_sink_name(pVsink, "xvimagesink");

        qcap2_video_sink_set_native_handle(pVsink, pFrame->winId());

    {
        std::shared_ptr<qcap2_video_format_t> pVideoFormat(
                    qcap2_video_format_new(), qcap2_video_format_delete);

        qcap2_video_format_set_property(pVideoFormat.get(),
                                        nColorSpaceType, nVideoFrameWidth, nVideoFrameHeight, FALSE, 60);

        qcap2_video_sink_set_video_format(pVsink, pVideoFormat.get());
    }

        qres = qcap2_video_sink_start(pVsink);
        if(qres != QCAP_RS_SUCCESSFUL) {
            printf("[QCAP DEBUG] %s(%d): qcap2_video_sink_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            break;
        } else {
            m_stFunc_Device.st_bSinkState = TRUE;
        }

        _FreeStack_ += [pVsink]() {
            QRESULT qres;

            qres = qcap2_video_sink_stop(pVsink);
            if(qres != QCAP_RS_SUCCESSFUL) {
                printf("[QCAP DEBUG] %s(%d): qcap2_video_sink_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            }
        };

        *ppVsink = pVsink;
    }

    return qres;
}
