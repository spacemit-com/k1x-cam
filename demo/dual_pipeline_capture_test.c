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

#include "bufferPool.h"
#include "cam_list.h"
#include "cam_log.h"
#include "cam_interface.h"
#include "condition.h"
#include "config.h"
#include "cpp_common.h"
#include "sensor_common.h"
#include "viisp_common.h"

#define MAX_BUFFER_NUM   4
#define MAX_PIPELINE_NUM 2
#define MAX_FIRMWARE_NUM 2

typedef void* (*threadFunc)(void* param);
typedef struct {
    pthread_t threadId;
    char threadName[20];
    int threadRunning;
    threadFunc threadProcessFunc;
    struct condition cond;
    int pipelineId;
    int firmwareId;
} THREAD_INFO;

typedef struct spmVI_BUFFER_INFO {
    IMAGE_BUFFER_S* buffer;
    uint32_t frameId;
} spmVI_BUFFER_INFO_S;

typedef struct spmISP_BUFFER_INFO {
    FRAME_INFO_S frameInfo;
    uint32_t frameId;
} spmISP_BUFFER_INFO_S;

static LIST_HANDLE vi_out_list;
static LIST_HANDLE isp_out_list;
static LIST_HANDLE cpp_out_list;
static THREAD_INFO pipelineProcThread;
static THREAD_INFO rawProcessProcThread;
static THREAD_INFO cppProcessProcThread;
static BUFFER_POOL* vi_out_buffer_pool;
static BUFFER_POOL* cpp_out_buffer_pool;
static BUFFER_POOL* vi_rawdump_buffer_pool;
static int streamOnFlag = 0;
static int outputDumpFlag = 0;
static int takePictureFlag = 0;
static IMAGE_BUFFER_S frameInfoBuf[MAX_BUFFER_NUM];

static BUFFER_POOL* vi_out_buffer_capture_pool;
static BUFFER_POOL* cpp_out_buffer_capture_pool;
static BUFFER_POOL* vi_rawdump_buffer_capture_pool;
static IMAGE_BUFFER_S frameInfoCaptureBuf;
static int captureFrameId;
static int testAutoRunFlag = {0};
static struct condition testAutoRunCond;
static int dumpFrame = AUTO_FRAME_NUM;
static int testFrame = 2 * AUTO_FRAME_NUM;

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

