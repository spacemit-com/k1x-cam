/*
 * Copyright (C) 2023 ASR Micro Limited
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
#include "tuning_server.h"

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

typedef struct asrVI_BUFFER_INFO {
    IMAGE_BUFFER_S* buffer;
    uint32_t frameId;
} asrVI_BUFFER_INFO_S;

typedef struct asrISP_BUFFER_INFO {
    FRAME_INFO_S frameInfo;
    uint32_t frameId;
} asrISP_BUFFER_INFO_S;

static LIST_HANDLE vi_out_list[MAX_PIPELINE_NUM] = {};
static LIST_HANDLE isp_out_list[MAX_FIRMWARE_NUM] = {};
static LIST_HANDLE cpp_out_list[MAX_PIPELINE_NUM] = {};
static THREAD_INFO pipelineProcThread[MAX_PIPELINE_NUM];
static BUFFER_POOL* vi_out_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* cpp_out_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* vi_rawdump_buffer_pool[MAX_PIPELINE_NUM];
static int streamOnFlags[MAX_PIPELINE_NUM] = {};
static int outputDumpFlag[MAX_PIPELINE_NUM] = {};
static IMAGE_BUFFER_S frameInfoBuf[MAX_FIRMWARE_NUM][MAX_BUFFER_NUM];
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
    asrISP_BUFFER_INFO_S* isp_buffer_info = (asrISP_BUFFER_INFO_S*)item;
    uint32_t* frameId = (uint32_t*)condition;

    return (isp_buffer_info->frameId == *frameId);
}

static void* testThreadFunc(void* param)
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
        // CLOG_INFO("thread handle");
        if ((List_IsEmpty(cpp_out_list[pipelineId]) == false) && (List_IsEmpty(isp_out_list[firmwareId]) == false)) {
            asrVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list[pipelineId]);
            if (vi_buffer_info) {
                asrISP_BUFFER_INFO_S* isp_buffer_info =
                    List_FindItemIf(isp_out_list[firmwareId], isp_buffer_list_find_ret, &(vi_buffer_info->frameId));
                if (!isp_buffer_info) {
                    CLOG_WARNING("frameId mismatch");
                    isp_buffer_info = List_GetBeginItem(isp_out_list[firmwareId]);
                }

                {
                    IMAGE_BUFFER_S* outputBuf = List_Pop(cpp_out_list[pipelineId]);
                    IMAGE_BUFFER_S* inputBuf = vi_buffer_info->buffer;
                    FRAME_INFO_S* frameInfo = &isp_buffer_info->frameInfo;
                    int32_t frameId = vi_buffer_info->frameId;
                    CLOG_DEBUG("cpp_post_buffer inputBuf %p, outputBuf %p, frameId %d, frameInfo %p", inputBuf,
                               outputBuf, frameId, frameInfo);
                    cpp_post_buffer(pipelineId, inputBuf, outputBuf, frameId, frameInfo);
                }
                if (isp_buffer_info->frameId <= vi_buffer_info->frameId) {
                    List_EraseByItem(isp_out_list[firmwareId], isp_buffer_info);
                    free(isp_buffer_info);
                }
            }
            free(vi_buffer_info);
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
    pthread_create(&thread->threadId, &attr, testThreadFunc, thread);
    pthread_setname_np(thread->threadId, thread->threadName);
    pthread_attr_destroy(&attr);
}

static void ProcThreadDeinit(THREAD_INFO* thread)
{
    thread->threadRunning = 0;
    condition_post(&thread->cond);
    pthread_join(thread->threadId, NULL);

    condition_deinit(&thread->cond);
}

static char* path = "/data/vendor_de/camera/";
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
    asrVI_BUFFER_INFO_S* vi_buffer_info = NULL;
    int pipelineId = nChn;
    int streamOnFlag = 0;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }

    ispout_framerate_stat(nChn);

    CLOG_DEBUG("VI chn %d out buffer frameId %d, buffer %p", nChn, frameId, buffer);

    streamOnFlag = streamOnFlags[pipelineId];
    if (!streamOnFlag) {
        return 0;
    }
    // viisp_vi_queueBuffer(nChn, buffer);
    vi_buffer_info = malloc(sizeof(asrVI_BUFFER_INFO_S));
    if (vi_buffer_info) {
        vi_buffer_info->buffer = buffer;
        vi_buffer_info->frameId = frameId;
        List_Push(vi_out_list[pipelineId], (void*)vi_buffer_info);
    }
    condition_post(&pipelineProcThread[pipelineId].cond);

    return 0;
}

static int isp_buffer_callback(uint32_t pipelineID, void* pstFrameinfoBuf)
{
    IMAGE_BUFFER_S* buffer = (IMAGE_BUFFER_S*)pstFrameinfoBuf;
    FRAME_INFO_S* data = buffer->planes[0].virAddr;
    int frameId = data->frameId;
    asrISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    int streamOnFlag = 0;

    CLOG_DEBUG("ISP pipelineID %d out buffer frameId %d", pipelineID, frameId);
    streamOnFlag = streamOnFlags[pipelineID];
    if (!streamOnFlag) {
        return 0;
    }

    isp_buffer_info = malloc(sizeof(asrISP_BUFFER_INFO_S));
    if (isp_buffer_info) {
        memcpy(&isp_buffer_info->frameInfo, data, sizeof(FRAME_INFO_S));
        isp_buffer_info->frameId = frameId;
        List_Push(isp_out_list[pipelineID], (void*)isp_buffer_info);
    }
    condition_post(&pipelineProcThread[pipelineID].cond);
    viisp_isp_queueBuffer(pipelineID, buffer);

    return 0;
}

static int32_t cpp_buffer_callback(MPP_CHN_S mppCpp, const IMAGE_BUFFER_S* callbackBuf, char success)
{
    int i = 0;
    char fileName[64], *suffix;
    int streamOnFlag = 0;

    //CLOG_INFO("mppCpp.modId %d, mppCpp.devId %d, mppCpp.chnId %d, callbackBuf %p", mppCpp.modId, mppCpp.devId,
    //          mppCpp.chnId, callbackBuf);
    CLOG_DEBUG("mppCpp.modId %d, mppCpp.devId %d, mppCpp.chnId %d, callbackBuf %p", mppCpp.modId, mppCpp.devId,
              mppCpp.chnId, callbackBuf);

    streamOnFlag = streamOnFlags[mppCpp.devId];
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

            for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
                if (callbackBuf->planes[0].virAddr == vi_out_buffer_pool[mppCpp.devId]->buffers[i].planes[0].virAddr)
                    break;
            }
            if (i == BUFFER_POOL_MAX_SIZE) {
                CLOG_ERROR("can't find valid vi out buffer");
            }
            viisp_vi_queueBuffer(mppCpp.devId, (IMAGE_BUFFER_S*)&vi_out_buffer_pool[mppCpp.devId]->buffers[i]);
            break;
        case 0:
            if (outputDumpFlag[mppCpp.devId]) {
                snprintf(fileName, sizeof(fileName), "%scpp%d_output_%dx%d_s%d.%s", path, mppCpp.devId,
                         callbackBuf->planes[0].width, callbackBuf->planes[0].height, callbackBuf->planes[0].stride,
                         suffix);
                image_buffer_save(callbackBuf, fileName);
                outputDumpFlag[mppCpp.devId] = 0;
            }

            for (i = 0; i < BUFFER_POOL_MAX_SIZE; i++) {
                if (callbackBuf->planes[0].virAddr == cpp_out_buffer_pool[mppCpp.devId]->buffers[i].planes[0].virAddr)
                    break;
            }
            if (i == BUFFER_POOL_MAX_SIZE) {
                CLOG_ERROR("can't find valid vi out buffer");
            }
            List_Push(cpp_out_list[mppCpp.devId], (void*)&cpp_out_buffer_pool[mppCpp.devId]->buffers[i]);
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
    char fileName[64];
    int pipelineId = nChn - VIU_MAX_CHN_NUM;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    CLOG_INFO("VI chn %d rawdump buffer frameId %d, buffer %p", nChn, frameId, buffer);

    if (buffer->planes[0].virAddr == vi_rawdump_buffer_pool[pipelineId]->buffers[0].planes[0].virAddr) {
        snprintf(fileName, sizeof(fileName), "%sraw_output%d_%dx%d.raw", path, pipelineId, buffer->size.width,
                 buffer->size.height);
        image_buffer_save(buffer, fileName);
        buffer_pool_put_buffer(vi_rawdump_buffer_pool[pipelineId], &vi_rawdump_buffer_pool[pipelineId]->buffers[0]);
    }

    return 0;
}

static int test_buffer_init(int pipelineId, int firmwareId, IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info)
{
    int i = 0;

    // buffer list init
    vi_out_list[pipelineId] = List_Create(0);
    isp_out_list[firmwareId] = List_Create(0);
    cpp_out_list[pipelineId] = List_Create(0);

    // buffer init
    vi_out_buffer_pool[pipelineId] =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "vi channel0 out buffer");
    buffer_pool_alloc(vi_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_alloc(&frameInfoBuf[firmwareId][i]);
    }

    cpp_out_buffer_pool[pipelineId] =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "cpp channel0 out buffer");
    buffer_pool_alloc(cpp_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

    vi_rawdump_buffer_pool[pipelineId] =
        create_buffer_pool(sensor_info.sensor_cfg->width, sensor_info.sensor_cfg->height,
                           toPixelFormatType(sensor_info.sensor_cfg->bitDepth), "vi rawdump channel0 out buffer");
    buffer_pool_alloc(vi_rawdump_buffer_pool[pipelineId], 1);

    return 0;
}

static int test_buffer_prepare(int pipelineId, int firmwareId)
{
    int i = 0;
    int viChnId = pipelineId;

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(vi_out_buffer_pool[pipelineId]);
        viisp_vi_queueBuffer(viChnId, buffer);
    }
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        viisp_isp_queueBuffer(firmwareId, &frameInfoBuf[firmwareId][i]);
    }

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(cpp_out_buffer_pool[pipelineId]);
        List_Push(cpp_out_list[pipelineId], (void*)buffer);
    }

    return 0;
}

static int test_buffer_reset(int pipelineId)
{
    int i = 0;

    List_Clear(vi_out_buffer_pool[pipelineId]->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(vi_out_buffer_pool[pipelineId], &vi_out_buffer_pool[pipelineId]->buffers[i]);
    }

    List_Clear(cpp_out_buffer_pool[pipelineId]->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(cpp_out_buffer_pool[pipelineId], &cpp_out_buffer_pool[pipelineId]->buffers[i]);
    }

    return 0;
}

static int test_buffer_deInit(int pipelineId, int firmwareId)
{
    int i = 0;
    asrISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    asrVI_BUFFER_INFO_S* vi_buffer_info = NULL;

    List_Destroy(cpp_out_list[pipelineId]);
    cpp_out_list[pipelineId] = NULL;

    if (List_IsEmpty(isp_out_list[firmwareId]) == false) {
        do {
            isp_buffer_info = List_Pop(isp_out_list[firmwareId]);
            if (isp_buffer_info) {
                free(isp_buffer_info);
            }
        } while (isp_buffer_info);
    }
    List_Destroy(isp_out_list[firmwareId]);
    isp_out_list[firmwareId] = NULL;

    if (List_IsEmpty(vi_out_list[pipelineId]) == false) {
        do {
            vi_buffer_info = List_Pop(vi_out_list[pipelineId]);
            if (vi_buffer_info) {
                free(vi_buffer_info);
            }
        } while (vi_buffer_info);
    }
    List_Destroy(vi_out_list[pipelineId]);
    vi_out_list[pipelineId] = NULL;

    buffer_pool_free(vi_out_buffer_pool[pipelineId]);
    destroy_buffer_pool(vi_out_buffer_pool[pipelineId]);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_free(&frameInfoBuf[firmwareId][i]);
    }
    buffer_pool_free(cpp_out_buffer_pool[pipelineId]);
    destroy_buffer_pool(cpp_out_buffer_pool[pipelineId]);
    buffer_pool_free(vi_rawdump_buffer_pool[pipelineId]);
    destroy_buffer_pool(vi_rawdump_buffer_pool[pipelineId]);

    return 0;
}

/************************************************************************************************/
int single_pipeline_online_test(int sensorId)
{
    int ret = 0;
    TESTCONFIG testConfig = {};
    void* sensorHandle = NULL;
    SENSOR_MODULE_INFO sensor_info;
    int pipelineId = 0;
    int firmwareId = 0;
    int viChnId = 0;
    int rawdumpChnId = 0;
    IMAGE_INFO_S img_info = {};
    struct tuning_objs_config tuning_cfg = {0};

    CLOG_INFO("test start");

    memset(&testConfig, 0, sizeof(TESTCONFIG));
    testConfig.pipeConfig[0].workMode = TEST_WORKMODE_ONLINE;
    switch (sensorId) {
    case 0:
        testConfig.pipeConfig[0].sensorId = 0;
        strcpy(testConfig.pipeConfig[0].sensorName, "imx135_asr");
        //strcpy(testConfig.pipeConfig[0].sensorName, "os05a10_asr");
        testConfig.pipeConfig[0].out_width = 1920;
        testConfig.pipeConfig[0].out_height = 1080;
        break;
    case 1:
        testConfig.pipeConfig[0].sensorId = 1;
        strcpy(testConfig.pipeConfig[0].sensorName, "gc2375h_asr");
        testConfig.pipeConfig[0].out_width = 1600;
        testConfig.pipeConfig[0].out_height = 1200;
        break;
    case 2:
        testConfig.pipeConfig[0].sensorId = 2;
        strcpy(testConfig.pipeConfig[0].sensorName, "s5k5e3yx_asr");
        testConfig.pipeConfig[0].out_width = 1920;
        testConfig.pipeConfig[0].out_height = 1080;
        break;
    default:
        CLOG_ERROR("invalid sensorId, expected: [0, 1, 2]\n");
        return -1;
        break;
    }

    testConfig.pipeConfig[1].workMode = TEST_WORKMODE_INVALID;
    testConfig.tuningServer = 1;

    // sensor init
    ret = testSensorInit(&sensorHandle, testConfig.pipeConfig[0].sensorName, testConfig.pipeConfig[0].sensorId);
    if (ret) {
        CLOG_ERROR("testSensorInit failed\n");
        return ret;
    }
    ret = testSensorGetDevInfo(sensorHandle, &sensor_info);
    if (ret) {
        CLOG_ERROR("testSensorGetDevInfo failed\n");
        testSensorDeInit(sensorHandle);
        return ret;
    }
    img_info.width = testConfig.pipeConfig[0].out_width;
    img_info.height = testConfig.pipeConfig[0].out_height;
    img_info.format = PIXEL_FORMAT_NV12_DWT;

    // viisp init
    viisp_vi_init();
    viisp_vi_online_config(pipelineId, img_info, &sensor_info);
    viChnId = pipelineId;
    viisp_set_vi_callback(viChnId, vi_buffer_callback);
    viisp_isp_init(firmwareId, img_info, &sensor_info, isp_buffer_callback, false);
    VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
    viisp_set_vi_callback(rawdumpChnId, vi_rawdump_buffer_callback);

    // cpp init
    cpp_init(pipelineId, img_info, cpp_buffer_callback);

    // buffer init
    test_buffer_init(pipelineId, firmwareId, img_info, sensor_info);

    // thread init
    strcpy(pipelineProcThread[pipelineId].threadName, "pipeline0Func");
    pipelineProcThread[pipelineId].threadProcessFunc = testThreadFunc;
    pipelineProcThread[pipelineId].pipelineId = pipelineId;
    pipelineProcThread[pipelineId].firmwareId = firmwareId;
    ProcThreadInit(&pipelineProcThread[pipelineId]);

    streamOnFlags[pipelineId] = 0;
    outputDumpFlag[pipelineId] = 0;

    if (testConfig.tuningServer) {
        // be consistent with isp firmwareId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_ISP0] = 1;
        // be consistent with cpp pipelineId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_CPP0] = 1;
        tuning_server_init(tuning_cfg);
    }

    while (1) {
        char ch;
        CLOG_INFO("Input a character:");
        ch = getc(stdin);
        if (ch == 'q' || ch == 'Q') {
            CLOG_INFO("enter q exit");
            break;
        }
        if (ch == 's' || ch == 'S') {
            test_buffer_prepare(pipelineId, firmwareId);
            cpp_start(pipelineId);
            viisp_vi_online_streamOn(pipelineId);
            viisp_isp_streamOn(firmwareId);
            testSensorStart(sensorHandle);
            streamOnFlags[pipelineId] = 1;
            CLOG_INFO("sensor stream on");
            continue;
        }
        if (ch == 'c' || ch == 'C') {
            streamOnFlags[pipelineId] = 0;
            viisp_vi_online_streamOff(pipelineId);
            testSensorStop(sensorHandle);
            viisp_isp_streamOff(firmwareId);
            cpp_stop(pipelineId);
            test_buffer_reset(pipelineId);
            CLOG_INFO("sensor stream off");
            continue;
        }
        if (ch == 'd' || ch == 'D') {
            outputDumpFlag[pipelineId] = 1;
            CLOG_INFO("dump one frame");
            continue;
        }
        if (ch == 'r' || ch == 'R') {
            IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(vi_rawdump_buffer_pool[pipelineId]);
            viisp_vi_queueBuffer(rawdumpChnId, buffer);
            CLOG_INFO("dump one raw frame");
            continue;
        }
    }

    if (testConfig.tuningServer)
        tuning_server_deinit();

    ProcThreadDeinit(&pipelineProcThread[pipelineId]);

    viisp_isp_deinit(firmwareId, sensor_info.sensorId);
    viisp_vi_deInit();

    test_buffer_deInit(pipelineId, firmwareId);

    cpp_deInit(pipelineId);

    if (testConfig.pipeConfig[0].workMode != TEST_WORKMODE_OFFLINE) {
        testSensorDeInit(sensorHandle);
    }
    CLOG_INFO("test end");

    return ret;
}

