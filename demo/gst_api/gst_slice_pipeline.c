/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdatomic.h>

#include "bufferPool.h"
#include "cam_list.h"
#include "cam_log.h"
#include "cam_interface.h"
#include "condition.h"
#include "config.h"
#include "cpp_common.h"
#include "sensor_common.h"
#include "viisp_common.h"
#include "gst_slice_pipeline.h"

#define MAX_BUFFER_NUM   4
#define MAX_CAP_BUFFER_NUM   8

#define MAX_PIPELINE_NUM 2
#define MAX_FIRMWARE_NUM 2

//#define DEBUG_USE_TIME

typedef enum {
    CAP_FRAMEINFO_CREATE = 0,
    CAP_FRAMEINFO_COPY,
    CAP_TRIGGER_RAW_BEGIN,
    CAP_TRIGGER_RAW_FINISH,
    CAP_CPP_POST_BEGIN,
    CAP_CPP_POST_FINISH,
    CAP_ISP_NUM,
} CAPTURE_STATE;

typedef enum {
    LIST_CAP_RAWDUMP = 0,
    LIST_CAP_VI,
    LIST_CAP_ISP,
    LIST_CAP_ISP_REPEAT,
    LIST_CAP_NUM,
} CAPTURE_LIST;

typedef struct spmISP_CAP_BUFFER_INFO {
    IMAGE_BUFFER_S buffer;
    int frameIspId;

    int frameViId;
    CAPTURE_STATE state;
} spmISP_CAP_BUFFER_INFO_S;

static LIST_HANDLE vi_out_list;
static LIST_HANDLE isp_out_list;
static LIST_HANDLE cpp_out_list;

static LIST_HANDLE rawdump_capture_list;
static LIST_HANDLE vi_capture_list;
static LIST_HANDLE rawdump_capture_back_list;
static LIST_HANDLE isp_capture_list;    //isp frameinfo buffer push, use in raw thread
static LIST_HANDLE isp_capture_repeat_list; //isp frameinfo buffer push, use in cpp thread
static LIST_HANDLE isp_capture_origin_list; //isp frameinfo buffer origin 

static THREAD_INFO pipelineProcThread;
static THREAD_INFO rawProcessProcThread;
static THREAD_INFO cppProcessProcThread;
static BUFFER_POOL* vi_out_buffer_pool;
static BUFFER_POOL* cpp_out_buffer_pool;
static int streamOnFlag = 0;
static IMAGE_BUFFER_S frameInfoBuf[MAX_BUFFER_NUM];

static BUFFER_POOL* vi_out_buffer_capture_pool;
static BUFFER_POOL* cpp_out_buffer_capture_pool;
static BUFFER_POOL* vi_rawdump_buffer_capture_pool;
static int dumpFrame = AUTO_FRAME_NUM;
static int testFrame = 2 * AUTO_FRAME_NUM;

static atomic_int flag_tirg = 0;
static long isp_capture_list_num = 0;

static int (*gst_get_cam_buffer)(IMAGE_BUFFER_S*, int);
static uint64_t get_timestamp(void)
{
    uint64_t tmp;
    struct timeval tv = {0};

    gettimeofday(&tv, NULL);
    tmp = tv.tv_sec;
    tmp = tmp * 1000000;
    tmp = tmp + tv.tv_usec;

    return tmp;
}
static int preview_cnt[MAX_PIPELINE_NUM*2] = {0};
static double viT1[MAX_PIPELINE_NUM*2] = {0};
static double viT2[MAX_PIPELINE_NUM*2] = {0};

/****************************************************************/
static PIXEL_FORMAT_E toPixelFormatType(int bitDepth)
{
    switch (bitDepth) {
        case 8:
            return PIXEL_FORMAT_RAW_8BPP;
        case 10:
            return PIXEL_FORMAT_RAW_10BPP;
        case 12:
            return PIXEL_FORMAT_RAW_12BPP;
        case 14:
            return PIXEL_FORMAT_RAW_14BPP;
        default:
            CLOG_ERROR("donot support pixel bitDepth %d", bitDepth);
            break;
    }
    return PIXEL_FORMAT_MAX;
}

static bool find_frameinfo_preview(const void* item, const void* condition)
{
    spmISP_BUFFER_INFO_S* isp_buffer_info = (spmISP_BUFFER_INFO_S*)item;
    uint32_t* frameId = (uint32_t*)condition;

    return (isp_buffer_info->frameId == *frameId);
}
// static bool find_frameinfo_raw_capture(const void* item, const void* condition)
// {
//     spmISP_CAP_BUFFER_INFO_S* isp_cap_buffer_info = (spmISP_CAP_BUFFER_INFO_S*)item;
//     uint32_t* frameId = (uint32_t*)condition;

//     return (isp_cap_buffer_info->frameIspId == *frameId);
// }

