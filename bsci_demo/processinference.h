#ifndef PROCESSINFERENCE_H
#define PROCESSINFERENCE_H
#include <mainwindow.h>
#include <qcap2.nvbuf.h>
#include <qcap2.h>
#include <qcap.h>
#include <qcap2.cuda.h>
#include <qcap2.user.h>
#include <qcap2.gst.h>
#include <testkit.h>

#include <stdlib.h>
#include <fcntl.h>
#include <memory>
#include <functional>
#include <stack>
#include <cstring>
#include <fstream>
#include <thread>
#include <atomic>
#include <vector>
#include <stack>
#include <string>
#include <cmath>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <cstdio>
#include <cstdarg>

#include <sys/time.h>
#include <stdint.h>
#include <QFrame>

#define SNAPSHOT_ENABLE 1

class processinference : public __testkit__::TestCase
{
    public:
//    qszBMPOutputPath
        processinference(QFrame *frame, const QString &outputPath);
        ~processinference();
        QRETURN OnStart(__testkit__::free_stack_t& _FreeStack_, QRESULT& qres);
        QRESULT OnStartTimer(__testkit__::free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_video_scaler_t* pVsca, qcap2_rcbuffer_t* pVsrc);
        void sourceRGB(__testkit__::free_stack_t& _FreeStack_, qcap2_rcbuffer_t** ppRCBuffer);
        QRESULT StartVscaInferI420(__testkit__::free_stack_t& _FreeStack_, qcap2_video_scaler_t** ppVsca, qcap2_event_t* pEvent);
        QRESULT StartVscaInferVsink(__testkit__::free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nVideoFrameWidth, ULONG nVideoFrameHeight, qcap2_video_sink_t** ppVsink);

        __testkit__::free_stack_t mFreeStack;

//        qcap2_event_handlers_t* mEventHandlers = nullptr;
        qcap2_event_t* pEvent_infer_sca = nullptr;
        qcap2_video_scaler_t* pVsca_infer_i420 = nullptr;
        qcap2_video_sink_t* pVsink_infer = nullptr;

        bool bInferSink = false;
        QString     l_qszOutputPath;

    private:
        QFrame *m_frame = nullptr;
};

#endif // PROCESSINFERENCE_H