static int online_test_viisp_init(PIPELINECONFIG* config, int pipelineId, int firmwareId,
                                  SENSOR_MODULE_INFO sensor_info)
{
    IMAGE_INFO_S img_info = {};
    int viChnId = 0;
    int rawdumpChnId = 0;

    img_info.width = config->out_width;
    img_info.height = config->out_height;
    img_info.format = PIXEL_FORMAT_NV12_DWT;

    viisp_vi_online_config(pipelineId, img_info, &sensor_info);
    viChnId = pipelineId;
    viisp_set_vi_callback(viChnId, vi_buffer_callback);
    viisp_isp_init(firmwareId, img_info, &sensor_info, isp_buffer_callback, false);
    VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
    viisp_set_vi_callback(rawdumpChnId, vi_rawdump_buffer_callback);

    // cpp init
    cpp_init(pipelineId, img_info, cpp_buffer_callback);

    // buffer init
    test_buffer_init(pipelineId, firmwareId, img_info, sensor_info);

    return 0;
}

static int online_test_viisp_deInit(int pipelineId, int firmwareId, SENSOR_MODULE_INFO sensor_info)
{
    viisp_isp_deinit(firmwareId, sensor_info.sensorId);
    cpp_deInit(pipelineId);
    test_buffer_deInit(pipelineId, firmwareId);

    return 0;
}