static bool find_frameinfo_frameid(const void* item, const void* condition)
{
    spmISP_CAP_BUFFER_INFO_S* isp_cap_buffer_info = (spmISP_CAP_BUFFER_INFO_S*)item;
    uint32_t* frameId = (uint32_t*)condition;

    return (isp_cap_buffer_info->frameViId == *frameId);
}
static int get_begin_item_frameid(LIST_HANDLE buffer_list, CAPTURE_LIST list_type)
{
    spmISP_CAP_BUFFER_INFO_S* isp_capture_buffer_info = NULL;
    IMAGE_BUFFER_S* buffer;
    int frameId = -1;

    List_Lock(buffer_list);
    if (list_type == LIST_CAP_RAWDUMP) {
        buffer = (IMAGE_BUFFER_S* )List_GetBeginItem(buffer_list);
        frameId = (buffer == NULL) ? -1 : buffer->frameId;
    } else if (list_type == LIST_CAP_ISP) {
        isp_capture_buffer_info = (spmISP_CAP_BUFFER_INFO_S* )List_GetBeginItem(buffer_list);
        frameId = (isp_capture_buffer_info == NULL) ? -1 : isp_capture_buffer_info->frameIspId;
    }
    List_Unlock(buffer_list);

    return frameId;
}

static spmISP_CAP_BUFFER_INFO_S* pop_near_item_to_capture(LIST_HANDLE buffer_list, uint32_t cur_id, CAPTURE_LIST list_type)
{
    spmISP_CAP_BUFFER_INFO_S* isp_capture_buffer_info = NULL;
    spmISP_CAP_BUFFER_INFO_S* ret_info = NULL;

    List_Lock(buffer_list);
    isp_capture_buffer_info = List_GetBeginItem(buffer_list);
    ret_info = isp_capture_buffer_info;

    while (isp_capture_buffer_info) {
        ret_info = isp_capture_buffer_info;

        if (list_type == LIST_CAP_ISP_REPEAT && isp_capture_buffer_info->frameViId >= cur_id) {
            break;
        }
        if (list_type == LIST_CAP_ISP && isp_capture_buffer_info->frameIspId >= cur_id) {
            break;
        }

        isp_capture_buffer_info = List_GetNextItem(buffer_list, isp_capture_buffer_info);
    }
    List_Unlock(buffer_list);
    if (ret_info)
        List_EraseByItem(buffer_list, ret_info);
    else
        CLOG_ERROR("list is empty");

    return ret_info;
}

static void release_unused_frameinfo_buffer(LIST_HANDLE buffer_list)
{
    spmISP_CAP_BUFFER_INFO_S* isp_capture_buffer_info;

    if (List_IsEmpty(buffer_list) == false) {
        do {
            isp_capture_buffer_info = List_Pop(buffer_list);
            if (isp_capture_buffer_info) {
                frameinfo_buffer_free(&isp_capture_buffer_info->buffer);
                free(isp_capture_buffer_info);
            }
        } while (isp_capture_buffer_info);
    }
}

static void* previewThreadFunc(void* param)
{
    THREAD_INFO* thread = (THREAD_INFO*)param;
    int pipelineId, firmwareId;

    if (!thread) {
        CLOG_ERROR("thread handle");
    }
    thread->threadRunning = 1;
    pipelineId = thread->pipelineId;
    firmwareId = thread->firmwareId;

    do {
        condition_timedwait(&thread->cond, 1000);
        if (!thread->threadRunning || streamOnFlag == 0) {
            break;
        }

        if ((List_IsEmpty(cpp_out_list) == false) && (List_IsEmpty(isp_out_list) == false)) {
            spmVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list);
            if (vi_buffer_info) {
                spmISP_BUFFER_INFO_S* isp_buffer_info =
                    List_FindItemIf(isp_out_list, find_frameinfo_preview, &(vi_buffer_info->frameId));
                if (!isp_buffer_info) {
                    CLOG_WARNING("frameId %d mismatch", vi_buffer_info->frameId);
                    isp_buffer_info = List_GetBeginItem(isp_out_list);
                }

                {
                    IMAGE_BUFFER_S* outputBuf = List_Pop(cpp_out_list);
                    IMAGE_BUFFER_S* inputBuf = vi_buffer_info->buffer;
                    FRAME_INFO_S* frameInfo = &isp_buffer_info->frameInfo;
                    int32_t frameId = vi_buffer_info->frameId;
                    CLOG_DEBUG("cpp_post_buffer inputBuf %p, outputBuf %p, frameId %d, frameInfo %p", inputBuf,
                               outputBuf, frameId, frameInfo);
                    cpp_post_buffer(pipelineId, inputBuf, outputBuf, frameId, frameInfo);
                }
                if (isp_buffer_info->frameId <= vi_buffer_info->frameId) {
                    List_EraseByItem(isp_out_list, isp_buffer_info);
                    free(isp_buffer_info);
                }
            }
            free(vi_buffer_info);
        }
    } while (thread->threadRunning);

    return NULL;
}

