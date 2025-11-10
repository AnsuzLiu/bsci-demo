#ifndef __TESTKIT_H__
#define __TESTKIT_H__

#define BUILD_WITH_NVBUF 1
#define BUILD_WITH_CUDA 1

#include "qcap2.h"
#include "qcap.linux.h"
#include "qcap2.user.h"

#if BUILD_WITH_NVBUF
#include "qcap2.nvbuf.h"
#endif

#if BUILD_WITH_CUDA
#include "qcap2.cuda.h"
#endif

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
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

#include <string.h>
#include <pthread.h>

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


namespace __testkit__ {

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

	inline void spinlock_lock(std::atomic_flag& lock) {
		while (lock.test_and_set(std::memory_order_acquire)) // acquire lock
		{
		// Since C++20, it is possible to update atomic_flag's
		// value only when there is a chance to acquire the lock.
		// See also: https://stackoverflow.com/questions/62318642
#if defined(__cpp_lib_atomic_flag_test)
			while (lock.test(std::memory_order_relaxed)) // test lock
#endif
			; // spin
		}
	}

	inline void spinlock_unlock(std::atomic_flag& lock) {
		lock.clear(std::memory_order_release);
	}

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

inline 	QRESULT NewEvent(free_stack_t& _FreeStack_, qcap2_event_t** ppEvent) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			qcap2_event_t* pEvent = qcap2_event_new();
			_FreeStack_ += [pEvent]() {
				qcap2_event_delete(pEvent);
			};

			qres = qcap2_event_start(pEvent);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_event_start() failed, qres=%d", qres);
				break;
			}
			_FreeStack_ += [pEvent]() {
				QRESULT qres;

				qres = qcap2_event_stop(pEvent);
				if(qres != QCAP_RS_SUCCESSFUL) {
					LOGE("%s(%d): qcap2_event_stop() failed, qres=%d", qres);
				}
			};

			*ppEvent = pEvent;
		}

		return qres;
	}

inline 	QRESULT StartEventHandlers(free_stack_t& _FreeStack_, qcap2_event_handlers_t** ppEventHandlers) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			qcap2_event_handlers_t* pEventHandlers = qcap2_event_handlers_new();
			_FreeStack_ += [pEventHandlers]() {
				qcap2_event_handlers_delete(pEventHandlers);
			};

			qres = qcap2_event_handlers_start(pEventHandlers);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_event_handlers_start() failed, qres=%d", qres);
				break;
			}
			_FreeStack_ += [pEventHandlers]() {
				QRESULT qres;

				qres = qcap2_event_handlers_stop(pEventHandlers);
				if(qres != QCAP_RS_SUCCESSFUL) {
					LOGE("%s(%d): qcap2_event_handlers_stop() failed, qres=%d", qres);
				}
			};

			*ppEventHandlers = pEventHandlers;
		}

		return qres;
	}

	template<class FUNC>
inline 	QRESULT ExecInEventHandlers(qcap2_event_handlers_t* pEventHandlers, FUNC func) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			std::shared_ptr<callback_t> pCallback(new callback_t(func));

			qres = qcap2_event_handlers_invoke(pEventHandlers,
				callback_t::_func, pCallback.get());
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_event_handlers_invoke() failed, qres=%d", __FUNCTION__, __LINE__,qres);
				break;
			}
		}

		return qres;
	}

	template<class FUNC>
inline 	QRESULT AddEventHandler(free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_event_t* pEvent, FUNC func) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			uintptr_t nHandle;
			qres = qcap2_event_get_native_handle(pEvent, &nHandle);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_event_get_native_handle() failed, qres=%d", qres);
				break;
			}

			callback_t* pCallback = new callback_t(func);
			_FreeStack_ += [pCallback]() {
				delete pCallback;
			};

			qres = qcap2_event_handlers_add_handler(pEventHandlers, nHandle,
				callback_t::_func, pCallback);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_event_handlers_add_handler() failed, qres=%d", qres);
				break;
			}
			_FreeStack_ += [pEventHandlers, nHandle]() {
				QRESULT qres;

				qres = qcap2_event_handlers_remove_handler(pEventHandlers, nHandle);
				if(qres != QCAP_RS_SUCCESSFUL) {
					LOGE("%s(%d): qcap2_event_handlers_remove_handler() failed, qres=%d", qres);
				}
			};
		}

		return qres;
	}

	template<class FUNC>