static int online_test_viisp_rawDump(int pipelineId)
{
    int rawdumpChnId = 0;

    VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
    IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(vi_rawdump_buffer_pool[pipelineId]);
    viisp_vi_queueBuffer(rawdumpChnId, buffer);

    return 0;
}

static int online_test_viisp_streamOn(void* sensorHandle, int pipelineId, int firmwareId)
{
    test_buffer_prepare(pipelineId, firmwareId);
    cpp_start(pipelineId);
    viisp_vi_online_streamOn(pipelineId);
    viisp_isp_streamOn(firmwareId);
    testSensorStart(sensorHandle);

    return 0;
}

static int online_test_viisp_streamOff(void* sensorHandle, int pipelineId, int firmwareId)
{
    viisp_vi_online_streamOff(pipelineId);
    testSensorStop(sensorHandle);
    viisp_isp_streamOff(firmwareId);
    cpp_stop(pipelineId);
    test_buffer_reset(pipelineId);

    return 0;
}

int dual_pipeline_online_test(int useAux)
{
    int ret = 0;
    TESTCONFIG testConfig = {};
    void* sensor0Handle = NULL;
    SENSOR_MODULE_INFO sensor0_info;
    int pipeline0Id = 0;
    int firmware0Id = 0;
    void* sensor1Handle = NULL;
    SENSOR_MODULE_INFO sensor1_info;
    int pipeline1Id = 1;
    int firmware1Id = 1;

    CLOG_INFO("test start");

    memset(&testConfig, 0, sizeof(TESTCONFIG));
    testConfig.pipeConfig[0].workMode = TEST_WORKMODE_ONLINE;
    testConfig.pipeConfig[0].sensorId = 0;
    strcpy(testConfig.pipeConfig[0].sensorName, "imx135_asr");
    testConfig.pipeConfig[0].out_width = 1920;
    testConfig.pipeConfig[0].out_height = 1080;
    testConfig.pipeConfig[1].workMode = TEST_WORKMODE_ONLINE;
    if (useAux == 1) {
        testConfig.pipeConfig[1].sensorId = 1;
        strcpy(testConfig.pipeConfig[1].sensorName, "gc2375h_asr");
        testConfig.pipeConfig[1].out_width = 1600;
        testConfig.pipeConfig[1].out_height = 1200;
    } else {
        testConfig.pipeConfig[1].sensorId = 2;
        strcpy(testConfig.pipeConfig[1].sensorName, "s5k5e3yx_asr");
        testConfig.pipeConfig[1].out_width = 1920;
        testConfig.pipeConfig[1].out_height = 1080;
    }
    testConfig.tuningServer = 0;

    // sensor init
    testSensorInit(&sensor0Handle, testConfig.pipeConfig[0].sensorName, testConfig.pipeConfig[0].sensorId);
    testSensorGetDevInfo(sensor0Handle, &sensor0_info);
    testSensorInit(&sensor1Handle, testConfig.pipeConfig[1].sensorName, testConfig.pipeConfig[1].sensorId);
    testSensorGetDevInfo(sensor1Handle, &sensor1_info);

    // viisp init
    viisp_vi_init();
    online_test_viisp_init(&testConfig.pipeConfig[0], pipeline0Id, firmware0Id, sensor0_info);
    online_test_viisp_init(&testConfig.pipeConfig[1], pipeline1Id, firmware1Id, sensor1_info);

    // thread init
    strcpy(pipelineProcThread[pipeline0Id].threadName, "pipeline0Func");
    pipelineProcThread[pipeline0Id].threadProcessFunc = testThreadFunc;
    pipelineProcThread[pipeline0Id].pipelineId = pipeline0Id;
    pipelineProcThread[pipeline0Id].firmwareId = firmware0Id;
    ProcThreadInit(&pipelineProcThread[pipeline0Id]);
    strcpy(pipelineProcThread[pipeline1Id].threadName, "pipeline1Func");
    pipelineProcThread[pipeline1Id].threadProcessFunc = testThreadFunc;
    pipelineProcThread[pipeline1Id].pipelineId = pipeline1Id;
    pipelineProcThread[pipeline1Id].firmwareId = firmware1Id;
    ProcThreadInit(&pipelineProcThread[pipeline1Id]);

    streamOnFlags[pipeline0Id] = 0;
    streamOnFlags[pipeline1Id] = 0;
    outputDumpFlag[pipeline0Id] = 0;
    outputDumpFlag[pipeline1Id] = 0;

    while (1) {
        char ch;
        CLOG_INFO("Input a character:");
        ch = getc(stdin);
        if (ch == 'q' || ch == 'Q') {
            CLOG_INFO("enter q exit");
            break;
        }
        if (ch == 's' || ch == 'S') {
            online_test_viisp_streamOn(sensor0Handle, pipeline0Id, firmware0Id);
            streamOnFlags[pipeline0Id] = 1;
            online_test_viisp_streamOn(sensor1Handle, pipeline1Id, firmware1Id);
            streamOnFlags[pipeline1Id] = 1;
            CLOG_INFO("sensor stream on");
            continue;
        }
        if (ch == 'c' || ch == 'C') {
            streamOnFlags[pipeline0Id] = 0;
            streamOnFlags[pipeline1Id] = 0;
            online_test_viisp_streamOff(sensor0Handle, pipeline0Id, firmware0Id);
            online_test_viisp_streamOff(sensor1Handle, pipeline1Id, firmware1Id);
            CLOG_INFO("sensor stream off");
            continue;
        }
        if (ch == 'd' || ch == 'D') {
            outputDumpFlag[pipeline0Id] = 1;
            outputDumpFlag[pipeline1Id] = 1;
            CLOG_INFO("dump one frame");
            continue;
        }
        if (ch == 'r' || ch == 'R') {
            online_test_viisp_rawDump(pipeline0Id);
            online_test_viisp_rawDump(pipeline1Id);
            CLOG_INFO("dump one raw frame");
            continue;
        }
    }

    ProcThreadDeinit(&pipelineProcThread[pipeline0Id]);
    ProcThreadDeinit(&pipelineProcThread[pipeline1Id]);

    viisp_vi_deInit();
    testSensorDeInit(sensor0Handle);
    online_test_viisp_deInit(pipeline0Id, firmware0Id, sensor0_info);
    testSensorDeInit(sensor1Handle);
    online_test_viisp_deInit(pipeline1Id, firmware1Id, sensor1_info);

    CLOG_INFO("test end");

    return ret;
}