static void* rawProcessThreadFunc(void* param)
{
    THREAD_INFO* thread = (THREAD_INFO*)param;
    int pipelineId, firmwareId;
    int viChnId, rawReadChannelId;
    IMAGE_BUFFER_S* inputBuffer = NULL;
    IMAGE_BUFFER_S* outputBuffer = NULL;
    IMAGE_BUFFER_S* metaBuffer = NULL;
    spmISP_CAP_BUFFER_INFO_S* isp_capture_buffer_info;
    int frameId = 0;
    int begin_raw_frameid = 0;
    int begin_isp_frameid = 0;

    if (!thread) {
        CLOG_ERROR("thread handle");
    }
    thread->threadRunning = 1;
    pipelineId = thread->pipelineId;
    firmwareId = thread->firmwareId;
    viChnId = pipelineId;
    VIU_GET_RAW_READ_CHN(pipelineId, rawReadChannelId);

    do {
try_again:
        condition_wait(&thread->cond);
        if (!thread->threadRunning || streamOnFlag == 0) {
            break;
        }

        // if ((List_IsEmpty(rawdump_capture_list) == false) && atomic_load(&flag_tirg) == 1 && (List_IsEmpty(isp_capture_list) == false)) {
        begin_raw_frameid = get_begin_item_frameid(rawdump_capture_list, LIST_CAP_RAWDUMP);
        begin_isp_frameid = get_begin_item_frameid(isp_capture_list, LIST_CAP_ISP);
        if ((begin_raw_frameid != -1) && (begin_isp_frameid != -1)) {
            if (begin_isp_frameid > begin_raw_frameid + 15) {
                CLOG_WARNING("rawdump buffer is too old, begin_raw_frameid %d << begin_isp_frameid %d",
                    begin_raw_frameid, begin_isp_frameid);
                inputBuffer = List_Pop(rawdump_capture_list);
                viisp_vi_queueBuffer(2, inputBuffer);
                goto try_again;
            }
            if (atomic_load(&flag_tirg) == 0)
                goto try_again;

            outputBuffer = buffer_pool_get_buffer(vi_out_buffer_capture_pool);
            if (outputBuffer) {
                inputBuffer = List_Pop(rawdump_capture_list);

                frameId = inputBuffer->frameId;
                outputBuffer->frameId = frameId;

                isp_capture_buffer_info = pop_near_item_to_capture(isp_capture_list, frameId, LIST_CAP_ISP);
                isp_capture_buffer_info->state = CAP_TRIGGER_RAW_BEGIN;
                isp_capture_buffer_info->frameViId = outputBuffer->frameId;
                metaBuffer = &isp_capture_buffer_info->buffer;
#ifdef DEBUG_USE_TIME
                outputBuffer->viT1 = get_timestamp();
#endif
                viisp_vi_queueBuffer(rawReadChannelId, inputBuffer);
                viisp_vi_queueBuffer(viChnId, outputBuffer);
                viisp_isp_triggerRawCapture(firmwareId, metaBuffer);

                List_Push(rawdump_capture_back_list, (void*)inputBuffer);
                List_Push(isp_capture_repeat_list, (void*)isp_capture_buffer_info);
                atomic_store(&flag_tirg, 0);

                if (frameId != isp_capture_buffer_info->frameIspId)
                    CLOG_WARNING("frameId %d mismatch, size:%ld, id:%d",
                        frameId, List_GetSize(isp_capture_list), isp_capture_buffer_info->frameIspId);
				CLOG_DEBUG("raw frame id:%d, %d, size:%ld",
                    frameId, isp_capture_buffer_info->frameIspId, List_GetSize(isp_capture_repeat_list));
            }
        }

    } while (thread->threadRunning);

    return NULL;
}

static void* cppProcessThreadFunc(void* param)
{
    THREAD_INFO* thread = (THREAD_INFO*)param;
    int pipelineId, firmwareId;
    int viChnId, rawReadChannelId;
    IMAGE_BUFFER_S* outputBuf = NULL;
    IMAGE_BUFFER_S* inputBuf = NULL;
    FRAME_INFO_S* frameInfo = NULL;
    IMAGE_BUFFER_S* metaBuffer = NULL;
    spmISP_CAP_BUFFER_INFO_S* isp_capture_buffer_info;
    int frameId = 0;

    if (!thread) {
        CLOG_ERROR("thread handle");
    }
    thread->threadRunning = 1;
    pipelineId = thread->pipelineId;
    firmwareId = thread->firmwareId;
    viChnId = pipelineId;
    VIU_GET_RAW_READ_CHN(pipelineId, rawReadChannelId);

    do {
        condition_wait(&thread->cond);
        if (!thread->threadRunning || streamOnFlag == 0) {
            break;
        }

        if ((List_IsEmpty(vi_capture_list) == false)) {
            outputBuf = buffer_pool_get_buffer(cpp_out_buffer_capture_pool);
            if (outputBuf) {
                inputBuf = List_Pop(vi_capture_list);
                frameId = inputBuf->frameId;
                outputBuf->frameId = frameId;

                isp_capture_buffer_info = pop_near_item_to_capture(isp_capture_repeat_list, frameId, LIST_CAP_ISP_REPEAT);
                isp_capture_buffer_info->state = CAP_CPP_POST_BEGIN;
                metaBuffer = &isp_capture_buffer_info->buffer;
                frameInfo = (FRAME_INFO_S*)metaBuffer->planes[0].virAddr;
#ifdef DEBUG_USE_TIME
                outputBuf->viT1 = get_timestamp();
#endif
                cpp_post_buffer(pipelineId, inputBuf, outputBuf, frameId, frameInfo);
                isp_capture_buffer_info->state = CAP_FRAMEINFO_CREATE;
                List_Push(isp_capture_origin_list, (void *)isp_capture_buffer_info);
                CLOG_DEBUG("cpp post, frameId %d, size:%ld, id:%d,%d",
                    frameId, List_GetSize(isp_capture_repeat_list), isp_capture_buffer_info->frameViId, isp_capture_buffer_info->frameIspId);
            }
        }
    } while (thread->threadRunning);

    return NULL;
}

