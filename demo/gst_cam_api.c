/*
 * Copyright (C) 2023 ASR Micro Limited
 * All Rights Reserved.
 */

#include "gst_cam_api.h"

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

int (*gst_get_cpp_buffer)(IMAGE_BUFFER_S*);

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
            asrVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list[pipelineId]);
            if (vi_buffer_info) {
                asrISP_BUFFER_INFO_S* isp_buffer_info =
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

void gst_release_cpp_buffer(IMAGE_BUFFER_S* outputBuf)
{
    List_Push(cpp_out_list[0], (void*)outputBuf);
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
    asrVI_BUFFER_INFO_S* vi_buffer_info = NULL;
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
    vi_buffer_info = malloc(sizeof(asrVI_BUFFER_INFO_S));
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
    asrISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    asrISP_BUFFER_INFO_S* isp_buffer_info1 = NULL;
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
    int ret = 0;

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
            ret = (*gst_get_cpp_buffer)((IMAGE_BUFFER_S*) &cpp_out_buffer_pool[mppCpp.devId]->buffers[i]);
            if (ret)
                return -EINVAL;
            // List_Push(cpp_out_list[mppCpp.devId], (void*)&cpp_out_buffer_pool[mppCpp.devId]->buffers[i]);
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
    asrTuning_BUFFER_INFO_S *tunBuffer = NULL;
    asrISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    asrISP_BUFFER_INFO_S* isp_buffer_info1 = NULL;
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
    asrTuning_BUFFER_INFO_S *tunBuffer = NULL;
    asrISP_BUFFER_INFO_S *isp_buffer_info = NULL;
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
    asrTuning_BUFFER_INFO_S *tunBuffer = NULL;
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

static int test_buffer_prepare(int pipelineId, int firmwareId, int (*gst_cpp_buf_prepare)(void *, IMAGE_BUFFER_S*), void *gst_cpp_buf_prepare_data)
{
    int i = 0;
    int viChnId = pipelineId;
    int ret = 0;

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(vi_out_buffer_pool[pipelineId]);
        viisp_vi_queueBuffer(viChnId, buffer);
    }
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        viisp_isp_queueBuffer(firmwareId, &frameInfoBuf[firmwareId][i]);
    }

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        IMAGE_BUFFER_S* buffer = buffer_pool_get_buffer(cpp_out_buffer_pool[pipelineId]);
        ret = gst_cpp_buf_prepare(gst_cpp_buf_prepare_data, buffer);
        if (ret) {
            CLOG_ERROR("gst_cpp_buf_prepare return %d, error!", ret);
            return ret;
        }
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
    asrISP_BUFFER_INFO_S* isp_buffer_info = NULL;
    asrVI_BUFFER_INFO_S* vi_buffer_info = NULL;

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

/************************************************************************************************/
static int single_pipeline_online_start(struct gstParam *para)
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
    struct testConfig *config = NULL;

    config = malloc(sizeof(struct testConfig));
    if (!config)
        return -1;

    CLOG_INFO("analysis json file");
    ret = getTestConfig(config, para->jsonfile);
    if (ret)
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

    test_buffer_prepare(pipelineId, firmwareId, para->gst_cpp_buf_prepare, para->gst_cpp_buf_prepare_data);
    cpp_start(pipelineId);
    viisp_vi_online_streamOn(pipelineId);
    viisp_isp_streamOn(firmwareId);
    testSensorStart(sensorHandle);
    streamOnFlags[pipelineId] = 1;
    CLOG_INFO("sensor stream on");

    para->sensorHandle = sensorHandle;
    para->sensorInfoId = sensor_info.sensorId;
    para->firmwareId = firmwareId;
    para->pipelineId = pipelineId;
    para->out_width = img_info.width;
    para->out_height = img_info.height;

    gst_get_cpp_buffer = para->gst_get_cpp_buffer;

    return ret;
}
static void single_pipeline_online_stop(struct gstParam *para)
{
  streamOnFlags[para->pipelineId] = 0;
  viisp_vi_online_streamOff(para->pipelineId);
  testSensorStop(para->sensorHandle);
  viisp_isp_streamOff(para->firmwareId);
  cpp_stop(para->pipelineId);
  test_buffer_reset(para->pipelineId);
  CLOG_INFO("sensor stream off");

  ProcThreadDeinit(&pipelineProcThread[para->pipelineId]);

  viisp_isp_deinit(para->firmwareId, para->sensorInfoId);
  viisp_vi_deInit();

  test_buffer_deInit(para->pipelineId, para->firmwareId);

  cpp_deInit(para->pipelineId);

  testSensorDeInit(para->sensorHandle);
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

static int online_test_viisp_streamOn(void* sensorHandle, int pipelineId, int firmwareId, struct gstParam *para)
{
    test_buffer_prepare(pipelineId, firmwareId, para->gst_cpp_buf_prepare, para->gst_cpp_buf_prepare_data);
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

static int dual_pipeline_online_start(struct gstParam *para)
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
    struct testConfig *config = NULL;

    config = malloc(sizeof(struct testConfig));
    if (!config)
        return -1;

    CLOG_INFO("analysis json file");
    ret = getTestConfig(config, para->jsonfile);
    if (ret)
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

    online_test_viisp_streamOn(sensor0Handle, pipeline0Id, firmware0Id, para);
    streamOnFlags[pipeline0Id] = 1;
    online_test_viisp_streamOn(sensor1Handle, pipeline1Id, firmware1Id, para);
    streamOnFlags[pipeline1Id] = 1;
    CLOG_INFO("sensor stream on");

    // para->sensorHandle = sensorHandle;
    // para->sensorInfoId = sensor_info.sensorId;
    // para->firmwareId = firmwareId;
    // para->pipelineId = pipelineId;

    return ret;
}

/************************************************************************/
static int only_viisp_online_start(struct gstParam *para)
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
    struct testConfig *config = NULL;

    config = malloc(sizeof(struct testConfig));
    if (!config)
        return -1;

    CLOG_INFO("analysis json file");
    ret = getTestConfig(config, para->jsonfile);
    if (ret)
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

    test_buffer_viisp_prepare(pipelineId, firmwareId);
    viisp_vi_online_streamOn(pipelineId);
    viisp_isp_streamOn(firmwareId);
    testSensorStart(sensorHandle);
    streamOnFlags[pipelineId] = 1;
    CLOG_INFO("sensor stream on");

    return ret;
}

int gst_setup_camera_start (struct gstParam *para)
{
    int ret = 0;
    ret = single_pipeline_online_start(para);

    return ret;
}

int gst_setup_camera_stop (struct gstParam *para)
{

    single_pipeline_online_stop(para);

    return 0;
}