inline 	QRESULT AddTimerHandler(free_stack_t& _FreeStack_, qcap2_event_handlers_t* pEventHandlers, qcap2_timer_t* pTimer, FUNC func) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			uintptr_t nHandle;
			qres = qcap2_timer_get_native_handle(pTimer, &nHandle);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_timer_get_native_handle() failed, qres=%d", qres);
				break;
			}

			callback_t* pCallback = new callback_t(func);
			_FreeStack_ += [pCallback]() {
				delete pCallback;
			};

			qres = qcap2_event_handlers_add_handler(pEventHandlers, nHandle,
				callback_t::_func, pCallback);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_event_handlers_add_handler() failed, qres=%d", qres);
				break;
			}
			_FreeStack_ += [pEventHandlers, nHandle]() {
				QRESULT qres;

				qres = qcap2_event_handlers_remove_handler(pEventHandlers, nHandle);
				if(qres != QCAP_RS_SUCCESSFUL) {
					LOGE("%s(%d): qcap2_event_handlers_remove_handler() failed, qres=%d", qres);
				}
			};
		}

		return qres;
	}

inline 	QRESULT StartVsink_ximage(free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nVideoFrameWidth, ULONG nVideoFrameHeight,
		qcap2_window_t* pWindow, qcap2_video_sink_t** ppVsink) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			qcap2_video_sink_t* pVsink = qcap2_video_sink_new();
			_FreeStack_ += [pVsink]() {
				qcap2_video_sink_delete(pVsink);
			};

			qcap2_video_sink_set_backend_type(pVsink, QCAP2_VIDEO_SINK_BACKEND_TYPE_GSTREAMER);
			qcap2_video_sink_set_gst_sink_name(pVsink, "xvimagesink");

			uintptr_t nHandle_win;
			qres = qcap2_window_get_native_handle(pWindow, &nHandle_win);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_window_get_native_handle() failed, qres=%d", __FUNCTION__, __LINE__, qres);
				break;
			}

			qcap2_video_sink_set_native_handle(pVsink, nHandle_win);

			{
				std::shared_ptr<qcap2_video_format_t> pVideoFormat(
					qcap2_video_format_new(), qcap2_video_format_delete);

				qcap2_video_format_set_property(pVideoFormat.get(),
					nColorSpaceType, nVideoFrameWidth, nVideoFrameHeight, FALSE, 60);

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

inline 	QRESULT new_video_sysbuf(free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nWidth, ULONG nHeight, qcap2_rcbuffer_t** ppRCBuffer) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			qcap2_rcbuffer_t* pRCBuffer = qcap2_rcbuffer_new_av_frame();
			_FreeStack_ += [pRCBuffer]() {
				qcap2_rcbuffer_delete(pRCBuffer);
			};

			qcap2_av_frame_t* pAVFrame = (qcap2_av_frame_t*)qcap2_rcbuffer_get_data(pRCBuffer);
			qcap2_av_frame_set_video_property(pAVFrame, nColorSpaceType, nWidth, nHeight);

			if(! qcap2_av_frame_alloc_buffer(pAVFrame, 32, 1)) {
				qres = QCAP_RS_ERROR_OUT_OF_MEMORY;
				LOGE("%s(%d): qcap2_av_frame_alloc_buffer() failed", __FUNCTION__, __LINE__);
				break;
			}
			_FreeStack_ += [pAVFrame]() {
				qcap2_av_frame_free_buffer(pAVFrame);
			};

			*ppRCBuffer = pRCBuffer;
		}

		return qres;
	}

inline 	QRESULT new_audio_sysbuf(free_stack_t& _FreeStack_, ULONG nChannels, ULONG nSampleFmt, ULONG nSampleFrequency, ULONG nFrameSize, qcap2_rcbuffer_t** ppRCBuffer) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			qcap2_rcbuffer_t* pRCBuffer = qcap2_rcbuffer_new_av_frame();
			_FreeStack_ += [pRCBuffer]() {
				qcap2_rcbuffer_delete(pRCBuffer);
			};

			qcap2_av_frame_t* pAVFrame = (qcap2_av_frame_t*)qcap2_rcbuffer_get_data(pRCBuffer);
			qcap2_av_frame_set_audio_property(pAVFrame, nChannels, nSampleFmt, nSampleFrequency, nFrameSize);

			if(! qcap2_av_frame_alloc_buffer(pAVFrame, 32, 1)) {
				qres = QCAP_RS_ERROR_OUT_OF_MEMORY;
				LOGE("%s(%d): qcap2_av_frame_alloc_buffer() failed", __FUNCTION__, __LINE__);
				break;
			}
			_FreeStack_ += [pAVFrame]() {
				qcap2_av_frame_free_buffer(pAVFrame);
			};

			*ppRCBuffer = pRCBuffer;
		}

		return qres;
	}