static void ProcThreadInit(THREAD_INFO* thread)
{
    pthread_attr_t attr;
    condition_init(&thread->cond);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread->threadId, &attr, thread->threadProcessFunc, thread);
    // pthread_setname_np(thread->threadId, thread->threadName);
    pthread_attr_destroy(&attr);
}

static void ProcThreadDeinit(THREAD_INFO* thread)
{
    thread->threadRunning = 0;
    condition_post(&thread->cond);
    pthread_join(thread->threadId, NULL);

    condition_deinit(&thread->cond);
}

/****************************************************************/

static int32_t vi_buffer_callback(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_buffer)
{
    IMAGE_BUFFER_S* buffer = vi_buffer->buffer;
    IMAGE_BUFFER_S* rawdump_buffer;
    uint32_t frameId = vi_buffer->frameId;
    uint32_t frameCapId = 0;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;
    IMAGE_BUFFER_S* rawBufferBack;
    int i, rawBufferBackCnt = 0, rawBufferQueueCnt = 0;
    double rawviT2;
    unsigned long long usetime = 0;
    static unsigned long long  usetime_sum = 0;
    static int count = 0;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    CLOG_DEBUG("VI chn %d out buffer frameId %d, buffer %p, w:%d, h:%d", nChn, frameId, buffer,  buffer->size.width, buffer->size.height);

    if (!streamOnFlag) {
        return 0;
    }

    if (nChn == 0) {
        vi_buffer_info = malloc(sizeof(spmVI_BUFFER_INFO_S));
        if (vi_buffer_info) {
            vi_buffer_info->buffer = buffer;
            vi_buffer_info->frameId = frameId;
            List_Push(vi_out_list, (void*)vi_buffer_info);
        }
        condition_post(&pipelineProcThread.cond);
    } else if (nChn == 1) {
        for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
            if (buffer->planes[0].virAddr == vi_out_buffer_capture_pool->buffers[i].planes[0].virAddr) {
                List_Push(vi_capture_list, (void*)&vi_out_buffer_capture_pool->buffers[i]);
#ifdef DEBUG_USE_TIME
                rawviT2 = get_timestamp();
                usetime = (unsigned long long)(rawviT2 - buffer->viT1);
                count++;
                usetime_sum += usetime;
#endif
                frameCapId = vi_out_buffer_capture_pool->buffers[i].frameId;
                condition_post(&cppProcessProcThread.cond);
                break;
            }
        }

        if (i == BUFFER_POOL_MAX_SIZE) {
            CLOG_ERROR("can't find valid vi out capture buffer");
        } else {
            while (1) {
                rawBufferBack = List_Pop(rawdump_capture_back_list);
                if (!rawBufferBack) {
                    break;
                }

                rawBufferBackCnt++;
                buffer_pool_put_buffer(vi_rawdump_buffer_capture_pool, rawBufferBack);
            }

            while (1) {
                rawdump_buffer = buffer_pool_get_buffer(vi_rawdump_buffer_capture_pool);
                if (!rawdump_buffer) {
                    break;
                }
                rawBufferQueueCnt++;
                viisp_vi_queueBuffer(2, rawdump_buffer);
            }
        }
        atomic_store(&flag_tirg, 1);
#ifdef DEBUG_USE_TIME
        {
            preview_cnt[nChn]++;
            if (preview_cnt[nChn] == 1)
                viT1[nChn] = (double)get_timestamp();
            if ((preview_cnt[nChn] % 101) == 0) {
                viT2[nChn] = (double)get_timestamp();
                CLOG_INFO("chn%d vi capture fps: %f", nChn,
                        (float)(100 / ((viT2[nChn] - viT1[nChn]) / 1000000)));
                viT1[nChn] = (double)get_timestamp();
            }
        }
        CLOG_INFO("VI chn%d frameId %d, cnt: %d, %d, rawnum: %ld, time: %llu us, fid: %d",
             nChn, frameId, rawBufferBackCnt, rawBufferQueueCnt, List_GetSize(rawdump_capture_list), usetime, frameCapId);
#else
        CLOG_INFO("VI chn%d frameId %d, cnt: %d, %d, rawnum: %ld, fid: %d",
             nChn, frameId, rawBufferBackCnt, rawBufferQueueCnt, List_GetSize(rawdump_capture_list), frameCapId);
#endif
    }

    return 0;
}

