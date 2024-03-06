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
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>

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

#define MAX_BUFFER_RAWDUMP_NUM 5
#define MAX_BUFFER_NUM   4
#define MAX_PIPELINE_NUM 2
#define MAX_FIRMWARE_NUM 2
#define RAW8_DUMP_SIZE(w, h) ((w / 16 + (w % 16 ? 1 : 0)) * 16 * h)
#define RAW10_DUMP_SIZE(w, h) ((w / 12 + (w % 12 ? 1 : 0)) * 16 * h)
#define RAW12_DUMP_SIZE(w, h) ((w / 10 + (w % 10 ? 1 : 0)) * 16 * h)
#define RAW14_DUMP_SIZE(w, h) ((w / 8 + (w % 8 ? 1 : 0)) * 16 * h)
#define VRF_INFO_LEN (128)

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

typedef struct spmTuning_BUFFER_INFO {
    TUNING_BUFFER_S tuningInfo;
    char hasVrf;
} spmTuning_BUFFER_INFO_S;

struct rawdump_info {
    int width;
    int height;
    int format;
    int start;
    int count;
    char addVrf;
    int bitDepth;
    uint32_t ispInfoListLen;
    CAM_VI_WORK_MODE_E viWorkMode;
    LIST_HANDLE rawdumpList;
    LIST_HANDLE ispInfoList;
    LIST_HANDLE isp_done_list;
    pthread_mutex_t rawdumpListLock;
    pthread_mutex_t ispInfoListLock;
    pthread_cond_t rawdumpListCond;
    BUFFER_POOL *rawdumpPool;
};

typedef struct VRF_INFO {
    uint16_t imageWidth;   /* [0  ] Raw image width in pixel number */
    uint16_t imageHeight;  /* [2  ] Raw image height in pixel number */
    uint16_t totalGain;    /* [4  ] Total gain for the raw file. Q4 format. 0x0010 means 1x, 0x0028 means 2.5x */
    uint16_t exposureLine; /* [6  ] Exposure line number for the raw file. Q0 format. 0x0001 means 1 line, 0x0010 means
                              16 line */
    uint16_t sensorVts;    /* [8  ] Sensor VTS line number. Q0 format */
    uint16_t awbBGain1;    /* [10 ] AWB gain set 1(before AWB shift), Q7 format, 0x80 means 1x */
    uint16_t awbGGain1;    /* [12 ] */
    uint16_t awbRGain1;    /* [14 ] */
    uint16_t awbBGain2;    /* [16 ] */
    uint16_t awbGbGain2;   /* [18 ] */
    uint16_t awbGrGain2;   /* [20 ] */
    uint16_t awbRGain2;    /* [22 ] */
    uint8_t blackLevel;    /* [24 ] In 10 bit */
    uint8_t bayerOrder;    /* [25 ] 0 BGGR, 1 GBRG, 2 GRBG, 3 RGGB */
    uint16_t blcApply;     /* [26 ] */
    uint8_t reserved[78];  /* [28 ~ 105] */
    uint16_t wbGoldenSignatureRG; /* [106] golden WB signature R*512/G (under version 2.8 and above)*/
    uint16_t wbGoldenSignatureBG; /* [108] golden WB signature B*512/G (under version 2.8 and above)*/
    uint16_t wbModuleSignatureRG; /* [110] module WB signature R*512/G (under version 2.8 and above)*/
    uint16_t wbModuleSignatureBG; /* [112] module WB signature B*512/G (under version 2.8 and above)*/
    uint8_t drcGainDark;          /* [114] Q4 format, range in [16, 255], (under version 2.9)*/
    uint8_t drcGain;              /* [115] Q4 format, range in [16, 255], (under version 2.9)*/
    uint32_t exposureTime;        /* [116] us */
    uint8_t packRawFlag;          /* [120] 0 - no pack, 1 - pack */
    uint8_t rawBitDepth;          /* [121] can be one of 8,10,12,14,16 */
    uint16_t analogGain; /* [122] Analog gain for the raw file. Q4 format. 0x0010 means 1x, 0x0028 means 2.5x */
    uint8_t version;     /* [124] 0x20 means v2.0, 0x21means v2.1 ... */
    uint8_t v;           /* [125] 'V'. In ASCII code, can be used for file format identification */
    uint8_t r;           /* [126] 'R'. In ASCII code, can be used for file format identification */
    uint8_t f;           /* [127] 'F'. In ASCII code, can be used for file format identification */
} VRF_INFO_S;            // V2.6

static LIST_HANDLE vi_out_list[MAX_PIPELINE_NUM] = {};
static LIST_HANDLE isp_out_list[MAX_FIRMWARE_NUM] = {};
static LIST_HANDLE cpp_out_list[MAX_PIPELINE_NUM] = {};
static LIST_HANDLE cpp_in_list[MAX_PIPELINE_NUM] = {};
static THREAD_INFO pipelineProcThread[MAX_PIPELINE_NUM];
static BUFFER_POOL* vi_out_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* cpp_out_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* cpp_in_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* vi_rawdump_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* vi_rawread_buffer_pool[MAX_PIPELINE_NUM];
static int streamOnFlags[MAX_PIPELINE_NUM] = {};
static int outputDumpFlag[MAX_PIPELINE_NUM] = {};
static IMAGE_BUFFER_S frameInfoBuf[MAX_FIRMWARE_NUM][MAX_BUFFER_NUM];
static struct rawdump_info g_rawdump_info[MAX_PIPELINE_NUM] = {0};

static char path[32] = "/tmp/";
static char *LayersName[5] = {"L0.nv12", "L1.raw", "L2.raw", "L3.raw", "L4.raw"};

static int testFrame = AUTO_FRAME_NUM;
static int testAutoRunFlag[MAX_PIPELINE_NUM] = {0};
static struct condition testAutoRunCond[MAX_PIPELINE_NUM];
static int showFps = 0;
/****************************************************************/
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

static PIXEL_FORMAT_E get_viisp_output_format(char *name)
{
    if (!name)
        return PIXEL_FORMAT_NV12_DWT;
    if (!strcmp(name, "NV12") || !strcmp(name, "nv12"))
        return PIXEL_FORMAT_NV12_DWT;
    else if (!strcmp(name, "RGB888") || !strcmp(name, "rgb888") ||
             !strcmp(name, "RGB24") || !strcmp(name, "rgb24") )
        return PIXEL_FORMAT_RGB888;
    else if (!strcmp(name, "RGB565") || !strcmp(name, "rgb565"))
        return PIXEL_FORMAT_RGB565;
    else if (!strcmp(name, "Y210") || !strcmp(name, "y210"))
        return PIXEL_FORMAT_Y210;
    else if (!strcmp(name, "P210") || !strcmp(name, "p210"))
        return PIXEL_FORMAT_P210;
    else if (!strcmp(name, "P010") || !strcmp(name, "p010"))
        return PIXEL_FORMAT_P010;
    else
        return PIXEL_FORMAT_NV12_DWT;
}

static int image_buffer_save(const IMAGE_BUFFER_S* imgBuf, char* fileName)
{
    FILE* fp;

    fp = fopen(fileName, "w+");
    if (!fp) {
        printf("%s: %s open failed\n", __func__, fileName);
        return -1;
    }
    for (uint32_t i = 0; i < imgBuf->numPlanes; i++)
        fwrite(imgBuf->planes[i].virAddr, imgBuf->planes[i].length, 1, fp);
    fclose(fp);
    CLOG_INFO("save img fileName %s", fileName);

    return 0;
}

static int dwt_buffer_save(int i, IMAGE_BUFFER_S* imgBuf)
{
    FILE* fp = NULL;
    IMAGE_BUFFER_PLANE_S *plane = NULL;
    char fileName[128] = {0};

    switch (i) {
    case 1:
        plane = imgBuf->dwt1;
        snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[1]);
        break;
    case 2:
        plane = imgBuf->dwt2;
        snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[2]);
        break;
    case 3:
        plane = imgBuf->dwt3;
        snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[3]);
        break;
    case 4:
        plane = imgBuf->dwt4;
        snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[4]);
        break;
    default:
        return -1;
    }
    fp = fopen(fileName, "w+");
    if (!fp) {
        CLOG_ERROR("%s: %s open failed\n", __func__, fileName);
        return -1;
    }
    for (uint32_t i = 0; i < 2; i++) fwrite(plane[i].virAddr, plane[i].length, 1, fp);
    fclose(fp);
    CLOG_INFO("save img fileName %s", fileName);

    return 0;
}

static int raw_buffer_save(const IMAGE_BUFFER_S* imgBuf, char* fileName)
{
    FILE* fp;

    fp = fopen(fileName, "w+");
    if (!fp) {
        CLOG_ERROR("%s: %s open failed\n", __func__, fileName);
        return -1;
    }
    fwrite(imgBuf->planes[0].virAddr, imgBuf->planes[0].length, 1, fp);
    fclose(fp);
    CLOG_INFO("save raw img fileName %s", fileName);

    return 0;
}