#if BUILD_WITH_NVBUF
inline 	QRESULT new_video_nvbuf(free_stack_t& _FreeStack_, NvBufSurfaceCreateParams& oNVBufParam, qcap2_rcbuffer_t** ppRCBuffer) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;
		qcap2_rcbuffer_t* pRCBuffer_ret = NULL;

		switch(1) { case 1:
			pRCBuffer_ret = qcap2_rcbuffer_new_av_frame();
			_FreeStack_ += [pRCBuffer_ret]() {
				qcap2_rcbuffer_delete(pRCBuffer_ret);
			};

			qres = qcap2_rcbuffer_alloc_nvbuf(pRCBuffer_ret, &oNVBufParam);
			if(qres != QCAP_RS_SUCCESSFUL) {
				LOGE("%s(%d): qcap2_rcbuffer_alloc_nvbuf() failed, qres=%d", __FUNCTION__, __LINE__, qres);
				break;
			}
			_FreeStack_ += [pRCBuffer_ret]() {
				QRESULT qres;

				qres = qcap2_rcbuffer_free_nvbuf(pRCBuffer_ret);
				if(qres != QCAP_RS_SUCCESSFUL) {
					LOGE("%s(%d): qcap2_rcbuffer_free_nvbuf() failed, qres=%d", __FUNCTION__, __LINE__, qres);
				}
			};

			*ppRCBuffer = pRCBuffer_ret;
		}

		return qres;
	}
#endif // BUILD_WITH_NVBUF

#if BUILD_WITH_CUDA
inline 	QRESULT new_video_cudabuf(free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nWidth, ULONG nHeight, qcap2_rcbuffer_t** ppRCBuffer) {
		QRESULT qres = QCAP_RS_SUCCESSFUL;

		switch(1) { case 1:
			qcap2_rcbuffer_t* pRCBuffer = qcap2_rcbuffer_new_av_frame();
			_FreeStack_ += [pRCBuffer]() {
				qcap2_rcbuffer_delete(pRCBuffer);
			};

			qcap2_av_frame_t* pAVFrame = (qcap2_av_frame_t*)qcap2_rcbuffer_get_data(pRCBuffer);
			qcap2_av_frame_set_video_property(pAVFrame, nColorSpaceType, nWidth, nHeight);

			if(! qcap2_av_frame_alloc_cuda_buffer(pAVFrame, 32, 1)) {
				qres = QCAP_RS_ERROR_OUT_OF_MEMORY;
				LOGE("%s(%d): qcap2_av_frame_alloc_cuda_buffer() failed", __FUNCTION__, __LINE__);
				break;
			}
			_FreeStack_ += [pAVFrame]() {
				qcap2_av_frame_free_cuda_buffer(pAVFrame);
			};

			*ppRCBuffer = pRCBuffer;
		}

		return qres;
	}

inline QRESULT new_video_cudahostbuf(free_stack_t& _FreeStack_, ULONG nColorSpaceType, ULONG nWidth, ULONG nHeight, unsigned int nFlags, qcap2_rcbuffer_t** ppRCBuffer) {
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
				LOGE("%s(%d): qcap2_av_frame_alloc_cuda_host_buffer() failed", __FUNCTION__, __LINE__);
				break;
			}
			_FreeStack_ += [pAVFrame]() {
				qcap2_av_frame_free_cuda_host_buffer(pAVFrame);
			};

			*ppRCBuffer = pRCBuffer;
		}

		return qres;
	}
#endif // BUILD_WITH_CUDA

    struct TestCase {
        typedef TestCase self_t;

        free_stack_t _FreeStack_main_;
        free_stack_t _FreeStack_evt_;
        qcap2_event_handlers_t* pEventHandlers;

        QRESULT StartEventHandlers() {
            return __testkit__::StartEventHandlers(_FreeStack_main_, &pEventHandlers);
        }

        template<class FUNC>
        QRESULT ExecInEventHandlers(FUNC func) {
            return __testkit__::ExecInEventHandlers(pEventHandlers, func);
        }

        template<class FUNC>
        QRESULT AddEventHandler(free_stack_t& _FreeStack_, qcap2_event_t* pEvent, FUNC func) {
            return __testkit__::AddEventHandler(_FreeStack_, pEventHandlers, pEvent, func);
        }

        template<class FUNC>
        QRESULT AddTimerHandler(free_stack_t& _FreeStack_, qcap2_timer_t* pTimer, FUNC func) {
            return __testkit__::AddTimerHandler(_FreeStack_, pEventHandlers, pTimer, func);
        }

        QRESULT OnExitEventHandlers() {
            return ExecInEventHandlers([&]() -> QRETURN {
                _FreeStack_evt_.flush();

                return QCAP_RT_OK;
            });
        }
    };
};

#endif // __TESTKIT_H__