static int isp_buffer_callback(uint32_t pipelineID, void* pstFrameinfoBuf)
{
    int try_cnt = 0, ret;
    IMAGE_BUFFER_S* buffer = (IMAGE_BUFFER_S*)pstFrameinfoBuf;
    FRAME_INFO_S* data = buffer->planes[0].virAddr;
    int frameId = data->frameId;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmISP_CAP_BUFFER_INFO_S* isp_cap_buffer_info = NULL;
    IMAGE_BUFFER_S *fCaptureBuf;

    CLOG_DEBUG("ISP pipelineID %d out buffer frameId %d", pipelineID, frameId);
    if (!streamOnFlag) {
        return 0;
    }

    //for preview
    isp_buffer_info = malloc(sizeof(spmISP_BUFFER_INFO_S));
    if (isp_buffer_info) {
        memcpy(&isp_buffer_info->frameInfo, data, sizeof(FRAME_INFO_S));
        isp_buffer_info->frameId = frameId;
        List_Push(isp_out_list, (void*)isp_buffer_info);
    }

try_again:
    //for slice capture
    isp_cap_buffer_info = List_Pop(isp_capture_origin_list);
    if (isp_cap_buffer_info) {
        fCaptureBuf = &isp_cap_buffer_info->buffer;
        memcpy(fCaptureBuf->planes[0].virAddr, buffer->planes[0].virAddr, buffer->planes[0].length);
        isp_cap_buffer_info->frameIspId = frameId;
        isp_cap_buffer_info->frameViId = -1;
        isp_cap_buffer_info->state = CAP_FRAMEINFO_COPY;
        List_Push(isp_capture_list, (void*)isp_cap_buffer_info);
    } else {
        if (try_cnt == 0) {
            //release all CAP_FRAMEINFO_COPY frameinfo buffer to origin list
            CLOG_DEBUG("manual release frameinfo buffer");
            isp_cap_buffer_info = List_Pop(isp_capture_list);
            if (isp_cap_buffer_info) {
                isp_cap_buffer_info->state = CAP_FRAMEINFO_CREATE;
                List_Push(isp_capture_origin_list, (void *)isp_cap_buffer_info);
                try_cnt++;
                goto try_again;
            } else {
                CLOG_ERROR("no frameinfo buffer");
            }
        }
    }

    condition_post(&rawProcessProcThread.cond);
    condition_post(&cppProcessProcThread.cond);
    condition_post(&pipelineProcThread.cond);
    viisp_isp_queueBuffer(pipelineID, buffer);

    return 0;
}
static int32_t capture_cpp_buffer_callback(MPP_CHN_S mppCpp, const IMAGE_BUFFER_S* callbackBuf, char success)
{
    int ret, i = 0;
    char fileName[64], *suffix;
    int frameCapId;
    uint64_t cppviT2;
    unsigned long long usetime;
    static unsigned long long  usetime_sum = 0;
    static int count = 0;

    CLOG_DEBUG("mppCpp.modId %d, mppCpp.devId %d, mppCpp.chnId %d, callbackBuf %p", mppCpp.modId, mppCpp.devId,
               mppCpp.chnId, callbackBuf);
    if (!streamOnFlag) {
        return 0;
    }

    switch (callbackBuf->format) {
        case PIXEL_FORMAT_FBC:
            suffix = "afbc";
            break;
        case PIXEL_FORMAT_NV12:
        case PIXEL_FORMAT_NV12_DWT:
            suffix = "nv12";
            break;
        default:
            CLOG_ERROR("callback: invalid video frame PIXEL_FORMAT_E %d", callbackBuf->format);
            return 0;
    }

    switch (mppCpp.chnId) {
        case -1:
            if (mppCpp.devId == 1) {
                for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
                    if (callbackBuf->planes[0].virAddr == vi_out_buffer_capture_pool->buffers[i].planes[0].virAddr)
                        break;
                }
                if (i == BUFFER_POOL_MAX_SIZE) {
                    CLOG_ERROR("can't find valid vi capture out buffer");
                } else {
                    buffer_pool_put_buffer(vi_out_buffer_capture_pool, &vi_out_buffer_capture_pool->buffers[i]);
                    condition_post(&rawProcessProcThread.cond);
                }

                break;
            } else {
				CLOG_ERROR("invail case");
			}

            break;
        case 0:
            if (mppCpp.devId == 1) {
                for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
                    if (callbackBuf->planes[0].virAddr == cpp_out_buffer_capture_pool->buffers[i].planes[0].virAddr) {
#ifdef DEBUG_USE_TIME
                        cppviT2 = get_timestamp();
                        usetime = (unsigned long long)(cppviT2 - callbackBuf->viT1);
                        count++;
                        usetime_sum += usetime;
#endif
                        break;
                    }
                }
                if (i == BUFFER_POOL_MAX_SIZE) {
                    CLOG_ERROR("can't find valid cpp capture out buffer");
                } else {
                    frameCapId = cpp_out_buffer_capture_pool->buffers[i].frameId;
                    // CLOG_INFO("cpp out frameid %d, num vi:%ld, raw:%ld, cpp:%ld, num frameinfo:(%ld,%ld,%ld) ", frameCapId,
                    //     List_GetSize(vi_capture_list), List_GetSize(rawdump_capture_list), get_buffer_residue_num(cpp_out_buffer_capture_pool),
                    //     isp_capture_list_num, List_GetSize(isp_capture_repeat_list), List_GetSize(isp_capture_origin_list));
					ret = (*gst_get_cam_buffer)((IMAGE_BUFFER_S*) &cpp_out_buffer_capture_pool->buffers[i], i);
					if (ret)
						return -EINVAL;
                }
                {
                    preview_cnt[mppCpp.devId+2]++;
                    if (preview_cnt[mppCpp.devId+2] == 1)
                        viT1[mppCpp.devId+2] = (double)get_timestamp();
                    if ((preview_cnt[mppCpp.devId+2] % 101) == 0) {
                        viT2[mppCpp.devId+2] = (double)get_timestamp();
                        CLOG_INFO("chn%d cpp capture fps: %f", mppCpp.devId,
                                (float)(100 / ((viT2[mppCpp.devId+2] - viT1[mppCpp.devId+2]) / 1000000)));
                        viT1[mppCpp.devId+2] = (double)get_timestamp();
                    }
                }
                break;
            } else {
				CLOG_ERROR("invail case");
			}

            break;
        default:
            return -EINVAL;
    }

    return 0;
}
static int32_t cpp_buffer_callback(MPP_CHN_S mppCpp, const IMAGE_BUFFER_S* callbackBuf, char success)
{
    int i = 0;
    int ret = 0;
    char fileName[64], *suffix;

    CLOG_DEBUG("mppCpp.modId %d, mppCpp.devId %d, mppCpp.chnId %d, callbackBuf %p", mppCpp.modId, mppCpp.devId,
               mppCpp.chnId, callbackBuf);
    if (!streamOnFlag) {
        return 0;
    }

    switch (callbackBuf->format) {
        case PIXEL_FORMAT_FBC:
            suffix = "afbc";
            break;
        case PIXEL_FORMAT_NV12:
        case PIXEL_FORMAT_NV12_DWT:
            suffix = "nv12";
            break;
        default:
            CLOG_ERROR("callback: invalid video frame PIXEL_FORMAT_E %d", callbackBuf->format);
            return 0;
    }

    switch (mppCpp.chnId) {
        case -1:
            if (mppCpp.devId == 1) {
				CLOG_ERROR("invail case");
            }

            for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
                if (callbackBuf->planes[0].virAddr == vi_out_buffer_pool->buffers[i].planes[0].virAddr)
                    break;
            }
            if (i == BUFFER_POOL_MAX_SIZE) {
                CLOG_ERROR("can't find valid vi out buffer");
            }
            viisp_vi_queueBuffer(mppCpp.devId, (IMAGE_BUFFER_S*)&vi_out_buffer_pool->buffers[i]);
            break;
        case 0:
            if (mppCpp.devId == 1) {
				CLOG_ERROR("invail case");
            }

            for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
                if (callbackBuf->planes[0].virAddr == cpp_out_buffer_pool->buffers[i].planes[0].virAddr)
                    break;
            }
            if (i == BUFFER_POOL_MAX_SIZE) {
                CLOG_ERROR("can't find valid vi out buffer");
            }
            List_Push(cpp_out_list, (void*)&cpp_out_buffer_pool->buffers[i]);
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