static bool isp_buffer_list_find_ret(const void* item, const void* condition)
{
    spmISP_BUFFER_INFO_S* isp_buffer_info = (spmISP_BUFFER_INFO_S*)item;
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
            spmVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list[pipelineId]);
            if (vi_buffer_info) {
                spmISP_BUFFER_INFO_S* isp_buffer_info =
                    List_FindItemIf(isp_out_list[firmwareId], isp_buffer_list_find_ret, &(vi_buffer_info->frameId));
                if (!isp_buffer_info) {
                    isp_buffer_info = List_GetBeginItem(isp_out_list[firmwareId]);
                    CLOG_WARNING("frameId mismatch %d", isp_buffer_info->frameId);
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

static void get_suffix(PIXEL_FORMAT_E format, char *suffix, int size)
{
    if (!suffix || size <= 0)
        return;

    switch (format) {
    case PIXEL_FORMAT_NV12:
        strncpy(suffix, ".nv12", size);
        break;
    case PIXEL_FORMAT_NV12_DWT:
        strncpy(suffix, ".nv12_dwt", size);
        break;
    case PIXEL_FORMAT_FBC:
        strncpy(suffix, ".fbc", size);
        break;
    case PIXEL_FORMAT_FBC_DWT:
        strncpy(suffix, ".fbc_dwt", size);
        break;
    case PIXEL_FORMAT_RAW_8BPP:
        strncpy(suffix, ".raw8", size);
        break;
    case PIXEL_FORMAT_RAW_10BPP:
        strncpy(suffix, ".raw10", size);
        break;
    case PIXEL_FORMAT_RAW_12BPP:
        strncpy(suffix, ".raw12", size);
        break;
    case PIXEL_FORMAT_RAW_14BPP:
        strncpy(suffix, ".raw14", size);
        break;
    case PIXEL_FORMAT_RAW:
        strncpy(suffix, ".raw", size);
        break;
    case PIXEL_FORMAT_JPEG:
        strncpy(suffix, ".jpeg", size);
        break;
    case PIXEL_FORMAT_RGB565:
        strncpy(suffix, ".rgb565", size);
        break;
    case PIXEL_FORMAT_RGB888:
        strncpy(suffix, ".rgb24", size);
        break;
    case PIXEL_FORMAT_Y210:
        strncpy(suffix, ".y210", size);
        break;
    case PIXEL_FORMAT_P210:
        strncpy(suffix, ".p210", size);
        break;
    case PIXEL_FORMAT_P010:
        strncpy(suffix, ".p010", size);
        break;
    default:
        strncpy(suffix, ".unknown", size);
        break;
    }
}

static void* testThreadFunc_viisp(void* param)
{
    THREAD_INFO* thread = (THREAD_INFO*)param;
    int pipelineId, firmwareId;
    char fileName[128] = {0};
    char suffix[16] = ".yuv";

    if (!thread) {
        CLOG_ERROR("thread handle");
    }
    thread->threadRunning = 1;
    pipelineId = thread->pipelineId;
    firmwareId = thread->firmwareId;

    do {
        condition_timedwait(&thread->cond, 1000);
        // CLOG_INFO("thread handle");
        if (List_IsEmpty(isp_out_list[firmwareId]) == false) {
            spmVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list[pipelineId]);
            if (vi_buffer_info) {
                spmISP_BUFFER_INFO_S* isp_buffer_info =
                    List_FindItemIf(isp_out_list[firmwareId], isp_buffer_list_find_ret, &(vi_buffer_info->frameId));
                if (!isp_buffer_info) {
                    CLOG_WARNING("frameId mismatch");
                    isp_buffer_info = List_GetBeginItem(isp_out_list[firmwareId]);
                }

                {
                    IMAGE_BUFFER_S* inputBuf = vi_buffer_info->buffer;
                    int32_t frameId = vi_buffer_info->frameId;
                    get_suffix(inputBuf->format, suffix, sizeof(suffix));
                    if (frameId % 100 == 0)
                        CLOG_INFO("get viisp buffer, frameId %d, format: %d", frameId, inputBuf->format);
                    if (outputDumpFlag[firmwareId]) {
                        snprintf(fileName, sizeof(fileName), "%svi%d_output_%dx%d_s%d%s", path, firmwareId,
                                inputBuf->planes[0].width, inputBuf->planes[0].height, inputBuf->planes[0].stride,
                                suffix);
                        image_buffer_save(inputBuf, fileName);
                        outputDumpFlag[firmwareId] = 0;
                }
                    viisp_vi_queueBuffer(firmwareId, inputBuf);
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

static int preview_cnt[MAX_PIPELINE_NUM] = {0};
static double viT1[MAX_PIPELINE_NUM] = {0};
static double viT2[MAX_PIPELINE_NUM] = {0};
static int tmpdump = 1;
static int32_t vi_buffer_callback(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_buffer)
{
    IMAGE_BUFFER_S* buffer = vi_buffer->buffer;
    // uint64_t timeStamp = vi_buffer->timeStamp;
    uint32_t frameId = vi_buffer->frameId;
    // char success = vi_buffer->bValid ? 1 : 0;
    // char closeDone = vi_buffer->bCloseDown ? 1 : 0;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;
    int pipelineId = nChn;
    int streamOnFlag = 0;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    CLOG_DEBUG("VI chn %d out buffer frameId %d, buffer %p", nChn, frameId, buffer);

    streamOnFlag = streamOnFlags[pipelineId];
    if (!streamOnFlag) {
        return 0;
    }
    // viisp_vi_queueBuffer(nChn, buffer);
    vi_buffer_info = malloc(sizeof(spmVI_BUFFER_INFO_S));
    if (vi_buffer_info) {
        vi_buffer_info->buffer = buffer;
        vi_buffer_info->frameId = frameId;
        List_Push(vi_out_list[pipelineId], (void*)vi_buffer_info);
    }

    if (tmpdump == 1 && outputDumpFlag[pipelineId] && (buffer->format == PIXEL_FORMAT_NV12_DWT)) {
        char filename[128] = {0};
        snprintf(filename, sizeof(filename), "%s/vi_%dx%d_l0.nv12", path, buffer->size.width, buffer->size.height);
        image_buffer_save(buffer, filename);
        for (int i = 1; i < 5; i++)
            dwt_buffer_save(i, buffer);
        tmpdump = 0;
    }
    condition_post(&pipelineProcThread[pipelineId].cond);

    if (testAutoRunFlag[pipelineId]) {
        if (frameId == testFrame)
            condition_post(&testAutoRunCond[pipelineId]);
        if (frameId == testFrame/2) {
            outputDumpFlag[pipelineId] = 1;
            buffer = buffer_pool_get_buffer(vi_rawdump_buffer_pool[pipelineId]);
            viisp_vi_queueBuffer(nChn + VIU_MAX_CHN_NUM, buffer);
            CLOG_INFO("dump one raw frame");
        }
    }
    if (showFps) {
        preview_cnt[pipelineId]++;
        if (preview_cnt[pipelineId] == 1)
            viT1[pipelineId] = (double)get_timestamp();
        if (preview_cnt[pipelineId] == 121) {
            viT2[pipelineId] = (double)get_timestamp();
            CLOG_INFO("chn%d preview fps: %f", pipelineId,
                     (float)(120 / ((viT2[pipelineId] - viT1[pipelineId]) / 1000000)));
        }
    }

    return 0;
}

static int isp_buffer_callback(uint32_t pipelineID, void* pstFrameinfoBuf)
{
    IMAGE_BUFFER_S* buffer = (IMAGE_BUFFER_S*)pstFrameinfoBuf;
    FRAME_INFO_S* data = buffer->planes[0].virAddr;
    int frameId = data->frameId;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmISP_BUFFER_INFO_S* isp_buffer_info1 = NULL;
    int streamOnFlag = 0;

    CLOG_DEBUG("ISP pipelineID %d out buffer %p frameId %d", pipelineID, buffer, frameId);
    streamOnFlag = streamOnFlags[pipelineID];
    if (!streamOnFlag) {
        return 0;
    }

    isp_buffer_info = malloc(sizeof(spmISP_BUFFER_INFO_S));
    if (isp_buffer_info) {
        memcpy(&isp_buffer_info->frameInfo, data, sizeof(FRAME_INFO_S));
        isp_buffer_info->frameId = frameId;
        List_Push(isp_out_list[pipelineID], (void*)isp_buffer_info);
        if (g_rawdump_info[pipelineID].start) {
            isp_buffer_info1 = List_Pop(g_rawdump_info[pipelineID].ispInfoList);
            if (isp_buffer_info1) {
                CLOG_INFO("to push isp info to done list\n");
                memcpy(isp_buffer_info1, isp_buffer_info, sizeof(*isp_buffer_info));
                List_Push(g_rawdump_info[pipelineID].isp_done_list, (void*)isp_buffer_info1);
            } else {
                CLOG_WARNING("no available isp info buffer\n");
            }
        }

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

static int init_rawdump_info(int pipelineID, SENSOR_CONFIG_S *sensor_cfg, CAM_VI_WORK_MODE_E workmode)
{
    g_rawdump_info[pipelineID].width = sensor_cfg->width;
    g_rawdump_info[pipelineID].height = sensor_cfg->height;
    g_rawdump_info[pipelineID].bitDepth = sensor_cfg->bitDepth;
    g_rawdump_info[pipelineID].format = toPixelFormatType(sensor_cfg->bitDepth);
    g_rawdump_info[pipelineID].viWorkMode = workmode;

    g_rawdump_info[pipelineID].rawdumpList = List_Create(0);
    g_rawdump_info[pipelineID].ispInfoList = List_Create(0);
    g_rawdump_info[pipelineID].isp_done_list = List_Create(0);
    pthread_mutex_init(&g_rawdump_info[pipelineID].rawdumpListLock, NULL);
    pthread_mutex_init(&g_rawdump_info[pipelineID].ispInfoListLock, NULL);

    g_rawdump_info[pipelineID].rawdumpPool = create_buffer_pool(g_rawdump_info[pipelineID].width,
                                                                g_rawdump_info[pipelineID].height,
                                                                g_rawdump_info[pipelineID].format,
                                                                "tuning tool rawdump buffer");
    if (!g_rawdump_info[pipelineID].rawdumpPool) {
        CLOG_ERROR("failed to create buffer pool for tuning tool rawdump\n");
        return -1;
    }
    return 0;
}

static void deinit_rawdump_info(int pipelineID)
{
    g_rawdump_info[pipelineID].width = 0;
    g_rawdump_info[pipelineID].height = 0;
    g_rawdump_info[pipelineID].format = 0;
    g_rawdump_info[pipelineID].viWorkMode = CAM_VI_WORK_MODE_INVALID;

    List_Clear(g_rawdump_info[pipelineID].rawdumpList);
    List_Clear(g_rawdump_info[pipelineID].ispInfoList);
    List_Clear(g_rawdump_info[pipelineID].isp_done_list);
    List_Destroy(g_rawdump_info[pipelineID].rawdumpList);
    List_Destroy(g_rawdump_info[pipelineID].ispInfoList);
    List_Destroy(g_rawdump_info[pipelineID].isp_done_list);
    pthread_mutex_destroy(&g_rawdump_info[pipelineID].rawdumpListLock);
    pthread_mutex_destroy(&g_rawdump_info[pipelineID].ispInfoListLock);
    pthread_cond_destroy(&g_rawdump_info[pipelineID].rawdumpListCond);

    destroy_buffer_pool(g_rawdump_info[pipelineID].rawdumpPool);
}

static int fillVrfInfo(VRF_INFO_S *mVrf, FRAME_INFO_S* frameInfo, int width, int height, int depth)
{
    if (!mVrf || !frameInfo)
        return -1;

    memset(mVrf, 0, sizeof(*mVrf));
    mVrf->imageWidth = width;
    mVrf->imageHeight = height;
    mVrf->totalGain = frameInfo->imageTGain >> 4;
    mVrf->exposureLine = (frameInfo->sensorExposureTime / 1000) >> 4;
    mVrf->sensorVts = (frameInfo->sensorExposureTime / 1000) >> 4;
    mVrf->awbBGain1 = frameInfo->awbBgain >> 5;
    mVrf->awbGGain1 = frameInfo->awbGgain >> 5;
    mVrf->awbRGain1 = frameInfo->awbRgain >> 5;
    mVrf->awbBGain2 = frameInfo->awbApplyBgain >> 5;
    mVrf->awbGbGain2 = frameInfo->awbGbgain >> 5;
    mVrf->awbGrGain2 = frameInfo->awbGrgain >> 5;
    mVrf->awbRGain2 = frameInfo->awbApplyRgain >> 5;
    mVrf->blackLevel = frameInfo->blc12b >> 2;
    mVrf->bayerOrder = (frameInfo->bayerOrder > 3) ? 4 : (3 - frameInfo->bayerOrder);
    mVrf->blcApply = frameInfo->blc12b;
    mVrf->exposureTime = frameInfo->sensorExposureTime / 1000;
    mVrf->wbGoldenSignatureRG = frameInfo->wbGoldenSignatureRG;
    mVrf->wbGoldenSignatureBG = frameInfo->wbGoldenSignatureBG;
    mVrf->wbModuleSignatureRG = frameInfo->wbModuleSignatureRG;
    mVrf->wbModuleSignatureBG = frameInfo->wbModuleSignatureBG;
    mVrf->drcGain = frameInfo->drcGain >> 4;
    mVrf->drcGainDark = frameInfo->drcGainDark >> 4;
    mVrf->packRawFlag = 1;
    mVrf->rawBitDepth = depth;
    mVrf->analogGain = (int)(((uint64_t)frameInfo->snsAGain * frameInfo->snsDGain) >> 16);
    mVrf->version = 0x29;
    mVrf->v = 'V';
    mVrf->r = 'R';
    mVrf->f = 'F';

    return 0;
}

static int32_t ispStartDumpRaw(TUNING_MODULE_TYPE_E type, uint32_t groupId, uint32_t count,
                        TUNING_BUFFER_S buffers[])
{
    uint32_t i;
    int ret = 0;
    spmTuning_BUFFER_INFO_S *tunBuffer = NULL;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmISP_BUFFER_INFO_S* isp_buffer_info1 = NULL;
    IMAGE_BUFFER_S *buffer;
    uint32_t rawChn = 0;
    int cnt = 0;
    uint32_t size = 0;
    uint32_t pipelineID = 0;

    if (groupId > 1) {
        CLOG_ERROR("unsupported isp group\n");
        return -1;
    }
    pipelineID = groupId;

    CLOG_INFO("isp group%d buffer count %d\n", groupId, count);

    ret = buffer_pool_alloc(g_rawdump_info[pipelineID].rawdumpPool, count);
    if (ret < 0) {
        CLOG_ERROR("failed to alloc buffer for tuning tool rawdump\n");
        return ret;
    }

    VIU_GET_RAW_CHN(pipelineID, rawChn);
    while (true) {
        buffer = buffer_pool_get_buffer(g_rawdump_info[pipelineID].rawdumpPool);
        if (!buffer) {
            CLOG_INFO("failed to get buffer from tuning tool pool\n");
            break;
        } else {
            CLOG_INFO("get one buffer frome tuning tool pool\n");
        }

        ret = ASR_VI_ChnQueueBuffer(rawChn, buffer);
        if (ret < 0)
            CLOG_ERROR("ASR_VI_ChnQueueBuffer failed func:%s line:%d\n", __func__, __LINE__);
    }

    g_rawdump_info[pipelineID].ispInfoListLen = MAX_BUFFER_NUM;
    for (i = 0; i < g_rawdump_info[pipelineID].ispInfoListLen; i++) {
        isp_buffer_info1 = malloc(sizeof(*isp_buffer_info1));
        if (isp_buffer_info1)
            List_Push(g_rawdump_info[pipelineID].ispInfoList, isp_buffer_info1);
    }

    pthread_mutex_lock(&g_rawdump_info[pipelineID].rawdumpListLock);
    g_rawdump_info[pipelineID].start = 1;
    g_rawdump_info[pipelineID].count = count;
    pthread_cond_wait(&g_rawdump_info[pipelineID].rawdumpListCond, &g_rawdump_info[pipelineID].rawdumpListLock);
    pthread_mutex_unlock(&g_rawdump_info[pipelineID].rawdumpListLock);

    for (tunBuffer = List_GetBeginItem(g_rawdump_info[pipelineID].rawdumpList), i = 0;
        tunBuffer && i < count;
        tunBuffer = List_GetNextItem(g_rawdump_info[pipelineID].rawdumpList, tunBuffer), i++) {
        buffers[i].frameId = tunBuffer->tuningInfo.frameId;
        buffers[i].length = tunBuffer->tuningInfo.length;
        buffers[i].virAddr = tunBuffer->tuningInfo.virAddr;
        if (tunBuffer->hasVrf) {
            CLOG_INFO("to append vrf info in raw buffer tail\n");
            isp_buffer_info = List_FindItemIf(g_rawdump_info[pipelineID].isp_done_list,
                                              isp_buffer_list_find_ret,
                                              &(tunBuffer->tuningInfo.frameId));
            if (!isp_buffer_info) {
                CLOG_WARNING("frameId mismatch, use the latest frame's info");
                isp_buffer_info = List_GetBeginItem(g_rawdump_info[pipelineID].isp_done_list);
                if (!isp_buffer_info) {
                    CLOG_WARNING("there is no isp info\n");
                    continue;
                }
            }
            fillVrfInfo((VRF_INFO_S *)((char *)(buffers[i].virAddr) + tunBuffer->tuningInfo.length),
                        &isp_buffer_info->frameInfo,
                        g_rawdump_info[pipelineID].width,
                        g_rawdump_info[pipelineID].height,
                        g_rawdump_info[pipelineID].bitDepth);
            buffers[i].length += VRF_INFO_LEN;

            if (isp_buffer_info->frameId == buffers[i].frameId) {
                List_Push(g_rawdump_info[pipelineID].ispInfoList, isp_buffer_info);
                List_EraseByItem(g_rawdump_info[pipelineID].isp_done_list, isp_buffer_info);
            }
        }
        cnt++;
    }

    g_rawdump_info[pipelineID].start = 0;
    return cnt;
}

static int32_t ispEndDumpRaw(TUNING_MODULE_TYPE_E type, uint32_t groupId)
{
    spmTuning_BUFFER_INFO_S *tunBuffer = NULL;
    spmISP_BUFFER_INFO_S *isp_buffer_info = NULL;
    uint32_t pipelineID;

    if (groupId > 1) {
        CLOG_INFO("unsupported isp groupId");
        return -1;
    }
    pipelineID = groupId;

    for (tunBuffer = List_GetBeginItem(g_rawdump_info[pipelineID].rawdumpList);
            tunBuffer != NULL;
            tunBuffer = List_GetNextItem(g_rawdump_info[pipelineID].rawdumpList, tunBuffer))
    {
        pthread_mutex_lock(&(g_rawdump_info[pipelineID].rawdumpListLock));
        List_EraseByItem(g_rawdump_info[pipelineID].rawdumpList, tunBuffer);
        pthread_mutex_unlock(&g_rawdump_info[pipelineID].rawdumpListLock);
        if (tunBuffer)
        {
            free(tunBuffer->tuningInfo.virAddr);
            tunBuffer->tuningInfo.virAddr = NULL;
            free(tunBuffer);
            tunBuffer = NULL;
        }
    }

    if (List_IsEmpty(g_rawdump_info[pipelineID].isp_done_list) == false) {
        do {
            isp_buffer_info = List_Pop(g_rawdump_info[pipelineID].isp_done_list);
            if (isp_buffer_info) {
                free(isp_buffer_info);
                isp_buffer_info = NULL;
            }
        } while (isp_buffer_info);
    }

    if (List_IsEmpty(g_rawdump_info[pipelineID].ispInfoList) == false) {
        do {
            isp_buffer_info = List_Pop(g_rawdump_info[pipelineID].ispInfoList);
            if (isp_buffer_info) {
                free(isp_buffer_info);
                isp_buffer_info = NULL;
            }
        } while (isp_buffer_info);
    }

    buffer_pool_free(g_rawdump_info[pipelineID].rawdumpPool);
    return 0;
}

static int getRawDump(int pipelineId, IMAGE_BUFFER_S *inputBuffer, int frameId)
{
    spmTuning_BUFFER_INFO_S *tunBuffer = NULL;
    int len;

    tunBuffer = malloc(sizeof(*tunBuffer));
    if (!tunBuffer) {
        CLOG_ERROR("malloc tunBuffer failed\n");
        return -1;
    }
    tunBuffer->tuningInfo.frameId = frameId;
    tunBuffer->tuningInfo.length = inputBuffer->planes[0].length;
    tunBuffer->hasVrf = 0;
    len = tunBuffer->tuningInfo.length;

    if (g_rawdump_info[pipelineId].viWorkMode == CAM_VI_WORK_MODE_ONLINE) {
        len += VRF_INFO_LEN;
        tunBuffer->hasVrf = 1;
    }
    tunBuffer->tuningInfo.virAddr = malloc(len);
    if (!tunBuffer->tuningInfo.virAddr) {
        CLOG_ERROR("malloc virAddr for tunBuffer failed\n");
        free(tunBuffer);
        tunBuffer = NULL;
    }
    memcpy(tunBuffer->tuningInfo.virAddr, inputBuffer->planes[0].virAddr, inputBuffer->planes[0].length);
    pthread_mutex_lock(&g_rawdump_info[pipelineId].rawdumpListLock);
    List_Push(g_rawdump_info[pipelineId].rawdumpList, tunBuffer);
    pthread_mutex_unlock(&g_rawdump_info[pipelineId].rawdumpListLock);
    if (List_GetSize(g_rawdump_info[pipelineId].rawdumpList) == g_rawdump_info[pipelineId].count) {
        pthread_cond_signal(&g_rawdump_info[pipelineId].rawdumpListCond);
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
    CLOG_INFO("VI chn %d rawdump buffer frameId %d, buffer %p, closeDown: %d",
              nChn, frameId, buffer->planes[0].virAddr, vi_rawdump_buffer->bCloseDown);

    if (buffer->planes[0].virAddr == vi_rawdump_buffer_pool[pipelineId]->buffers[0].planes[0].virAddr) {
        snprintf(fileName, sizeof(fileName), "%sraw_output%d_%dx%d.raw", path, pipelineId, buffer->size.width,
                 buffer->size.height);
        raw_buffer_save(buffer, fileName);
        buffer_pool_put_buffer(vi_rawdump_buffer_pool[pipelineId], &vi_rawdump_buffer_pool[pipelineId]->buffers[0]);
    } else {
        if (g_rawdump_info[pipelineId].start == 1) {
            getRawDump(pipelineId, buffer, frameId);
            g_rawdump_info[pipelineId].start = 0;
        }
    }

    return 0;
}

static int rawdump_cnt[MAX_PIPELINE_NUM] = {0};
static double rawdump_t0[MAX_PIPELINE_NUM] = {0};
static double rawdump_t1[MAX_PIPELINE_NUM] = {0};
static int32_t vi_rawdump_onlyrawdump_buffer_callback(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_rawdump_buffer)
{
    IMAGE_BUFFER_S* buffer = vi_rawdump_buffer->buffer;
    uint32_t frameId = vi_rawdump_buffer->frameId;
    char fileName[64];
    int pipelineId = nChn - VIU_MAX_CHN_NUM;

    if (nChn >= (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)) {
        CLOG_ERROR("invalid chnId %d", nChn);
        return -1;
    }
    //CLOG_INFO("VI chn %d rawdump buffer frameId %d, buffer %p, closeDown: %d",
    //           nChn, frameId, buffer, vi_rawdump_buffer->bCloseDown);
    //CLOG_INFO("rawdump_queue_time : %lu us", get_timestamp());

    if (outputDumpFlag[pipelineId] == 1 || (testAutoRunFlag[pipelineId] &&  frameId == DUMP_FRAME_NUM)) {
        snprintf(fileName, sizeof(fileName), "%schn%d_frame%d_%dx%d.raw", path, nChn, frameId,
                         buffer->planes[0].width, buffer->planes[0].height);
        raw_buffer_save(buffer, fileName);
        if (outputDumpFlag[pipelineId] == 1)
            outputDumpFlag[pipelineId] = 0;
    }
    if (testAutoRunFlag[pipelineId] && frameId == testFrame) {
        condition_post(&testAutoRunCond[pipelineId]);
    } else {
        viisp_vi_queueBuffer(nChn, buffer);
    }
    if (showFps) {
        rawdump_cnt[pipelineId]++;
        if (rawdump_cnt[pipelineId] == 1)
            rawdump_t0[pipelineId] = (double)get_timestamp();
        if (rawdump_cnt[pipelineId] == 121) {
            rawdump_t1[pipelineId] = (double)get_timestamp();
            CLOG_INFO("chn%d rawdump fps: %f", pipelineId,
                     (120 / (((rawdump_t1[pipelineId] - rawdump_t0[pipelineId]) / 1000000))));
        }
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
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;

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

static int test_buffer_viisp_init(int pipelineId, int firmwareId, IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info)
{
    int i = 0;

    // buffer list init
    vi_out_list[pipelineId] = List_Create(0);
    isp_out_list[firmwareId] = List_Create(0);

    // buffer init
    vi_out_buffer_pool[pipelineId] =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "vi channel0 out buffer");
    buffer_pool_alloc(vi_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_alloc(&frameInfoBuf[firmwareId][i]);
    }

    vi_rawdump_buffer_pool[pipelineId] =
        create_buffer_pool(sensor_info.sensor_cfg->width, sensor_info.sensor_cfg->height,
                           toPixelFormatType(sensor_info.sensor_cfg->bitDepth), "vi rawdump channel0 out buffer");
    buffer_pool_alloc(vi_rawdump_buffer_pool[pipelineId], 1);

    return 0;
}

static int test_buffer_viisp_prepare(int pipelineId, int firmwareId)
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

    return 0;
}

static int test_buffer_viisp_reset(int pipelineId)
{
    int i = 0;

    List_Clear(vi_out_buffer_pool[pipelineId]->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(vi_out_buffer_pool[pipelineId], &vi_out_buffer_pool[pipelineId]->buffers[i]);
    }

    return 0;
}

static int test_buffer_viisp_deInit(int pipelineId, int firmwareId)
{
    int i = 0;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;

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

    buffer_pool_free(vi_rawdump_buffer_pool[pipelineId]);
    destroy_buffer_pool(vi_rawdump_buffer_pool[pipelineId]);

    return 0;
}

static int test_buffer_viisp_offline_preview_init(int pipelineId, int firmwareId, struct testConfig *config)
{
    int i = 0;

    // buffer list init
    vi_out_list[pipelineId] = List_Create(0);
    isp_out_list[firmwareId] = List_Create(0);

    // buffer init
    vi_out_buffer_pool[pipelineId] =
        create_buffer_pool(config->ispFeConfig[pipelineId].outWidth,
                           config->ispFeConfig[pipelineId].outHeight,
                           get_viisp_output_format(config->ispFeConfig[pipelineId].format),
                           "vi channel0 out buffer");
    buffer_pool_alloc(vi_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_alloc(&frameInfoBuf[firmwareId][i]);
    }

    vi_rawread_buffer_pool[pipelineId] =
        create_buffer_pool(config->ispFeConfig[pipelineId].inWidth,
                           config->ispFeConfig[pipelineId].inHeight,
                           PIXEL_FORMAT_RAW,
                           "vi rawread channel buffer");
    buffer_pool_alloc(vi_rawread_buffer_pool[pipelineId], 1);

    return 0;
}

static int test_buffer_viisp_offline_preview_prepare(int pipelineId, int firmwareId, IMAGE_BUFFER_S* rawImage)
{
    int i = 0;
    int viChnId = pipelineId;
    int chnRead;
    IMAGE_BUFFER_S* buffer = NULL;
    int ret = 0;

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer = buffer_pool_get_buffer(vi_out_buffer_pool[pipelineId]);
        viisp_vi_queueBuffer(viChnId, buffer);
    }

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        viisp_isp_queueBuffer(firmwareId, &frameInfoBuf[firmwareId][i]);
    }

    VIU_GET_RAW_READ_CHN(pipelineId, chnRead);
    ret = ASR_VI_ChnQueueBuffer(chnRead, rawImage);
    if (ret) {
        CLOG_ERROR("ASR_VI_ChnQueueBuffer to raw read chn failed, ret = %d", ret);
    }

    return 0;
}

static int test_buffer_viisp_offline_preview_reset(int pipelineId)
{
    int i = 0;

    List_Clear(vi_out_buffer_pool[pipelineId]->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(vi_out_buffer_pool[pipelineId], &vi_out_buffer_pool[pipelineId]->buffers[i]);
    }
    buffer_pool_put_buffer(vi_rawread_buffer_pool[pipelineId], &vi_rawread_buffer_pool[pipelineId]->buffers[0]);

    return 0;
}

static int test_buffer_viisp_offline_preview_deInit(int pipelineId, int firmwareId)
{
    int i = 0;
    spmISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    spmVI_BUFFER_INFO_S* vi_buffer_info = NULL;

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

    return 0;
}

static int test_buffer_onlyrawdump_init(int pipelineId, int firmwareId, IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info)
{
    int i = 0;

    vi_rawdump_buffer_pool[pipelineId] =
        create_buffer_pool(sensor_info.sensor_cfg->width, sensor_info.sensor_cfg->height,
                           toPixelFormatType(sensor_info.sensor_cfg->bitDepth), "vi rawdump channel0 out buffer");
    buffer_pool_alloc(vi_rawdump_buffer_pool[pipelineId], MAX_BUFFER_RAWDUMP_NUM);

    return 0;
}

static int test_buffer_onlyrawdump_prepare(int pipelineId, int firmwareId)
{
    int i = 0;
    int viChnId = pipelineId;
    int rawdumpChnId;
    IMAGE_BUFFER_S* buffer = NULL;

    for (i = 0; i < MAX_BUFFER_RAWDUMP_NUM; i++) {
        buffer = buffer_pool_get_buffer(vi_rawdump_buffer_pool[pipelineId]);
        VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
        viisp_vi_queueBuffer(rawdumpChnId, buffer);
    }

    return 0;
}

static int test_buffer_onlyrawdump_reset(int pipelineId)
{
    int i = 0;

    return 0;
}

static int test_buffer_onlyrawdump_deInit(int pipelineId, int firmwareId)
{
    int i = 0;

    buffer_pool_free(vi_rawdump_buffer_pool[pipelineId]);
    destroy_buffer_pool(vi_rawdump_buffer_pool[pipelineId]);

    return 0;
}

static int test_buffer_only_cpp_init(int pipelineId, IMAGE_INFO_S inImgInfo, IMAGE_INFO_S outImgInfo)
{
    int i = 0;

    cpp_in_list[pipelineId] = List_Create(0);
    cpp_out_list[pipelineId] = List_Create(0);

    cpp_in_buffer_pool[pipelineId] =
        create_buffer_pool(inImgInfo.width, inImgInfo.height, inImgInfo.format, "cpp channel0 out buffer");
    buffer_pool_alloc(cpp_in_buffer_pool[pipelineId], 1);

    cpp_out_buffer_pool[pipelineId] =
        create_buffer_pool(outImgInfo.width, outImgInfo.height, outImgInfo.format, "cpp channel0 out buffer");
    buffer_pool_alloc(cpp_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

    return 0;
}

static int test_buffer_only_cpp_prepare(int pipelineId)
{
    int i = 0;
    IMAGE_BUFFER_S* buffer = NULL;

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer = buffer_pool_get_buffer(cpp_out_buffer_pool[pipelineId]);
        List_Push(cpp_out_list[pipelineId], (void*)buffer);
    }

    buffer = buffer_pool_get_buffer(cpp_in_buffer_pool[pipelineId]);
    List_Push(cpp_in_list[pipelineId], (void*)buffer);

    return 0;
}

static int test_buffer_only_cpp_reset(int pipelineId)
{
    int i = 0;

    List_Clear(cpp_out_buffer_pool[pipelineId]->buf_list);
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        buffer_pool_put_buffer(cpp_out_buffer_pool[pipelineId], &cpp_out_buffer_pool[pipelineId]->buffers[i]);
    }

    List_Clear(cpp_in_buffer_pool[pipelineId]->buf_list);
    buffer_pool_put_buffer(cpp_in_buffer_pool[pipelineId], &cpp_in_buffer_pool[pipelineId]->buffers[0]);

    return 0;
}

static int test_buffer_only_cpp_deInit(int pipelineId)
{
    List_Destroy(cpp_out_list[pipelineId]);
    cpp_out_list[pipelineId] = NULL;

    List_Destroy(cpp_in_list[pipelineId]);
    cpp_in_list[pipelineId] = NULL;

    buffer_pool_free(cpp_out_buffer_pool[pipelineId]);
    destroy_buffer_pool(cpp_out_buffer_pool[pipelineId]);

    buffer_pool_free(cpp_in_buffer_pool[pipelineId]);
    destroy_buffer_pool(cpp_in_buffer_pool[pipelineId]);
    return 0;
}

/************************************************************************************************/
int single_pipeline_online_test(struct testConfig *config)
{
    int ret = 0;
    void* sensorHandle = NULL;
    SENSOR_MODULE_INFO sensor_info;
    int pipelineId = 0;
    int firmwareId = 0;
    int viChnId = 0;
    int rawdumpChnId = 0;
    IMAGE_INFO_S img_info = {};
    struct tuning_objs_config tuning_cfg = {0};

    CLOG_INFO("test start");

    if (!config)
        return -1;

    // sensor init
    ret = testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                         config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode);
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

    img_info.width = config->ispFeConfig[0].outWidth;
    img_info.height = config->ispFeConfig[0].outHeight;
    // Note: with cpp, isp must output NV12 format image
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

    if (config->tuningServerEnalbe) {
        // be consistent with isp firmwareId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_ISP0] = 1;
        tuning_cfg.objs_rawdump_is_enabled[TUNING_OBJS_ISP0] = 1;
        tuning_cfg.StartDumpRaw = ispStartDumpRaw;
        tuning_cfg.EndDumpRaw = ispEndDumpRaw;
        // be consistent with cpp pipelineId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_CPP0] = 1;
        init_rawdump_info(pipelineId, sensor_info.sensor_cfg, CAM_VI_WORK_MODE_ONLINE);
        tuning_server_init(tuning_cfg);
    }

    if (config->showFps)
        showFps = 1;

    if (config->autoRun) {
        if (config->testFrame)
            testFrame = config->testFrame;
        CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

        testAutoRunFlag[pipelineId] = 1;
        condition_init(&testAutoRunCond[pipelineId]);

        test_buffer_prepare(pipelineId, firmwareId);
        cpp_start(pipelineId);
        viisp_vi_online_streamOn(pipelineId);

        viisp_isp_streamOn(firmwareId);
        testSensorStart(sensorHandle);
        streamOnFlags[pipelineId] = 1;
        CLOG_INFO("sensor stream on");
        condition_wait(&testAutoRunCond[pipelineId]);

        streamOnFlags[pipelineId] = 0;
        viisp_vi_online_streamOff(pipelineId);
        testSensorStop(sensorHandle);
        viisp_isp_streamOff(firmwareId);
        cpp_stop(pipelineId);
        test_buffer_reset(pipelineId);
        CLOG_INFO("sensor stream off");

        condition_deinit(&testAutoRunCond[pipelineId]);
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
    }

    if (config->tuningServerEnalbe) {
        tuning_server_deinit();
        deinit_rawdump_info(pipelineId);
    }

    ProcThreadDeinit(&pipelineProcThread[pipelineId]);

    viisp_isp_deinit(firmwareId, sensor_info.sensorId);
    viisp_vi_deInit();

    test_buffer_deInit(pipelineId, firmwareId);

    cpp_deInit(pipelineId);

    testSensorDeInit(sensorHandle);

    CLOG_INFO("test end");

    return ret;
}

static int online_test_viisp_init(int out_width, int out_height, int pipelineId, int firmwareId,
                                  SENSOR_MODULE_INFO sensor_info)
{
    IMAGE_INFO_S img_info = {};
    int viChnId = 0;
    int rawdumpChnId = 0;

    img_info.width = out_width;
    img_info.height = out_height;
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

int dual_pipeline_online_test(struct testConfig *config)
{
    int ret = 0;
    void* sensor0Handle = NULL;
    SENSOR_MODULE_INFO sensor0_info = {0};
    int pipeline0Id = 0;
    int firmware0Id = 0;
    void* sensor1Handle = NULL;
    SENSOR_MODULE_INFO sensor1_info = {0};
    int pipeline1Id = 1;
    int firmware1Id = 1;
    struct tuning_objs_config tuning_cfg = {0};

    if (!config)
        return -1;

    CLOG_INFO("test start");

    // sensor init
    testSensorInit(&sensor0Handle, config->ispFeConfig[0].sensorName,
                    config->ispFeConfig[0].sensorId, config->ispFeConfig[0].workMode);
    testSensorGetDevInfo(sensor0Handle, &sensor0_info);
    testSensorInit(&sensor1Handle, config->ispFeConfig[1].sensorName,
                    config->ispFeConfig[1].sensorId, config->ispFeConfig[1].workMode);
    testSensorGetDevInfo(sensor1Handle, &sensor1_info);

    // viisp init
    viisp_vi_init();
    online_test_viisp_init(config->ispFeConfig[0].outWidth, config->ispFeConfig[0].outHeight,
                           pipeline0Id, firmware0Id, sensor0_info);
    online_test_viisp_init(config->ispFeConfig[1].outWidth, config->ispFeConfig[1].outHeight,
                           pipeline1Id, firmware1Id, sensor1_info);

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

    if (config->tuningServerEnalbe) {
        // be consistent with isp firmwareId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_ISP0] = 1;
        tuning_cfg.objs_rawdump_is_enabled[TUNING_OBJS_ISP0] = 1;
        tuning_cfg.objs_is_enabled[TUNING_OBJS_ISP1] = 1;
        tuning_cfg.objs_rawdump_is_enabled[TUNING_OBJS_ISP1] = 1;
        tuning_cfg.StartDumpRaw = ispStartDumpRaw;
        tuning_cfg.EndDumpRaw = ispEndDumpRaw;
        // be consistent with cpp pipelineId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_CPP0] = 1;
        tuning_cfg.objs_is_enabled[TUNING_OBJS_CPP1] = 1;
        init_rawdump_info(pipeline0Id, sensor0_info.sensor_cfg, CAM_VI_WORK_MODE_ONLINE);
        init_rawdump_info(pipeline1Id, sensor1_info.sensor_cfg, CAM_VI_WORK_MODE_ONLINE);
        tuning_server_init(tuning_cfg);
    }

    if (config->showFps)
        showFps = 1;

    if (config->autoRun) {
        if (config->testFrame)
            testFrame = config->testFrame;
        CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

        testAutoRunFlag[pipeline0Id] = 1;
        testAutoRunFlag[pipeline1Id] = 1;
        condition_init(&testAutoRunCond[pipeline0Id]);
        condition_init(&testAutoRunCond[pipeline1Id]);

        online_test_viisp_streamOn(sensor0Handle, pipeline0Id, firmware0Id);
        streamOnFlags[pipeline0Id] = 1;
        online_test_viisp_streamOn(sensor1Handle, pipeline1Id, firmware1Id);
        streamOnFlags[pipeline1Id] = 1;
        CLOG_INFO("sensor stream on");

        condition_wait(&testAutoRunCond[pipeline0Id]);
        condition_wait(&testAutoRunCond[pipeline1Id]);

        streamOnFlags[pipeline0Id] = 0;
        streamOnFlags[pipeline1Id] = 0;
        online_test_viisp_streamOff(sensor0Handle, pipeline0Id, firmware0Id);
        online_test_viisp_streamOff(sensor1Handle, pipeline1Id, firmware1Id);
        CLOG_INFO("sensor stream off");

        condition_deinit(&testAutoRunCond[pipeline0Id]);
        condition_deinit(&testAutoRunCond[pipeline1Id]);
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
    }

    if (config->tuningServerEnalbe) {
        tuning_server_deinit();
        deinit_rawdump_info(pipeline0Id);
        deinit_rawdump_info(pipeline1Id);
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

/************************************************************************/
int only_viisp_online_test(struct testConfig *config)
{
    int ret = 0;
    void* sensorHandle = NULL;
    SENSOR_MODULE_INFO sensor_info;
    int pipelineId = 0;
    int firmwareId = 0;
    int viChnId = 0;
    int rawdumpChnId = 0;
    IMAGE_INFO_S img_info = {};
    struct tuning_objs_config tuning_cfg = {0};

    if (!config)
        return -1;

    CLOG_INFO("test start");

    // sensor init
    ret = testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                         config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode);
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

    img_info.width = config->ispFeConfig[0].outWidth;
    img_info.height = config->ispFeConfig[0].outHeight;
    // supported formats: nv12_dwt, rgb888, rgb565, y210, p210, p010
    img_info.format = get_viisp_output_format(config->ispFeConfig[0].format);

    // viisp init
    viisp_vi_init();
    viisp_vi_online_config(pipelineId, img_info, &sensor_info);
    viChnId = pipelineId;
    viisp_set_vi_callback(viChnId, vi_buffer_callback);
    viisp_isp_init(firmwareId, img_info, &sensor_info, isp_buffer_callback, false);
    VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
    viisp_set_vi_callback(rawdumpChnId, vi_rawdump_buffer_callback);

    // buffer init
    test_buffer_viisp_init(pipelineId, firmwareId, img_info, sensor_info);

    // thread init
    strcpy(pipelineProcThread[pipelineId].threadName, "pipeline0Func");
    pipelineProcThread[pipelineId].threadProcessFunc = testThreadFunc_viisp;
    pipelineProcThread[pipelineId].pipelineId = pipelineId;
    pipelineProcThread[pipelineId].firmwareId = firmwareId;
    ProcThreadInit(&pipelineProcThread[pipelineId]);

    streamOnFlags[pipelineId] = 0;
    outputDumpFlag[pipelineId] = 0;

    if (config->tuningServerEnalbe) {
        // be consistent with isp firmwareId
        tuning_cfg.objs_is_enabled[TUNING_OBJS_ISP0] = 1;
        tuning_cfg.objs_rawdump_is_enabled[TUNING_OBJS_ISP0] = 1;
        tuning_cfg.StartDumpRaw = ispStartDumpRaw;
        tuning_cfg.EndDumpRaw = ispEndDumpRaw;
        init_rawdump_info(pipelineId, sensor_info.sensor_cfg, CAM_VI_WORK_MODE_ONLINE);
        tuning_server_init(tuning_cfg);
    }

    if (config->showFps)
        showFps = 1;

    if (config->autoRun) {
        if (config->testFrame)
            testFrame = config->testFrame;
        CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

        testAutoRunFlag[pipelineId] = 1;
        condition_init(&testAutoRunCond[pipelineId]);

        test_buffer_viisp_prepare(pipelineId, firmwareId);
        viisp_vi_online_streamOn(pipelineId);
        viisp_isp_streamOn(firmwareId);
        testSensorStart(sensorHandle);
        streamOnFlags[pipelineId] = 1;
        CLOG_INFO("sensor stream on");

        condition_wait(&testAutoRunCond[pipelineId]);

        streamOnFlags[pipelineId] = 0;
        viisp_vi_online_streamOff(pipelineId);
        testSensorStop(sensorHandle);
        viisp_isp_streamOff(firmwareId);
        test_buffer_viisp_reset(pipelineId);
        CLOG_INFO("sensor stream off");

        condition_deinit(&testAutoRunCond[pipelineId]);
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
                test_buffer_viisp_prepare(pipelineId, firmwareId);
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
                test_buffer_viisp_reset(pipelineId);
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
    }

    if (config->tuningServerEnalbe) {
        tuning_server_deinit();
        deinit_rawdump_info(pipelineId);
    }

    ProcThreadDeinit(&pipelineProcThread[pipelineId]);

    viisp_isp_deinit(firmwareId, sensor_info.sensorId);
    viisp_vi_deInit();

    test_buffer_viisp_deInit(pipelineId, firmwareId);

    testSensorDeInit(sensorHandle);

    CLOG_INFO("test end");

    return ret;
}

/************************************************************************/
static int get_offline_attr_from_vrf_file(char *pFileName, void *bufVirAddr, uint32_t rawImageSize,
                                   ISP_OFFLINE_ATTR_S *pstOfflineAttr, ISP_PUB_ATTR_S *pstPubAttr)
{
    FILE *pFile = NULL;
    int ret = 0, fileSize = 0, rawType = -1, bayerOrder = -1;
    struct stat statbuff;
    uint32_t tempValue = 0, widht = 0, height = 0;
    unsigned char *pChBuf = NULL;

    if (!pFileName) {
        printf("Invalid argument!\n");
        return -1;
    }

    if (rawImageSize < 1) {
        printf("raw image size invalid:%d!\n", rawImageSize);
        return -1;
    }

    pFile = fopen(pFileName, "rb");
    if (!pFile) {
        printf("open %s failed\n!", pFileName);
        return -1;
    }

    if (stat(pFileName, &statbuff) < 0) {
        printf("stat %s failed\n!", pFileName);
        goto File_Exit;
    } else {
        fileSize = statbuff.st_size;
        if (fileSize < (int)(rawImageSize + 128)) {
            printf("this %s file doesn't have vrf info\n!", pFileName);
            goto File_Exit;
        }
    }

    ret = fread(bufVirAddr, sizeof(char), fileSize, pFile);
    if (ret != fileSize) {
        printf("read vrf failed to addr 0x%p,ret=%d, %s!\n", bufVirAddr, ret, strerror(errno));
        ret = -1;
        goto File_Exit;
    }

    printf("rawImageSize=%d!\n", rawImageSize);
    pChBuf = (unsigned char *)(bufVirAddr);
    pChBuf = pChBuf + rawImageSize;  // vrf info
    // 1. witdh and height
    widht = (pChBuf[1] << 8) | pChBuf[0];
    height = (pChBuf[3] << 8) | pChBuf[2];
    pstPubAttr->stInputSize.width = widht;
    pstPubAttr->stInputSize.height = height;

    // 2. expTime ns
    tempValue = (pChBuf[119] << 24) | (pChBuf[118] << 16) | (pChBuf[117] << 8) | pChBuf[116];
    pstOfflineAttr->exposureTime = tempValue;

    // 3. sensor again
    tempValue = (pChBuf[123] << 8) | pChBuf[122];
    pstOfflineAttr->AGain = (tempValue << 4);  // Q4->Q8

    // 4. imageTGain
    tempValue = (pChBuf[5] << 8) | pChBuf[4];       // Q4
    pstOfflineAttr->imageTGain = (tempValue << 4);  // Q4->Q8

    // 5. awb gain.
    tempValue = (pChBuf[23] << 8) | pChBuf[22];  // Q7
    pstOfflineAttr->wbRGain = (tempValue << 5);  // Q7->Q12
    tempValue = (pChBuf[21] << 8) | pChBuf[20];  // Q7
    pstOfflineAttr->wbGGain = (tempValue << 5);  // Q7->Q12
    tempValue = (pChBuf[17] << 8) | pChBuf[16];  // Q7
    pstOfflineAttr->wbBGain = (tempValue << 5);  // Q7->Q12

    // 6. BLC in 12bit
    tempValue = (pChBuf[27] << 8) | pChBuf[26];
    pstOfflineAttr->blackLevel[0] = tempValue;
    pstOfflineAttr->blackLevel[1] = tempValue;
    pstOfflineAttr->blackLevel[2] = tempValue;
    pstOfflineAttr->blackLevel[3] = tempValue;
    pstOfflineAttr->blcBitDepth = 12;

    // rawtype:8, 10, 12, 14...
    rawType = pChBuf[121];
    switch (rawType) {
        case 8:
            pstPubAttr->enRawType = CAM_SENSOR_RAWTYPE_RAW8;
            break;
        case 10:
            pstPubAttr->enRawType = CAM_SENSOR_RAWTYPE_RAW10;
            break;
        case 12:
            pstPubAttr->enRawType = CAM_SENSOR_RAWTYPE_RAW12;
            break;
        case 14:
            pstPubAttr->enRawType = CAM_SENSOR_RAWTYPE_RAW14;
            break;
        default:
            printf("invalid raw type:%d!", rawType);
            ret = -1;
            goto File_Exit;
    }

    // bayer pattern:0 BGGR, 1 GBRG, 2 GRBG, 3 RGGB, 4 Monochrome
    bayerOrder = pChBuf[25];
    switch (bayerOrder) {
        case 0:
            pstPubAttr->enBayerFmt = ISP_BAYER_PATTERN_BGGR;
            break;
        case 1:
            pstPubAttr->enBayerFmt = ISP_BAYER_PATTERN_GBRG;
            break;
        case 2:
            pstPubAttr->enBayerFmt = ISP_BAYER_PATTERN_GRBG;
            break;
        case 3:
            pstPubAttr->enBayerFmt = ISP_BAYER_PATTERN_RGGB;
            break;
        case 4:
            pstPubAttr->enBayerFmt = ISP_BAYER_PATTERN_MONO;
            break;
        default:
            printf("invalid bayer order:%d!", bayerOrder);
            ret = -1;
            goto File_Exit;
    }

    CLOG_INFO("parse vrf widht=%d,height=%d,expTime=%d,AGain=0x%x,imageTgain=0x%x,awb gain[%d-%d-%d],raw=%d,bayer=%d!\n",
           widht, height, pstOfflineAttr->exposureTime, pstOfflineAttr->AGain, pstOfflineAttr->imageTGain,
           pstOfflineAttr->wbRGain, pstOfflineAttr->wbGGain, pstOfflineAttr->wbBGain, rawType, bayerOrder);

    ret = 0;

File_Exit:
    if (pFile)
        fclose(pFile);

    return ret;
}

static int32_t single_preview_buffer_callback(uint32_t nChn, VI_IMAGE_BUFFER_S *vi_buffer)
{
    int32_t ret = 0, imageSize = 0;
    IMAGE_BUFFER_S *buffer = vi_buffer->buffer;
    uint32_t frameId = vi_buffer->frameId;
    uint64_t timeStamp = vi_buffer->timeStamp;
    FILE *pFile = NULL;
    char filePath[128];

    //CLOG_INFO("preview frame(%u) done timeStamp(%lu)\n", frameId, timeStamp);
    if (frameId % 100 == 0)
        CLOG_INFO("frame(%u) done timeStamp(%lu)\n", frameId, timeStamp);

    if (testAutoRunFlag[nChn] == 1) {
        if (frameId == testFrame/2)
            outputDumpFlag[nChn] = 1;
        if (frameId == testFrame)
            condition_post(&testAutoRunCond[nChn]);
    }
    if (outputDumpFlag[nChn] == 1 || frameId == 0) {
        imageSize = buffer->planes[0].length + buffer->planes[1].length;
        //snprintf(filePath, 128, "/vendor/etc/camera/%u_ch%d.yuv", frameId, nChn);
        snprintf(filePath, 128, "%sch%d_frame%u_%dx%d.yuv", path, nChn, frameId,
                buffer->size.width, buffer->size.height);
        pFile = fopen(filePath, "wb");
        ret = fwrite(buffer->planes[0].virAddr, sizeof(char), imageSize, pFile);
        fclose(pFile);
        CLOG_INFO("dump the %d frame from ch:%d, write size:%d!\n", frameId, nChn, ret);
        outputDumpFlag[nChn] = 0;
    }

    ret = ASR_VI_ChnQueueBuffer(nChn, buffer);
    if (ret < 0) {
        CLOG_ERROR("ASR_VI_ChnQueueBuffer failed file:%s line:%d\n", __FILE__, __LINE__);
    }
    return 0;
}

int only_viisp_offline_preview_test(struct testConfig *config)
{
    int ret = 0;
    int pipelineId = 0;
    int firmwareId = 0;
    int viChnId = 0;
    int rawdumpChnId = 0;
    ISP_PUB_ATTR_S stIspPubAttr = {0};
    ISP_OFFLINE_ATTR_S stOfflineAttr = {0};
    uint32_t rawImageSize = 0;
    IMAGE_BUFFER_S *buffer = NULL;

    if (!config)
        return -1;

    CLOG_INFO("test start");

    // buffer init
    test_buffer_viisp_offline_preview_init(pipelineId, firmwareId, config);
    switch (config->ispFeConfig[pipelineId].bitDepth) {
    case 12:
        rawImageSize = RAW12_DUMP_SIZE(config->ispFeConfig[pipelineId].inWidth,
                                       config->ispFeConfig[pipelineId].inHeight);
        break;
    case 10:
        rawImageSize = RAW10_DUMP_SIZE(config->ispFeConfig[pipelineId].inWidth,
                                       config->ispFeConfig[pipelineId].inHeight);
        break;
    default:
        return -1;
    }

    buffer = buffer_pool_get_buffer(vi_rawread_buffer_pool[pipelineId]);
    memset(&stIspPubAttr, 0, sizeof(ISP_PUB_ATTR_S));
    stIspPubAttr.enBayerFmt = ISP_BAYER_PATTERN_BGGR; //update later
    stIspPubAttr.enRawType = CAM_SENSOR_RAWTYPE_RAW12; //update later
    stIspPubAttr.stInputSize.width = config->ispFeConfig[pipelineId].inWidth;
    stIspPubAttr.stInputSize.height = config->ispFeConfig[pipelineId].inHeight;
    stIspPubAttr.stOutSize.width = config->ispFeConfig[pipelineId].outWidth;
    stIspPubAttr.stOutSize.height = config->ispFeConfig[pipelineId].outHeight;
    get_offline_attr_from_vrf_file(config->ispFeConfig[pipelineId].srcFile,
                                   buffer->planes[0].virAddr,
                                   rawImageSize, &stOfflineAttr, &stIspPubAttr);

    // viisp init
    viisp_vi_init();
    viisp_vi_offline_preview_config(pipelineId, &stIspPubAttr);
    viChnId = pipelineId;
    viisp_set_vi_callback(viChnId, single_preview_buffer_callback);

    viisp_isp_offline_preview_init(firmwareId, &stIspPubAttr, &stOfflineAttr, isp_buffer_callback);
    outputDumpFlag[pipelineId] = 0;

    if (config->showFps)
        showFps = 1;

    if (config->autoRun) {
        if (config->testFrame)
            testFrame = config->testFrame;
        CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

        testAutoRunFlag[pipelineId] = 1;
        condition_init(&testAutoRunCond[pipelineId]);

        test_buffer_viisp_offline_preview_prepare(pipelineId, firmwareId, buffer);
        viisp_vi_offline_preview_streamOn(pipelineId);
        viisp_isp_streamOn(firmwareId);
        streamOnFlags[pipelineId] = 1;
        CLOG_INFO("isp stream on");

        condition_wait(&testAutoRunCond[pipelineId]);

        streamOnFlags[pipelineId] = 0;
        viisp_vi_offline_preview_streamOff(pipelineId);
        viisp_isp_streamOff(firmwareId);
        test_buffer_viisp_offline_preview_reset(pipelineId);
        CLOG_INFO("isp stream off");

        condition_deinit(&testAutoRunCond[pipelineId]);
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
                test_buffer_viisp_offline_preview_prepare(pipelineId, firmwareId, buffer);
                viisp_vi_offline_preview_streamOn(pipelineId);
                viisp_isp_streamOn(firmwareId);
                streamOnFlags[pipelineId] = 1;
                CLOG_INFO("isp stream on");
                continue;
            }
            if (ch == 'c' || ch == 'C') {
                streamOnFlags[pipelineId] = 0;
                viisp_vi_offline_preview_streamOff(pipelineId);
                viisp_isp_streamOff(firmwareId);
                test_buffer_viisp_offline_preview_reset(pipelineId);
                CLOG_INFO("isp stream off");
                continue;
            }
            if (ch == 'd' || ch == 'D') {
                outputDumpFlag[pipelineId] = 1;
                CLOG_INFO("dump one frame");
                continue;
            }
        }
    }

    viisp_isp_offline_preview_deinit(firmwareId);
    viisp_vi_deInit();

    test_buffer_viisp_offline_preview_deInit(pipelineId, firmwareId);
    CLOG_INFO("test end");

    return ret;
}

/************************************************************************/
int only_rawdump_test(struct testConfig *config)
{
    int ret = 0;
    void* sensorHandle = NULL;
    SENSOR_MODULE_INFO sensor_info;
    int pipelineId = 0;
    int firmwareId = 0;
    int viChnId = 0;
    int rawdumpChnId = 0;
    IMAGE_INFO_S img_info = {};

    if (!config)
        return -1;

    CLOG_INFO("test start");

    // sensor init
    ret = testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                         config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode);
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

    img_info.width = config->ispFeConfig[0].outWidth;
    img_info.height = config->ispFeConfig[0].outHeight;
    img_info.format = PIXEL_FORMAT_NV12_DWT;

    // viisp init
    viisp_vi_init();
    viisp_vi_onlyrawdump_config(pipelineId, img_info, &sensor_info);
    VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
    viisp_set_vi_callback(rawdumpChnId, vi_rawdump_onlyrawdump_buffer_callback);

    // buffer init
    test_buffer_onlyrawdump_init(pipelineId, firmwareId, img_info, sensor_info);

    streamOnFlags[pipelineId] = 0;
    outputDumpFlag[pipelineId] = 0;

    if (config->showFps)
        showFps = 1;

    if (config->autoRun) {
        if (config->testFrame)
            testFrame = config->testFrame;
        CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

        testAutoRunFlag[pipelineId] = 1;
        condition_init(&testAutoRunCond[pipelineId]);

        test_buffer_onlyrawdump_prepare(pipelineId, firmwareId);
        viisp_vi_onlyrawdump_streamOn(pipelineId);
        testSensorStart(sensorHandle);
        streamOnFlags[pipelineId] = 1;
        CLOG_INFO("sensor stream on");

        condition_wait(&testAutoRunCond[pipelineId]);

        streamOnFlags[pipelineId] = 0;
        viisp_vi_onlyrawdump_streamOff(pipelineId);
        testSensorStop(sensorHandle);
        test_buffer_onlyrawdump_reset(pipelineId);
        CLOG_INFO("sensor stream off");

        condition_deinit(&testAutoRunCond[pipelineId]);
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
                test_buffer_onlyrawdump_prepare(pipelineId, firmwareId);
                viisp_vi_onlyrawdump_streamOn(pipelineId);
                testSensorStart(sensorHandle);
                streamOnFlags[pipelineId] = 1;
                CLOG_INFO("sensor stream on");
                continue;
            }
            if (ch == 'c' || ch == 'C') {
                streamOnFlags[pipelineId] = 0;
                viisp_vi_onlyrawdump_streamOff(pipelineId);
                testSensorStop(sensorHandle);
                test_buffer_onlyrawdump_reset(pipelineId);
                CLOG_INFO("sensor stream off");
                continue;
            }
            if (ch == 'd' || ch == 'D') {
                outputDumpFlag[pipelineId] = 1;
                CLOG_INFO("dump one frame");
                continue;
            }
        }
    }

    viisp_vi_deInit();

    test_buffer_onlyrawdump_deInit(pipelineId, firmwareId);

    testSensorDeInit(sensorHandle);

    CLOG_INFO("test end");

    return ret;
}

/************************************************************************/
static int frame_done = 0;
static pthread_cond_t frame_done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int cpp_in_cnt = 0;
static int cpp_out_cnt = 0;
static int32_t only_cpp_buffer_callback(MPP_CHN_S mppCpp, const IMAGE_BUFFER_S *callbackBuf, char success)
{
    char fileName[64], *suffix;

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
            cpp_in_cnt++;
            CLOG_INFO("cpp inputBuf %d callback\n", cpp_in_cnt);
            snprintf(fileName, sizeof(fileName), "%scpp%d_input_%dx%d_s%d_%d.%s",
                     path, mppCpp.devId,
                     callbackBuf->planes[0].width, callbackBuf->planes[0].height,
                     callbackBuf->planes[0].stride, cpp_out_cnt, suffix);
            if (outputDumpFlag[mppCpp.devId]) {
                image_buffer_save(callbackBuf, fileName);
            }
            break;
        case 0:
            cpp_out_cnt++;
            CLOG_INFO("cpp outputBuf %d callback\n", cpp_out_cnt);
            snprintf(fileName, sizeof(fileName), "%scpp%d_output_%dx%d_s%d_%d.%s",
                     path, mppCpp.devId,
                     callbackBuf->planes[0].width, callbackBuf->planes[0].height,
                     callbackBuf->planes[0].stride, cpp_out_cnt, suffix);
            if (outputDumpFlag[mppCpp.devId]) {
                image_buffer_save(callbackBuf, fileName);
                outputDumpFlag[mppCpp.devId] = 0;
            }

            pthread_mutex_lock(&mutex);
            frame_done = 1;
            pthread_cond_signal(&frame_done_cond);
            pthread_mutex_unlock(&mutex);
            break;
        default:
            CLOG_ERROR("invalid chnl %d\n", mppCpp.chnId);
            return -EINVAL;
    }

    return 0;
}

static int image_buffer_load_raw(IMAGE_BUFFER_S *imgBuf, char *path)
{
    char fileName[128];
    FILE *in_file;
    size_t readLen = 0;
    int32_t ret = 0;

    switch (imgBuf->format) {
        case PIXEL_FORMAT_NV12_DWT:
        case PIXEL_FORMAT_NV12:
            LayersName[0] = "L0.nv12";
            break;
        case PIXEL_FORMAT_FBC_DWT:
        case PIXEL_FORMAT_FBC:
            LayersName[0] = "L0.fbc";
            break;
        default:
            printf("invalid pixel format %d\n", imgBuf->format);
            return -EINVAL;
    }

    snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[0]);
    in_file = fopen(fileName, "r");
    if (in_file == NULL) {
        printf("%s %s failed, %s\n", __func__, fileName, strerror(errno));
        ret = -1;
    } else {
        for (int plane = 0; plane < 2; ++plane) {
            size_t readsize = imgBuf->planes[plane].length;
            readLen = fread(imgBuf->planes[plane].virAddr, 1, readsize, in_file);
            printf("loading %s at length 0x%lx\n", fileName, readLen);
            if (readLen == 0)
                perror("fread");
        }
        fclose(in_file);
    }

    snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[1]);
    in_file = fopen(fileName, "r");
    if (in_file == NULL) {
        printf("%s %s failed, %s\n", __func__, fileName, strerror(errno));
        ret = -1;
    } else {
        for (int plane = 0; plane < 2; ++plane) {
            size_t readsize = imgBuf->dwt1[plane].length;
            readLen = fread(imgBuf->dwt1[plane].virAddr, 1, readsize, in_file);
            printf("loading %s at length 0x%lx\n", fileName, readLen);
            if (readLen == 0)
                perror("fread");
        }
        fclose(in_file);
    }

    snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[2]);
    in_file = fopen(fileName, "r");
    if (in_file == NULL) {
        printf("%s %s failed, %s\n", __func__, fileName, strerror(errno));
        ret = -1;
    } else {
        for (int plane = 0; plane < 2; ++plane) {
            size_t readsize = imgBuf->dwt2[plane].length;
            readLen = fread(imgBuf->dwt2[plane].virAddr, 1, readsize, in_file);
            printf("loading %s at length 0x%lx\n", fileName, readLen);
        }
        fclose(in_file);
    }

    snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[3]);
    in_file = fopen(fileName, "r");
    if (in_file == NULL) {
        printf("%s %s failed, %s\n", __func__, fileName, strerror(errno));
        ret = -1;
    } else {
        for (int plane = 0; plane < 2; ++plane) {
            size_t readsize = imgBuf->dwt3[plane].length;
            readLen += fread(imgBuf->dwt3[plane].virAddr, 1, readsize, in_file);
            printf("loading %s at length 0x%lx\n", fileName, readLen);
        }
        fclose(in_file);
    }

    snprintf(fileName, sizeof(fileName), "%s/%s", path, LayersName[4]);
    in_file = fopen(fileName, "r");
    if (in_file == NULL) {
        printf("%s %s failed, %s\n", __func__, fileName, strerror(errno));
        ret = -1;
    } else {
        for (int plane = 0; plane < 2; ++plane) {
            size_t readsize = imgBuf->dwt4[plane].length;
            readLen += fread(imgBuf->dwt4[plane].virAddr, 1, readsize, in_file);
            printf("loading %s at length 0x%lx\n", fileName, readLen);
        }
        fclose(in_file);
    }

    return ret;
}

