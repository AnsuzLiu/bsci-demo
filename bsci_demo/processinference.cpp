#include "processinference.h"

static QRETURN OnEvent_infer_sca(qcap2_video_scaler_t* pVsca, qcap2_video_sink_t* pVsink, PVOID pUserData) {

    processinference* m_pProcessinference = (processinference*)pUserData;

    QRESULT qres;
    QRETURN qret = QCAP_RT_OK;

    qcap2_rcbuffer_t* pRCBuffer_ = nullptr;

    qres = qcap2_video_scaler_pop(pVsca, &pRCBuffer_);
    if (qres != QCAP_RS_SUCCESSFUL || !pRCBuffer_) {
        LOGE("%s(%d): qcap2_video_scaler_pop() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return QCAP_RT_FAIL;
    }

    std::shared_ptr<qcap2_av_frame_t> pAVFrame_i420(
        (qcap2_av_frame_t*)qcap2_rcbuffer_lock_data(pRCBuffer_),
        [pRCBuffer_](qcap2_av_frame_t*) {
            qcap2_rcbuffer_unlock_data(pRCBuffer_);
        });

    qres = qcap2_cuda_device_synchronize();
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_cuda_device_synchronize() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return QCAP_RT_FAIL;
    }

/*
#if SNAPSHOT_ENABLE
    switch(1) { case 1:
        static int nIndex = 0;
        const char* strBase = "/home/nvidia/images";
        // const char* strBase = "images";
        char fn[PATH_MAX];

        if(nIndex > 30) break;

        sprintf(fn, "%s/snapshot-%02d.jpg", strBase, nIndex);
        if(nIndex++ >= 100) nIndex = 0;

        qres = qcap2_av_frame_store_picture(pAVFrame_i420.get(), fn);
        if(qres != QCAP_RS_SUCCESSFUL) {
            LOGE("%s(%d): qcap2_av_frame_store_picture() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            break;
        }
    }
#endif
    */

#if SNAPSHOT_ENABLE
    switch(1) { case 1:
        static int nIndex = 0;

        char fn[PATH_MAX];

        if(nIndex > 30) break;

        sprintf(fn, "%s/snapshot-%02d.jpg", m_pProcessinference->l_qszOutputPath.toUtf8().data(), nIndex);

        printf("[QCAP DEBUG] %s\n", fn);

        if(nIndex++ >= 100) nIndex = 0;

        qres = qcap2_av_frame_store_picture(pAVFrame_i420.get(), fn);
        if(qres != QCAP_RS_SUCCESSFUL) {
            LOGE("%s(%d): qcap2_av_frame_store_picture() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            break;
        }
    }
#endif

    qres = qcap2_video_sink_push(pVsink, pRCBuffer_);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_video_sink_push() failed, qres=%d", __FUNCTION__, __LINE__, qres);
    }
    qcap2_rcbuffer_release(pRCBuffer_);

    return qret;
}

static QRETURN OnEvent_Timer(qcap2_timer_t* pTimer, __testkit__::tick_ctrl_t* pTickCtrl, qcap2_video_scaler_t* pVsca, qcap2_rcbuffer_t* pVsrc) {
    QRESULT qres;

    int64_t now = _clk();
    uint64_t nExpirations;
    qres = qcap2_timer_wait(pTimer, &nExpirations);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_timer_wait() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return QCAP_RT_FAIL;
    }

    qres = qcap2_timer_next(pTimer, pTickCtrl->advance(now));
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_timer_next() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return QCAP_RT_FAIL;
    }

    if(pVsca) {
//        qcap2_print_video_frame_info(pVsrc, "vvv");
        qres = qcap2_video_scaler_push(pVsca, pVsrc);
    }

    return QCAP_RT_OK;
}