static int32_t vi_rawdump_buffer_callback(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_rawdump_buffer)
{
    IMAGE_BUFFER_S* buffer = vi_rawdump_buffer->buffer;
    uint32_t frameId = vi_rawdump_buffer->frameId;
    char fileName[128];
    int pipelineId = nChn - VIU_MAX_CHN_NUM;
    int i = 0;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    CLOG_DEBUG("VI chn %d rawdump buffer frameId %d, buffer %p", nChn, frameId, buffer);

    for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
        if (buffer->planes[0].virAddr == vi_rawdump_buffer_capture_pool->buffers[i].planes[0].virAddr) {
            vi_rawdump_buffer_capture_pool->buffers[i].frameId = frameId;
            List_Push(rawdump_capture_list, (void*)&vi_rawdump_buffer_capture_pool->buffers[i]);

            condition_post(&rawProcessProcThread.cond);
            break;
        }
    }

    if (i == BUFFER_POOL_MAX_SIZE) {
        CLOG_ERROR("can't find valid vi rawdump capture buffer");
    }

    return 0;
}

static int test_buffer_init(IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info)
{
    int i = 0;

    // buffer list init
    vi_out_list = List_Create(0);
    isp_out_list = List_Create(0);
    cpp_out_list = List_Create(0);

    // buffer init
    vi_out_buffer_pool = create_buffer_pool(img_info.width, img_info.height, img_info.format, "vi channel0 out buffer");
    buffer_pool_alloc(vi_out_buffer_pool, MAX_BUFFER_NUM);

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_alloc(&frameInfoBuf[i]);
    }

    cpp_out_buffer_pool =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "cpp channel0 out buffer");
    buffer_pool_alloc(cpp_out_buffer_pool, MAX_BUFFER_NUM);

    return 0;
}

static int test_buffer_prepare(int pipelineId, int firmwareId)
{
    int i = 0;
    int viChnId = pipelineId;

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(vi_out_buffer_pool);
        viisp_vi_queueBuffer(viChnId, buffer);
    }
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        viisp_isp_queueBuffer(firmwareId, &frameInfoBuf[i]);
    }

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(cpp_out_buffer_pool);
        List_Push(cpp_out_list, (void*)buffer);
    }

    return 0;
}

static int test_buffer_reset(int pipelineId)
{
    int i = 0;

    List_Clear(vi_out_buffer_pool->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(vi_out_buffer_pool, &vi_out_buffer_pool->buffers[i]);
    }

    List_Clear(cpp_out_buffer_pool->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(cpp_out_buffer_pool, &cpp_out_buffer_pool->buffers[i]);
    }

    return 0;
}

static int test_buffer_deInit()
{
    int i = 0;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;

    List_Destroy(cpp_out_list);
    cpp_out_list = NULL;

    if (List_IsEmpty(isp_out_list) == false) {
        do {
            isp_buffer_info = List_Pop(isp_out_list);
            if (isp_buffer_info) {
                free(isp_buffer_info);
            }
        } while (isp_buffer_info);
    }
    List_Destroy(isp_out_list);
    isp_out_list = NULL;

    if (List_IsEmpty(vi_out_list) == false) {
        do {
            vi_buffer_info = List_Pop(vi_out_list);
            if (vi_buffer_info) {
                free(vi_buffer_info);
            }
        } while (vi_buffer_info);
    }
    List_Destroy(vi_out_list);
    vi_out_list = NULL;

    buffer_pool_free(vi_out_buffer_pool);
    destroy_buffer_pool(vi_out_buffer_pool);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_free(&frameInfoBuf[i]);
    }
    buffer_pool_free(cpp_out_buffer_pool);
    destroy_buffer_pool(cpp_out_buffer_pool);

    return 0;
}