int only_cpp_test(struct testConfig *config)
{
    IMAGE_INFO_S inImgInfo = {0};
    IMAGE_INFO_S outImgInfo = {0};
    FRAME_INFO_S frameInfo = {0};
    IMAGE_BUFFER_S *inputBuf = {0};
    IMAGE_BUFFER_S *outputBuf = {0};
    int viChnId = 0;
    int rawdumpChnId = 0;
    int pipelineId = 0;
    int loopNum = 10;
    int fps = 30;
    int ret = 0;

    int i;
    char inSettingFile[64] = "/tmp/cpp_in_setting.data";
    char outSettingFile[64] = "/tmp/cpp_out_setting.data";

    if (!config)
        return -1;

    if (config->testFrame)
        testFrame = config->testFrame;
    CLOG_INFO("sensor config parse, testFrame:%d, showFps:%d", config->testFrame, showFps);

    inImgInfo.width = config->cppConfig[pipelineId].width;
    inImgInfo.height = config->cppConfig[pipelineId].height;
    inImgInfo.format = get_viisp_output_format(config->cppConfig[pipelineId].format);//PIXEL_FORMAT_NV12_DWT;

    outImgInfo.width = inImgInfo.width;
    outImgInfo.height = inImgInfo.height;
    outImgInfo.format = PIXEL_FORMAT_NV12;

    test_buffer_only_cpp_init(pipelineId, inImgInfo, outImgInfo);
    test_buffer_only_cpp_prepare(pipelineId);

    inputBuf = List_Pop(cpp_in_list[pipelineId]);
    ret = image_buffer_load_raw(inputBuf, config->cppConfig[pipelineId].srcFile);
    if (ret) {
        CLOG_ERROR("image_buffer_load_raw failed\n");
        return -1;
    }
    // cpp init
    cpp_init(pipelineId, outImgInfo, only_cpp_buffer_callback);

    cpp_load_fw_settingfile(pipelineId, inSettingFile);

    cpp_start(pipelineId);

    memset(&frameInfo, 0, sizeof(FRAME_INFO_S));
    frameInfo.imageTGain = 1 << 8;
    for (i = 0; i < loopNum; i++) {
        frame_done = 0;
        frameInfo.frameId = i;
        if (i % 5 == 0)
            outputDumpFlag[pipelineId] = 1;
        cpp_test_fw_infs(pipelineId);
        pthread_mutex_lock(&mutex);
        outputBuf = List_Pop(cpp_out_list[pipelineId]);
        cpp_post_buffer(pipelineId, inputBuf, outputBuf, i, &frameInfo);

        usleep(1000000 / fps);
        /* Wait for condition signal */
        while (frame_done == 0) {
            pthread_cond_wait(&frame_done_cond, &mutex);
        }
        List_Push(cpp_out_list[pipelineId], outputBuf);
        pthread_mutex_unlock(&mutex);
    }
    cpp_save_fw_settingfile(pipelineId, outSettingFile);
    cpp_stop(pipelineId);

    cpp_deInit(pipelineId);

    List_Push(inputBuf, cpp_in_list[pipelineId]);
    test_buffer_only_cpp_reset(pipelineId);
    test_buffer_only_cpp_deInit(pipelineId);

    return 0;
}