static bool isp_buffer_list_find_ret(const void* item, const void* condition)
{
    spmISP_BUFFER_INFO_S* isp_buffer_info = (spmISP_BUFFER_INFO_S*)item;
    uint32_t* frameId = (uint32_t*)condition;

    return (isp_buffer_info->frameId == *frameId);
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
        if (!thread->threadRunning) {
            break;
        }
        // CLOG_INFO("thread handle");
        if ((List_IsEmpty(cpp_out_list) == false) && (List_IsEmpty(isp_out_list) == false)) {
            spmVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list);
            if (vi_buffer_info) {
                spmISP_BUFFER_INFO_S* isp_buffer_info =
                    List_FindItemIf(isp_out_list, isp_buffer_list_find_ret, &(vi_buffer_info->frameId));
                if (!isp_buffer_info) {
                    CLOG_WARNING("frameId mismatch");
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
        if (!thread->threadRunning) {
            break;
        }

        inputBuffer = &vi_rawdump_buffer_capture_pool->buffers[0];
        outputBuffer = &vi_out_buffer_capture_pool->buffers[0];
        metaBuffer = &frameInfoCaptureBuf;

        viisp_vi_queueBuffer(rawReadChannelId, inputBuffer);
        viisp_vi_queueBuffer(viChnId, outputBuffer);
        viisp_isp_triggerRawCapture(firmwareId, metaBuffer);
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
        if (!thread->threadRunning) {
            break;
        }
        inputBuf = &vi_out_buffer_capture_pool->buffers[0];
        outputBuf = &cpp_out_buffer_capture_pool->buffers[0];
        frameInfo = (FRAME_INFO_S*)frameInfoCaptureBuf.planes[0].virAddr;

        cpp_post_buffer(pipelineId, inputBuf, outputBuf, captureFrameId, frameInfo);
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

static char* path = "/tmp/";
static int image_buffer_save(const IMAGE_BUFFER_S* imgBuf, char* fileName)
{
    FILE* fp;

    fp = fopen(fileName, "w+");
    if (!fp) {
        printf("%s: %s open failed\n", __func__, fileName);
        return -1;
    }
    for (uint32_t i = 0; i < 2; i++) fwrite(imgBuf->planes[i].virAddr, imgBuf->planes[i].length, 1, fp);
    fclose(fp);
    CLOG_INFO("save img fileName %s", fileName);

    return 0;
}

static int raw_buffer_save(const IMAGE_BUFFER_S* imgBuf, char* fileName)
{
    FILE* fp;

    fp = fopen(fileName, "w+");
    if (!fp) {
        printf("%s: %s open failed\n", __func__, fileName);
        return -1;
    }
    fwrite(imgBuf->planes[0].virAddr, imgBuf->planes[0].length, 1, fp);
    fclose(fp);
    CLOG_INFO("save raw img fileName %s", fileName);

    return 0;
}

static int32_t vi_buffer_callback(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_buffer)
{
    IMAGE_BUFFER_S* buffer = vi_buffer->buffer;
    // uint64_t timeStamp = vi_buffer->timeStamp;
    uint32_t frameId = vi_buffer->frameId;
    // char success = vi_buffer->bValid ? 1 : 0;
    // char closeDone = vi_buffer->bCloseDown ? 1 : 0;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    CLOG_DEBUG("VI chn %d out buffer frameId %d, buffer %p", nChn, frameId, buffer);

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
        if (frameId == dumpFrame) {
            outputDumpFlag = 1;
            takePictureFlag = 1;
            viisp_vi_queueBuffer(2, &vi_rawdump_buffer_capture_pool->buffers[0]);
        }
        condition_post(&pipelineProcThread.cond);
        if (frameId == testFrame) {
            usleep(1000);
            condition_post(&testAutoRunCond);
        }
    } else if (nChn == 1) {
        CLOG_INFO("VI chn %d out buffer frameId %d, buffer %p", nChn, frameId, buffer);
        captureFrameId = frameId;
        condition_post(&cppProcessProcThread.cond);
    }

    return 0;
}

static int isp_buffer_callback(uint32_t pipelineID, void* pstFrameinfoBuf)
{
    IMAGE_BUFFER_S* buffer = (IMAGE_BUFFER_S*)pstFrameinfoBuf;
    FRAME_INFO_S* data = buffer->planes[0].virAddr;
    int frameId = data->frameId;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    CLOG_DEBUG("ISP pipelineID %d out buffer frameId %d", pipelineID, frameId);
    if (!streamOnFlag) {
        return 0;
    }

    isp_buffer_info = malloc(sizeof(spmISP_BUFFER_INFO_S));
    if (isp_buffer_info) {
        memcpy(&isp_buffer_info->frameInfo, data, sizeof(FRAME_INFO_S));
        isp_buffer_info->frameId = frameId;
        List_Push(isp_out_list, (void*)isp_buffer_info);
    }
    if (takePictureFlag) {
        FRAME_INFO_S* frameInfo = frameInfoCaptureBuf.planes[0].virAddr;
        memcpy(frameInfoCaptureBuf.planes[0].virAddr, buffer->planes[0].virAddr, buffer->planes[0].length);
        CLOG_INFO("save frameinfo for ISP pipelineID %d out buffer frameId %d", pipelineID, frameInfo->frameId);
    }
    condition_post(&pipelineProcThread.cond);
    viisp_isp_queueBuffer(pipelineID, buffer);

    return 0;
}

static int32_t cpp_buffer_callback(MPP_CHN_S mppCpp, const IMAGE_BUFFER_S* callbackBuf, char success)
{
    int i = 0;
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
            /*if (outputDumpFlag) {
                snprintf(fileName, sizeof(fileName), "%scpp%d_input_%dx%d_s%d.%s", path, mppCpp.devId,
                         callbackBuf->planes[0].width, callbackBuf->planes[0].height, callbackBuf->planes[0].stride,
                         suffix);
                image_buffer_save(callbackBuf, fileName);
                outputDumpFlag = 0;
            }*/
            if (mppCpp.devId == 1) {
                break;
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
                snprintf(fileName, sizeof(fileName), "%scpp%d_output_%dx%d_s%d.%s", path, mppCpp.devId,
                         callbackBuf->planes[0].width, callbackBuf->planes[0].height, callbackBuf->planes[0].stride,
                         suffix);
                image_buffer_save(callbackBuf, fileName);
                CLOG_INFO("capture image cpp process done");
                break;
            }

            if (outputDumpFlag) {
                snprintf(fileName, sizeof(fileName), "%scpp%d_output_%dx%d_s%d.%s", path, mppCpp.devId,
                         callbackBuf->planes[0].width, callbackBuf->planes[0].height, callbackBuf->planes[0].stride,
                         suffix);
                image_buffer_save(callbackBuf, fileName);
                outputDumpFlag = 0;
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
    // uint64_t timeStamp = vi_buffer->timeStamp;
    uint32_t frameId = vi_rawdump_buffer->frameId;
    // char success = vi_buffer->bValid ? 1 : 0;
    // char closeDone = vi_buffer->bCloseDown ? 1 : 0;
    char fileName[128];
    int pipelineId = nChn - VIU_MAX_CHN_NUM;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    CLOG_INFO("VI chn %d rawdump buffer frameId %d, buffer %p", nChn, frameId, buffer);

    if (buffer->planes[0].virAddr == vi_rawdump_buffer_pool->buffers[0].planes[0].virAddr) {
        snprintf(fileName, sizeof(fileName), "%sraw_output%d_frame%d_%dx%d.raw",
                 path, pipelineId, frameId, buffer->size.width, buffer->size.height);
        raw_buffer_save(buffer, fileName);
        buffer_pool_put_buffer(vi_rawdump_buffer_pool, &vi_rawdump_buffer_pool->buffers[0]);
    }
    if (buffer->planes[0].virAddr == vi_rawdump_buffer_capture_pool->buffers[0].planes[0].virAddr) {
        snprintf(fileName, sizeof(fileName), "%sraw_output%d_frame%d_%dx%d.raw",
                 path, pipelineId, frameId, buffer->size.width, buffer->size.height);
        raw_buffer_save(buffer, fileName);

        buffer_pool_put_buffer(vi_rawdump_buffer_capture_pool, &vi_rawdump_buffer_pool->buffers[0]);
        takePictureFlag = 0;
        condition_post(&rawProcessProcThread.cond);
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

    vi_rawdump_buffer_pool =
        create_buffer_pool(sensor_info.sensor_cfg->width, sensor_info.sensor_cfg->height,
                           toPixelFormatType(sensor_info.sensor_cfg->bitDepth), "vi rawdump channel0 out buffer");
    buffer_pool_alloc(vi_rawdump_buffer_pool, 1);

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
    buffer_pool_free(vi_rawdump_buffer_pool);
    destroy_buffer_pool(vi_rawdump_buffer_pool);

    return 0;
}

static int test_buffer_capture_init(IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info)
{
    // buffer init
    vi_out_buffer_capture_pool =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "vi channel1 out buffer");
    buffer_pool_alloc(vi_out_buffer_capture_pool, 1);

    frameinfo_buffer_alloc(&frameInfoCaptureBuf);

    cpp_out_buffer_capture_pool =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "cpp channel0 out buffer");
    buffer_pool_alloc(cpp_out_buffer_capture_pool, 1);

    vi_rawdump_buffer_capture_pool =
        create_buffer_pool(sensor_info.sensor_cfg->width, sensor_info.sensor_cfg->height,
                           toPixelFormatType(sensor_info.sensor_cfg->bitDepth), "vi rawdump channel0 out buffer");
    buffer_pool_alloc(vi_rawdump_buffer_capture_pool, 1);

    return 0;
}

static int test_buffer_capture_deInit()
{
    buffer_pool_free(vi_out_buffer_capture_pool);
    destroy_buffer_pool(vi_out_buffer_capture_pool);
    frameinfo_buffer_free(&frameInfoCaptureBuf);
    buffer_pool_free(cpp_out_buffer_capture_pool);
    destroy_buffer_pool(cpp_out_buffer_capture_pool);
    buffer_pool_free(vi_rawdump_buffer_capture_pool);
    destroy_buffer_pool(vi_rawdump_buffer_capture_pool);

    return 0;
}

/************************************************************************************************/
int dual_pipeline_capture_test(struct testConfig *config)
{
    int ret = 0;
    void* sensorHandle = NULL;
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

    if (!config)
        return -1;

    CLOG_INFO("test start");

    viChn0Id = pipeline0Id;
    viChn1Id = pipeline1Id;
    VIU_GET_RAW_CHN(pipeline0Id, rawdumpChnId);
    VIU_GET_RAW_READ_CHN(pipeline1Id, rawReadChannelId);

    // sensor init
    testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                    config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode);
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
    viisp_set_vi_callback(rawReadChannelId, NULL);
    viisp_set_vi_callback(viChn1Id, vi_buffer_callback);

    // cpp init
    cpp_init(pipeline0Id, img0_out_info, cpp_buffer_callback);
    cpp_init(pipeline1Id, img1_out_info, cpp_buffer_callback);

    // buffer init
    test_buffer_init(img0_out_info, sensor_info);
    test_buffer_capture_init(img1_out_info, sensor_info);

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
    outputDumpFlag = 0;
    takePictureFlag = 0;

    testFrame =  config->testFrame;
    dumpFrame = config->dumpFrame;

    if (config->autoRun == 1) {
        // CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

        testAutoRunFlag = 1;
        condition_init(&testAutoRunCond);

        cpp_start(pipeline1Id);
        viisp_vi_offline_streamOn(pipeline1Id);
        test_buffer_prepare(pipeline0Id, firmwareId);
        cpp_start(pipeline0Id);
        viisp_vi_online_streamOn(pipeline0Id);
        viisp_isp_streamOn(firmwareId);
        testSensorStart(sensorHandle);
        streamOnFlag = 1;
        CLOG_INFO("sensor stream on");

        condition_wait(&testAutoRunCond);

        streamOnFlag = 0;
        viisp_vi_offline_streamOff(pipeline1Id);
        cpp_stop(pipeline1Id);

        viisp_vi_online_streamOff(pipeline0Id);
        testSensorStop(sensorHandle);
        viisp_isp_streamOff(firmwareId);
        cpp_stop(pipeline0Id);
        test_buffer_reset(pipeline0Id);
        CLOG_INFO("sensor stream off");

        condition_deinit(&testAutoRunCond);
    } else {
        while (1) {
            char ch;
            CLOG_INFO("Input a character:");
            ch = getc(stdin);
            if (ch == 'q' || ch == 'Q') {
                CLOG_INFO("enter q exit");
                break;
            }
            if (ch == 's' || ch == 'S') {
                cpp_start(pipeline1Id);
                viisp_vi_offline_streamOn(pipeline1Id);

                test_buffer_prepare(pipeline0Id, firmwareId);
                cpp_start(pipeline0Id);
                viisp_vi_online_streamOn(pipeline0Id);
                viisp_isp_streamOn(firmwareId);
                testSensorStart(sensorHandle);
                streamOnFlag = 1;
                CLOG_INFO("sensor stream on");
                continue;
            }
            if (ch == 'c' || ch == 'C') {
                streamOnFlag = 0;
                viisp_vi_offline_streamOff(pipeline1Id);
                cpp_stop(pipeline1Id);

                viisp_vi_online_streamOff(pipeline0Id);
                testSensorStop(sensorHandle);
                viisp_isp_streamOff(firmwareId);
                cpp_stop(pipeline0Id);
                test_buffer_reset(pipeline0Id);
                CLOG_INFO("sensor stream off");
                continue;
            }
            if (ch == 'd' || ch == 'D') {
                outputDumpFlag = 1;
                CLOG_INFO("dump one frame");
                continue;
            }
            if (ch == 'r' || ch == 'R') {
                IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(vi_rawdump_buffer_pool);
                viisp_vi_queueBuffer(rawdumpChnId, buffer);
                CLOG_INFO("dump one raw frame");
                continue;
            }
            if (ch == 't' || ch == 'T') {
                takePictureFlag = 1;
                viisp_vi_queueBuffer(rawdumpChnId, &vi_rawdump_buffer_capture_pool->buffers[0]);
                CLOG_INFO("take picture");
                continue;
            }
        }
    }

    ProcThreadDeinit(&cppProcessProcThread);
    ProcThreadDeinit(&rawProcessProcThread);
    ProcThreadDeinit(&pipelineProcThread);

    viisp_isp_deinit(firmwareId, sensor_info.sensorId);
    viisp_vi_deInit();
    testSensorDeInit(sensorHandle);
    cpp_deInit(pipeline0Id);

    test_buffer_capture_deInit();
    test_buffer_deInit();
    CLOG_INFO("test end");

    return ret;
}