static int test_buffer_capture_init(IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info, int (*gst_cam_buf_prepare)(void *, IMAGE_BUFFER_S*), void *gst_cam_buf_prepare_data)
{
    spmISP_CAP_BUFFER_INFO_S* isp_cap_buffer_info = NULL;
    IMAGE_BUFFER_S *fCaptureBuf;
    int i, ret = 0;
    IMAGE_BUFFER_S* buffer = NULL;

    // buffer list init
    rawdump_capture_list = List_Create(0);
    vi_capture_list = List_Create(0);
    rawdump_capture_back_list = List_Create(0);
    isp_capture_list = List_Create(0);
    isp_capture_repeat_list = List_Create(0);
    isp_capture_origin_list = List_Create(0);

    for (i = 0; i < 50; i++) {
        isp_cap_buffer_info = malloc(sizeof(spmISP_CAP_BUFFER_INFO_S));
        if (!isp_cap_buffer_info) {
            CLOG_ERROR("isp_cap_buffer_info malloc failed");
        }
        fCaptureBuf = &isp_cap_buffer_info->buffer;

        ret = frameinfo_buffer_alloc(fCaptureBuf);
        if (ret) {
            CLOG_ERROR("frameinfo_buffer_alloc failed");
        } else {
            isp_cap_buffer_info->frameIspId = -1;
            isp_cap_buffer_info->frameViId = -1;
            isp_cap_buffer_info->state = CAP_FRAMEINFO_CREATE;
            List_Push(isp_capture_origin_list, (void*)isp_cap_buffer_info);
        }
    }

    // buffer init
    vi_out_buffer_capture_pool =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "vi channel1 out buffer");
    buffer_pool_alloc(vi_out_buffer_capture_pool, MAX_CAP_BUFFER_NUM);

    cpp_out_buffer_capture_pool =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "cpp channel0 out buffer");
    buffer_pool_alloc(cpp_out_buffer_capture_pool, MAX_CAP_BUFFER_NUM);

    for (i = 0; i < MAX_CAP_BUFFER_NUM; i++) {
        buffer = &cpp_out_buffer_capture_pool->buffers[i];
        ret = gst_cam_buf_prepare(gst_cam_buf_prepare_data, buffer);
        if (ret) {
            CLOG_ERROR("gst_cam_buf_prepare return %d, error!", ret);
            return ret;
        }
    }

    vi_rawdump_buffer_capture_pool =
        create_buffer_pool(sensor_info.sensor_cfg->width, sensor_info.sensor_cfg->height,
                           toPixelFormatType(sensor_info.sensor_cfg->bitDepth), "vi rawdump channel0 out buffer");
    buffer_pool_alloc(vi_rawdump_buffer_capture_pool, MAX_BUFFER_NUM);

    return 0;
}

static int test_buffer_capture_deInit()
{
    release_unused_frameinfo_buffer(isp_capture_list);
    release_unused_frameinfo_buffer(isp_capture_repeat_list);
    release_unused_frameinfo_buffer(isp_capture_origin_list);

    List_Destroy(isp_capture_list);
    List_Destroy(isp_capture_repeat_list);
    List_Destroy(isp_capture_origin_list);
    List_Destroy(rawdump_capture_list);
    List_Destroy(vi_capture_list);
    List_Destroy(rawdump_capture_back_list);

    buffer_pool_free(vi_out_buffer_capture_pool);
    destroy_buffer_pool(vi_out_buffer_capture_pool);
    buffer_pool_free(cpp_out_buffer_capture_pool);
    destroy_buffer_pool(cpp_out_buffer_capture_pool);
    buffer_pool_free(vi_rawdump_buffer_capture_pool);
    destroy_buffer_pool(vi_rawdump_buffer_capture_pool);

    return 0;
}

void slice_pipeline_release_buffer(IMAGE_BUFFER_S* outputBuf, int index)
{
    buffer_pool_put_buffer(cpp_out_buffer_capture_pool, outputBuf);
    condition_post(&cppProcessProcThread.cond);
}

