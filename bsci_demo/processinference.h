#ifndef PROCESSINFERENCE_H
#define PROCESSINFERENCE_H
#include <mainwindow.h>
#include <qcap2.nvbuf.h>
#include <qcap2.h>
#include <qcap.h>
#include <qcap2.cuda.h>
#include <qcap2.user.h>
#include <qcap2.gst.h>

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

static inline uint64_t _clk(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static inline void LOGE(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    printf("\033[31m");
    vprintf(fmt, args);

    printf("\033[0m\n");

    va_end(args);
}

struct free_stack_t : protected std::stack<std::function<void ()> > {
    typedef free_stack_t self_t;
    typedef std::stack<std::function<void ()> > parent_t;

    free_stack_t() {
    }

    ~free_stack_t() {
        if(! empty()) {
            LOGE("%s(%d): unexpected value, size()=%d", __FUNCTION__, __LINE__, size());
        }
    }

    template<class FUNC>
    free_stack_t& operator +=(const FUNC& func) {
        push(func);

        return *this;
    }

    void flush() {
        while(! empty()) {
            top()();
            pop();
        }
    }
};

struct callback_t {
    typedef callback_t self_t;
    typedef std::function<QRETURN ()> cb_func_t;

    cb_func_t func;

    template<class FUNC>
    callback_t(FUNC func) : func(func) {
    }

    static QRETURN _func(PVOID pUserData) {
        self_t* pThis = (self_t*)pUserData;

        return pThis->func();
    }
};

struct tick_ctrl_t {
    typedef tick_ctrl_t self_t;

    int num;
    int den;

    explicit tick_ctrl_t() {
    }

    void start(int64_t t) {
        nDenSecs = den * 1000000LL;
        nTimer = t;
    }

    int64_t advance(int64_t t) {
        int64_t nDiff = t - nTimer;
        int64_t nTicks = nDiff * num / nDenSecs;
        int64_t nNextTimer = nTimer + (nTicks + 1) * nDenSecs / num;

        nTimer += nTicks * nDenSecs / num;

        return nNextTimer - t;
    }

    int64_t nDenSecs;
    int64_t nTimer;
};

class processinference
{

public:
    processinference();
    processinference(QFrame *frame);
    QRETURN OnStart(free_stack_t& _FreeStack_, QRESULT& qres);
    QRESULT OnStartTimer(free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_video_scaler_t* pVsca);
    QRESULT NewEvent(free_stack_t& _FreeStack_, qcap2_event_t** ppEvent);

    QRESULT StartEventHandlers();
    template<class FUNC>
    QRESULT AddEventHandler(free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_event_t* pEvent, FUNC func);
    template<class FUNC>
    QRESULT AddTimerHandler(free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_timer_t* pTimer, FUNC func);
    QRESULT ExecInEventHandlers(std::function<QRETURN ()> cb);
    void sourceRGB(free_stack_t& _FreeStack_, qcap2_video_scaler_t** ppVsca);
    QRESULT StartVscaInferI420(free_stack_t& _FreeStack_, qcap2_video_scaler_t** ppVsca, qcap2_event_t* pEvent);
    QRESULT StartVscaInferVsink(free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nVideoFrameWidth, ULONG nVideoFrameHeight, qcap2_video_sink_t** ppVsink);
    QRESULT new_video_cudahostbuf(free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nWidth, ULONG nHeight, unsigned int nFlags, qcap2_rcbuffer_t** ppRCBuffer);

    free_stack_t mFreeStack;

    qcap2_event_handlers_t* mEventHandlers = nullptr;
    qcap2_event_t* pEvent_infer_sca = nullptr;
    qcap2_video_scaler_t* pVsca_infer_i420 = nullptr;
    qcap2_video_sink_t* pVsink_infer = nullptr;

    bool bInferSink = false;

private:
    QFrame *m_frame = nullptr;
};

#endif // PROCESSINFERENCE_H