QRESULT processinference::OnStartTimer(__testkit__::free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_video_scaler_t* pVsca, qcap2_rcbuffer_t* pVsrc) {
    QRESULT qres = QCAP_RS_SUCCESSFUL;

    __testkit__::tick_ctrl_t* pTickCtrl = new __testkit__::tick_ctrl_t();
    _FreeStack_ += [pTickCtrl]() {
        delete pTickCtrl;
    };

    pTickCtrl->num = 30 * 1000LL;
    pTickCtrl->den = 1000LL;

    qcap2_timer_t* pTimer = qcap2_timer_new();
    _FreeStack_ += [pTimer]() {
        qcap2_timer_delete(pTimer);
    };

    qres = qcap2_timer_start(pTimer);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_timer_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return QCAP_RS_ERROR_GENERAL;
    }
    _FreeStack_ += [pTimer]() {
        QRESULT qres;

        qres = qcap2_timer_stop(pTimer);
        if(qres != QCAP_RS_SUCCESSFUL) {
            LOGE("%s(%d): qcap2_timer_stop() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        }
    };

    pTickCtrl->start(_clk());
    qres = qcap2_timer_next(pTimer, 4000);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_timer_next() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return QCAP_RS_ERROR_GENERAL;
    }

    __testkit__::AddTimerHandler(_FreeStack_, pEventHandlers, pTimer, std::bind(&OnEvent_Timer, pTimer, pTickCtrl, pVsca, pVsrc));

    return qres;
}

void processinference::sourceRGB(__testkit__::free_stack_t& _FreeStack_, qcap2_rcbuffer_t** ppRCBuffer) {
    QRESULT qres;
    qcap2_rcbuffer_t* pRCBuffer;
    qres = new_video_cudahostbuf(_FreeStack_, QCAP_COLORSPACE_TYPE_GBRP, 560, 560, cudaHostAllocMapped, &pRCBuffer);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): new_video_cudahostbuf() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return;
    }

    qres = qcap2_fill_video_test_pattern(pRCBuffer, QCAP2_TEST_PATTERN_0);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): qcap2_fill_video_test_pattern() failed, qres=%d", __FUNCTION__, __LINE__, qres);
    }

    *ppRCBuffer = pRCBuffer;
}