/************************************************************************************************/
int slice_pipeline_start(struct gstParam *para, struct testConfig *config)
{
    int i, ret = 0;
    void* sensorHandle = NULL;
    IMAGE_BUFFER_S* rawdump_buffer;

    SENSOR_MODULE_INFO sensor_info;
    int pipeline0Id = 0;
    int pipeline1Id = 1;
    int firmwareId = 0;
    int viChn0Id = 0;
    int viChn1Id = 0;
    int rawdumpChnId = 0;
    int rawReadChannelId = 0;
    IMAGE_INFO_S img0_out_info = {};
    IMAGE_INFO_S img1_in_info = {};
    IMAGE_INFO_S img1_out_info = {};

    CLOG_INFO("test start");

    atomic_store(&flag_tirg, 1);
    viChn0Id = pipeline0Id;
    viChn1Id = pipeline1Id;
    VIU_GET_RAW_CHN(pipeline0Id, rawdumpChnId);
    VIU_GET_RAW_READ_CHN(pipeline1Id, rawReadChannelId);

    // sensor init
    testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                    config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode,
                    config);
    testSensorGetDevInfo(sensorHandle, &sensor_info);
    img0_out_info.width = config->ispFeConfig[0].outWidth;
    img0_out_info.height = config->ispFeConfig[0].outHeight;
    img0_out_info.format = PIXEL_FORMAT_NV12_DWT;
    img1_in_info.width = sensor_info.sensor_cfg->width;
    img1_in_info.height = sensor_info.sensor_cfg->height;
    img1_in_info.format = toPixelFormatType(sensor_info.sensor_cfg->bitDepth);
    img1_out_info.width = sensor_info.sensor_cfg->width;
    img1_out_info.height = sensor_info.sensor_cfg->height;
    img1_out_info.format = PIXEL_FORMAT_NV12_DWT;

    // viisp init
    viisp_vi_init();
    viisp_vi_online_config(pipeline0Id, img0_out_info, &sensor_info);
    viisp_set_vi_callback(viChn0Id, vi_buffer_callback);
    viisp_isp_init(firmwareId, img0_out_info, &sensor_info, isp_buffer_callback, true);
    viisp_set_vi_callback(rawdumpChnId, vi_rawdump_buffer_callback);

    viisp_vi_offline_config(pipeline1Id, img1_out_info, img1_in_info, true);
    // viisp_set_vi_callback(rawReadChannelId, NULL);
    viisp_set_vi_callback(viChn1Id, vi_buffer_callback);

    // cpp init
    cpp_init(pipeline0Id, img0_out_info, cpp_buffer_callback);
    cpp_init(pipeline1Id, img1_out_info, capture_cpp_buffer_callback);

    // buffer init
    test_buffer_init(img0_out_info, sensor_info);
    test_buffer_capture_init(img1_out_info, sensor_info, para->gst_cam_buf_prepare, para->gst_cam_buf_prepare_data);

    // thread init
    strcpy(pipelineProcThread.threadName, "previewFunc");
    pipelineProcThread.threadProcessFunc = previewThreadFunc;
    pipelineProcThread.pipelineId = pipeline0Id;
    pipelineProcThread.firmwareId = firmwareId;
    ProcThreadInit(&pipelineProcThread);

    strcpy(rawProcessProcThread.threadName, "rawProcessFunc");
    rawProcessProcThread.threadProcessFunc = rawProcessThreadFunc;
    rawProcessProcThread.pipelineId = pipeline1Id;
    rawProcessProcThread.firmwareId = firmwareId;
    ProcThreadInit(&rawProcessProcThread);

    strcpy(cppProcessProcThread.threadName, "cppProcessFunc");
    cppProcessProcThread.threadProcessFunc = cppProcessThreadFunc;
    cppProcessProcThread.pipelineId = pipeline1Id;
    cppProcessProcThread.firmwareId = firmwareId;
    ProcThreadInit(&cppProcessProcThread);

    streamOnFlag = 0;

    testFrame =  config->testFrame;
    dumpFrame = config->dumpFrame;

    cpp_start(pipeline1Id);
    viisp_vi_offline_streamOn(pipeline1Id);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        rawdump_buffer = buffer_pool_get_buffer(vi_rawdump_buffer_capture_pool);
        viisp_vi_queueBuffer(2, rawdump_buffer);
    }
    CLOG_INFO("queue rawdump_buffer all");

    test_buffer_prepare(pipeline0Id, firmwareId);
	cpp_start(pipeline0Id);
    viisp_vi_online_streamOn(pipeline0Id);
    viisp_isp_streamOn(firmwareId);
    testSensorStart(sensorHandle);
    streamOnFlag = 1;

    CLOG_INFO("sensor stream on");

    para->sensorHandle = sensorHandle;
    para->sensorInfoId = sensor_info.sensorId;
    para->firmwareId = firmwareId;
    para->pipeline0Id = pipeline0Id;
    para->pipeline1Id = pipeline1Id;
    para->out_width = img1_out_info.width;
    para->out_height = img1_out_info.height;

    gst_get_cam_buffer = para->gst_get_cam_buffer;

    return ret;
}

int slice_pipeline_stop(struct gstParam *para)
{
    CLOG_INFO("sensor stream off");
    streamOnFlag = 0;
	usleep(1000);

    viisp_vi_offline_streamOff(para->pipeline1Id);
    cpp_stop(para->pipeline1Id);
    viisp_vi_online_streamOff(para->pipeline0Id);
    testSensorStop(para->sensorHandle);
    viisp_isp_streamOff(para->firmwareId);
    cpp_stop(para->pipeline0Id);
    test_buffer_reset(para->pipeline0Id);
    ProcThreadDeinit(&cppProcessProcThread);
    ProcThreadDeinit(&rawProcessProcThread);
    ProcThreadDeinit(&pipelineProcThread);

    viisp_isp_deinit(para->firmwareId, para->sensorInfoId);
    viisp_vi_deInit();
    testSensorDeInit(para->sensorHandle);
    cpp_deInit(para->pipeline0Id);

    test_buffer_capture_deInit();
    test_buffer_deInit();

    return 0;
}