QRESULT processinference::StartVscaInferI420(__testkit__::free_stack_t& _FreeStack_, qcap2_video_scaler_t** ppVsca, qcap2_event_t *pEvent) {
    QRESULT qres = QCAP_RS_SUCCESSFUL;

    switch(1) { case 1:
        const int nBuffers = 2;
        const ULONG nColorSpaceType = QCAP_COLORSPACE_TYPE_I420;
        const ULONG nVideoFrameWidth = 560;
        const ULONG nVideoFrameHeight = 560;

        qcap2_video_scaler_t* pVsca = qcap2_video_scaler_new();
        _FreeStack_ += [pVsca]() {
            qcap2_video_scaler_delete(pVsca);
        };

        qcap2_rcbuffer_t** pRCBuffers = new qcap2_rcbuffer_t*[nBuffers];
        _FreeStack_ += [pRCBuffers]() {
            delete[] pRCBuffers;
        };
        for(int i = 0;i < nBuffers;i++) {
            qcap2_rcbuffer_t* pRCBuffer;
            qres = new_video_cudahostbuf(_FreeStack_,
                nColorSpaceType, nVideoFrameWidth, nVideoFrameHeight, cudaHostAllocMapped, &pRCBuffer);
            if(qres != QCAP_RS_SUCCESSFUL) {
                LOGE("%s(%d): new_video_cudahostbuf() failed, qres=%d", __FUNCTION__, __LINE__, qres);
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

        {
            std::shared_ptr<qcap2_video_format_t> pVideoFormat(
                qcap2_video_format_new(), qcap2_video_format_delete);

            qcap2_video_format_set_property(pVideoFormat.get(),
                nColorSpaceType, nVideoFrameWidth, nVideoFrameHeight, FALSE, 30.0);

            qcap2_video_scaler_set_video_format(pVsca, pVideoFormat.get());
        }

        qcap2_video_scaler_set_event(pVsca, pEvent);

        qres = qcap2_video_scaler_start(pVsca);
        if(qres != QCAP_RS_SUCCESSFUL) {
            LOGE("%s(%d): qcap2_video_scaler_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            break;
        }
        _FreeStack_ += [pVsca]() {
            QRESULT qres;

            qres = qcap2_video_scaler_stop(pVsca);
            if(qres != QCAP_RS_SUCCESSFUL) {
                LOGE("%s(%d): qcap2_video_scaler_stop() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            }
        };

        *ppVsca = pVsca;
    }
    return qres;
}

QRESULT processinference::StartVscaInferVsink(__testkit__::free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nVideoFrameWidth, ULONG nVideoFrameHeight, qcap2_video_sink_t** ppVsink) {
    QRESULT qres = QCAP_RS_SUCCESSFUL;

    switch(1) { case 1:
        qcap2_video_sink_t* pVsink = qcap2_video_sink_new();
        _FreeStack_ += [pVsink]() {
            qcap2_video_sink_delete(pVsink);
        };

        qcap2_video_sink_set_backend_type(pVsink, QCAP2_VIDEO_SINK_BACKEND_TYPE_GSTREAMER);
        qcap2_video_sink_set_gst_sink_name(pVsink, "xvimagesink");

        uintptr_t nHandle_win = m_frame->winId();

        qcap2_video_sink_set_native_handle(pVsink, nHandle_win);

        {
            std::shared_ptr<qcap2_video_format_t> pVideoFormat(
                qcap2_video_format_new(), qcap2_video_format_delete);

            qcap2_video_format_set_property(pVideoFormat.get(),
                nColorSpaceType, nVideoFrameWidth, nVideoFrameHeight, FALSE, 30);

            qcap2_video_sink_set_video_format(pVsink, pVideoFormat.get());
        }

        qres = qcap2_video_sink_start(pVsink);
        if(qres != QCAP_RS_SUCCESSFUL) {
            LOGE("%s(%d): qcap2_video_sink_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            break;
        }
        _FreeStack_ += [pVsink]() {
            QRESULT qres;

            qres = qcap2_video_sink_stop(pVsink);
            if(qres != QCAP_RS_SUCCESSFUL) {
                LOGE("%s(%d): qcap2_video_sink_start() failed, qres=%d", __FUNCTION__, __LINE__, qres);
            }
        };

        *ppVsink = pVsink;
    }
    return qres;
}

QRETURN processinference::OnStart(__testkit__::free_stack_t& _FreeStack_, QRESULT& qres) {
    __testkit__::NewEvent(mFreeStack, &pEvent_infer_sca);
    qres = StartVscaInferI420(_FreeStack_, &pVsca_infer_i420, pEvent_infer_sca);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): StartVsca() failed, qres=%d", __FUNCTION__, __LINE__, qres);
    }
    printf("pVsca_infer_i420 :%p \n", pVsca_infer_i420);
    StartVscaInferVsink(_FreeStack_, QCAP_COLORSPACE_TYPE_I420, 560, 560, &pVsink_infer);
    qres = __testkit__::AddEventHandler(mFreeStack, pEventHandlers, pEvent_infer_sca, std::bind(&OnEvent_infer_sca, pVsca_infer_i420, pVsink_infer, this));
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s[%d]AddEventHandler Failed", __FUNCTION__, __LINE__);
        return QCAP_RT_FAIL;
    }
    qcap2_rcbuffer_t* pRCBuffer_src;
    sourceRGB(_FreeStack_, &pRCBuffer_src);
    qres = OnStartTimer(_FreeStack_, pEventHandlers, pVsca_infer_i420, pRCBuffer_src);
    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s[%d]OnStartTimer Failed", __FUNCTION__, __LINE__);
        return QCAP_RT_FAIL;
    }

    return QCAP_RT_OK;
}

processinference::processinference(QFrame *frame, const QString &outputPath)
    : m_frame(frame),
      l_qszOutputPath(outputPath) {
    QRESULT qres = StartEventHandlers();

    if (qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): StartEventHandlers() failed, qres=%d", __FUNCTION__, __LINE__, qres);
        return;
    }

    QRESULT qres_evt = QCAP_RS_SUCCESSFUL;
    qres = ExecInEventHandlers(std::bind(&processinference::OnStart, this, std::ref(mFreeStack), std::ref(qres_evt)));

    if(qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): ExecInEventHandlers() failed, qres=%d", __FUNCTION__, __LINE__, qres);
    }
}

processinference::~processinference() {
    if (!pEventHandlers) {
        mFreeStack.flush();
        return;
    }

    QRESULT qres = ExecInEventHandlers([this]() -> QRETURN {
        mFreeStack.flush();
        return QCAP_RT_OK;
    });

    if (qres != QCAP_RS_SUCCESSFUL) {
        LOGE("%s(%d): ExecInEventHandlers(flush) failed, qres=%d",  __FUNCTION__, __LINE__, qres);
    }

    QRESULT qres2 = qcap2_event_handlers_stop(pEventHandlers);
        if (qres2 != QCAP_RS_SUCCESSFUL) {
            LOGE("%s(%d): qcap2_event_handlers_stop() failed, qres=%d",
                 __FUNCTION__, __LINE__, qres2);
        }

        qcap2_event_handlers_delete(pEventHandlers);
        pEventHandlers  = nullptr;
}
