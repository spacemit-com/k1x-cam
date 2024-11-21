/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include "v4l2_single_online.h"
#include "v4l2_common.h"
#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define MAX_BUFFER_RAWDUMP_NUM 5
#define MAX_BUFFER_NUM   4
#define MAX_PIPELINE_NUM 2
#define MAX_FIRMWARE_NUM 2
#define RAW8_DUMP_SIZE(w, h) ((w / 16 + (w % 16 ? 1 : 0)) * 16 * h)
#define RAW10_DUMP_SIZE(w, h) ((w / 12 + (w % 12 ? 1 : 0)) * 16 * h)
#define RAW12_DUMP_SIZE(w, h) ((w / 10 + (w % 10 ? 1 : 0)) * 16 * h)
#define RAW14_DUMP_SIZE(w, h) ((w / 8 + (w % 8 ? 1 : 0)) * 16 * h)
#define VRF_INFO_LEN (128)

static LIST_HANDLE vi_out_list[MAX_PIPELINE_NUM] = {};
static LIST_HANDLE isp_out_list[MAX_FIRMWARE_NUM] = {};
static LIST_HANDLE cpp_done_list[MAX_PIPELINE_NUM] = {};
static LIST_HANDLE cpp_origin_list[MAX_PIPELINE_NUM] = {};

static THREAD_INFO pipelineProcThread[MAX_PIPELINE_NUM];
static BUFFER_POOL* vi_out_buffer_pool[MAX_PIPELINE_NUM];
static BUFFER_POOL* cpp_out_buffer_pool[MAX_PIPELINE_NUM];

static BUFFER_POOL* vi_rawdump_buffer_pool[MAX_PIPELINE_NUM];

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

#define SVIVI_NETLINK	17
#define RX_BUF_SIZE		100		//接收buf size
#define MAX_PLOAD		100		//发送message内存size
enum V4L2_PIPE_SEQ_ID {
    START_INIT = 0,
    FINISH_INIT,
    START_OPEN,
    FINISH_OPEN,
    START_QUERYCAP,
    FINISH_QUERYCAP,
    START_S_FMT,
    FINISH_S_FMT,
    START_S_PARM,
    FINISH_S_PARM,
    START_REQBUFS,
    FINISH_REQBUFS,
    START_QUERYBUF,
    FINISH_QUERYBUF,
    START_MMAP,
    FINISH_MMAP,
    START_QBUF,
    FINISH_QBUF,
    START_STREAMON,
    FINISH_STREAMON,
    START_POLL,
    FINISH_POLL,
    START_DQBUF,
    FINISH_DQBUF,
    START_STREAMOFF,
    FINISH_STREAMOFF,
    START_CLOSE,
    FINISH_CLOSE,
    MAX_SEQ_SIZE,
};
struct v4l2_vinit {
	int 			ret;
};
struct v4l2_vformat {
	int 			ret;
	int 			v4l2_buf_type;
	unsigned int	width;
	unsigned int	height;
	unsigned int	pixelformat;
};
struct v4l2_vrequestbuffers {
	int 			ret;
	unsigned int	count;
};
struct v4l2_vbuffer {
	int 			ret;
    int             index;
	unsigned int	length;
	unsigned int	m_offset;
    int 			m_fd;
};
struct v4l2_vstream {
	int 			ret;
};
struct v4l2_vpoll {
	int 			ret;
};
struct vcam_header {
	unsigned int	kpos;
	unsigned int	upos;
	unsigned int	user_pid;

	struct v4l2_vinit vinit;
	struct v4l2_vformat vfmt;
	struct v4l2_vrequestbuffers vreq_buf;
	struct v4l2_vbuffer vbuf;
	struct v4l2_vstream vstream;
	struct v4l2_vpoll vpoll;

	char pbuf[64];
};
typedef struct {
    struct nlmsghdr hdr;
    struct vcam_header rcv_header;
} recv_msg;

struct vcam_header snd_header;
static int skfd;
static struct sockaddr_nl src_addr;


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

        if ((List_IsEmpty(cpp_origin_list[pipelineId]) == false) && (List_IsEmpty(isp_out_list[firmwareId]) == false)) {
            spmVI_BUFFER_INFO_S* vi_buffer_info = List_Pop(vi_out_list[pipelineId]);
            if (vi_buffer_info) {
                spmISP_BUFFER_INFO_S* isp_buffer_info =
                    List_FindItemIf(isp_out_list[firmwareId], isp_buffer_list_find_ret, &(vi_buffer_info->frameId));
                if (!isp_buffer_info) {
                    CLOG_WARNING("frameId mismatch");
                    isp_buffer_info = List_GetBeginItem(isp_out_list[firmwareId]);
                }
                {
                    IMAGE_BUFFER_S* outputBuf = List_Pop(cpp_origin_list[pipelineId]);
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

    CLOG_DEBUG("ISP pipelineID %d out buffer frameId %d", pipelineID, frameId);
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
            //TODO: push buffer to done list
            List_Push(cpp_done_list[mppCpp.devId], (void*)&cpp_out_buffer_pool[mppCpp.devId]->buffers[i]);
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

static int test_buffer_init(int pipelineId, int firmwareId, IMAGE_INFO_S img_info, SENSOR_MODULE_INFO sensor_info)
{
    int i = 0;

    // buffer list init
    vi_out_list[pipelineId] = List_Create(0);
    isp_out_list[firmwareId] = List_Create(0);
    cpp_done_list[pipelineId] = List_Create(0);
    cpp_origin_list[pipelineId] = List_Create(0);

    // buffer init
    vi_out_buffer_pool[pipelineId] =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "vi channel0 out buffer");
    buffer_pool_alloc(vi_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        frameinfo_buffer_alloc(&frameInfoBuf[firmwareId][i]);
    }

    cpp_out_buffer_pool[pipelineId] =
        create_buffer_pool(img_info.width, img_info.height, img_info.format, "cpp channel0 out buffer");
    // buffer_pool_alloc(cpp_out_buffer_pool[pipelineId], MAX_BUFFER_NUM);

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
        List_Push(cpp_origin_list[pipelineId], (void*)buffer);
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

    List_Destroy(cpp_done_list[pipelineId]);
    cpp_done_list[pipelineId] = NULL;

    List_Destroy(cpp_origin_list[pipelineId]);
    cpp_origin_list[pipelineId] = NULL;

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

static int netlink_init(void)
{
    /* 创建NETLINK socket */
    skfd = socket(PF_NETLINK, SOCK_RAW, SVIVI_NETLINK);
    if (skfd < 0) {
        CLOG_ERROR("can not create a netlink socket\n");
        return -1;
    }

    //初始化 netlink 源地址
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = 0;
    if (bind(skfd, (struct sockaddr *)&src_addr, sizeof(src_addr)) != 0){
        CLOG_ERROR("bind() error\n");
        return -1;
    }

    return 0;
}

static int netlink_recv(void *data, unsigned int len, unsigned int need_seq)
{
    struct sockaddr_nl dest_addr;
    int ret = 0;
    int cur_pid, i;
    socklen_t rxlen = sizeof(struct sockaddr_nl);

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;            //0表示发送给kernel
    dest_addr.nl_groups = 0;

    memset(data, 0, len);
    ret = recvfrom(skfd, data, len, 0, (struct sockaddr*)&dest_addr, &rxlen);
    if (ret < 0) {
        CLOG_ERROR("recv form kerner error!");
        return ret;
    }

    struct nlmsghdr *hdr = (struct nlmsghdr *)data;
    //verify kernel pos
    if (hdr->nlmsg_seq != need_seq) {
        CLOG_WARNING("recv message dismatch seq %d != %d", hdr->nlmsg_seq, need_seq);
        return hdr->nlmsg_seq;
    }

    CLOG_INFO("recv msg seq%d, hdr len:%d, ret:%d", hdr->nlmsg_seq, hdr->nlmsg_len, ret);

    return ret;
}

static int netlink_send(void *data, unsigned int len, unsigned int seq)
{
    int ret = 0;
    struct nlmsghdr *message;
    // char *data = "This message is from user's space";
    char *retval;
    struct sockaddr_nl dest_addr;    //sockaddr_nl 是 netlink 使用的地址数据结构

    //初始化 netlink 目标地址
    // memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;            //0表示发送给kernel
    dest_addr.nl_groups = 0;         //多播组

    /* 2.设置消息 */
    message = (struct nlmsghdr *)malloc(NLMSG_SPACE(len));
    message->nlmsg_len = NLMSG_SPACE(len); //包括netlink消息头在内，整个消息长度 
    message->nlmsg_flags = 0;            //消息标志
    message->nlmsg_type = 0;             //消息类型
    message->nlmsg_seq = seq;              //消息报文的序列号
    message->nlmsg_pid = getpid(); //消息中加入本应用端口pid，内核应答时往此pid写入
    retval = memcpy(NLMSG_DATA(message), data, len);    //数据

    CLOG_INFO("User Send len:%d, cur pos:%d", message->nlmsg_len, seq);

    /* 3.发送消息 */
    ret = sendto(skfd, message, message->nlmsg_len, 0,(struct sockaddr *)&dest_addr, sizeof(dest_addr));	//将message消息发送给内核
    if(!ret){
        CLOG_ERROR("send error,  pid: %d", message->nlmsg_pid);
        return -1;
    }

    free(message);

    return ret;
}

/************************************************************************************************/
int v4l2_single_online_test(struct testConfig *config)
{
    int fd, i, ret = 0;
    void* sensorHandle = NULL;
    SENSOR_MODULE_INFO sensor_info;
    int pipelineId = 0;
    int firmwareId = 0;
    int viChnId = 0;
    int rawdumpChnId = 0;
    IMAGE_INFO_S img_info = {};
    struct tuning_objs_config tuning_cfg = {0};
    struct sockaddr_nl src_addr, dest_addr;
    int skfd, rxlen = sizeof(struct sockaddr_nl);
    recv_msg *info;

    CLOG_INFO("[netlink] init");
    netlink_init();

    snd_header.vinit.ret = 0;
    netlink_send(&snd_header.vinit, sizeof(struct v4l2_vinit), FINISH_INIT);

    info = (recv_msg *) malloc (sizeof(recv_msg));
    if (info == NULL) {
        CLOG_ERROR("malloc failed, please check!\n");
        return -1;
    }
    CLOG_INFO("[netlink] wait kernel set fmt");

    struct {
        struct nlmsghdr hdr;
        struct v4l2_vformat data;
    } recv_vfmt;
    ret = netlink_recv(&recv_vfmt, sizeof(recv_vfmt), START_S_FMT);

    //TODO: set fmt by kernel vformat, now default 480p@nv12
    CLOG_INFO("recv vformat: (%d, %d)", recv_vfmt.data.width, recv_vfmt.data.height);
    ret = testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                         config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode,
                         config);
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

    img_info.width = recv_vfmt.data.width;
    img_info.height = recv_vfmt.data.height;
    img_info.format = PIXEL_FORMAT_NV12_DWT;

    // viisp init
    viisp_vi_init();
    viisp_vi_online_config(pipelineId, img_info, &sensor_info);
    viChnId = pipelineId;
    viisp_set_vi_callback(viChnId, vi_buffer_callback);
    viisp_isp_init(firmwareId, img_info, &sensor_info, isp_buffer_callback, false);
    VIU_GET_RAW_CHN(pipelineId, rawdumpChnId);
    viisp_set_vi_callback(rawdumpChnId, vi_rawdump_buffer_callback);

    cpp_init(pipelineId, img_info, cpp_buffer_callback);

    snd_header.vfmt.ret = 0;
    netlink_send(&snd_header.vfmt, sizeof(struct v4l2_vformat), FINISH_S_FMT);

    CLOG_INFO("[netlink] wait kernel request buffer");
    struct {
        struct nlmsghdr hdr;
        struct v4l2_vrequestbuffers data;
    } recv_vreq_buf;
    ret = netlink_recv(&recv_vreq_buf, sizeof(recv_vreq_buf), START_REQBUFS);

    //TODO: req cpp buffer nums by struct v4l2_vrequestbuffers. now default 4
    test_buffer_init(pipelineId, firmwareId, img_info, sensor_info);
    CLOG_INFO("[netlink] request buffer count:%d", recv_vreq_buf.data.count);

    snd_header.vreq_buf.ret = 0;
    netlink_send(&snd_header.vreq_buf, sizeof(struct v4l2_vrequestbuffers), FINISH_REQBUFS);

    struct {
        struct nlmsghdr hdr;
        struct v4l2_vbuffer data;
    } recv_vquery_buf;
    for (i = 0; i < recv_vreq_buf.data.count; i++) {
        CLOG_INFO("[netlink] wait kernel query buffer");

        ret = netlink_recv(&recv_vquery_buf, sizeof(recv_vquery_buf), START_QUERYBUF);

        //TODO: fetch buffer info to kernel query
        buffer_pool_alloc_one(cpp_out_buffer_pool[pipelineId], &fd);

        snd_header.vbuf.ret = 0;
        snd_header.vbuf.m_fd = fd;
        netlink_send(&snd_header.vbuf, sizeof(struct v4l2_vbuffer), FINISH_QUERYBUF);
    }

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

    struct {
        struct nlmsghdr hdr;
        struct v4l2_vbuffer data;
    } recv_vqbuf;
    for (i = 0; i < recv_vreq_buf.data.count; i++) {
        CLOG_INFO("[netlink] wait kernel queue buffer");
        ret = netlink_recv(&recv_vqbuf, sizeof(recv_vqbuf), START_QBUF);

        snd_header.vbuf.ret = 0;
        netlink_send(&snd_header.vbuf, sizeof(struct v4l2_vbuffer), FINISH_QBUF);
    }

    test_buffer_prepare(pipelineId, firmwareId);

    CLOG_INFO("[netlink] wait kernel stream on, req buf %d", recv_vreq_buf.data.count);
    struct {
        struct nlmsghdr hdr;
        struct v4l2_vstream data;
    } recv_vstreamon;
    ret = netlink_recv(&recv_vstreamon, sizeof(recv_vstreamon), START_STREAMON);

    cpp_start(pipelineId);
    viisp_vi_online_streamOn(pipelineId);
    viisp_isp_streamOn(firmwareId);
    testSensorStart(sensorHandle);
    streamOnFlags[pipelineId] = 1;
    CLOG_INFO("sensor stream on");

    snd_header.vstream.ret = 0;
    netlink_send(&snd_header.vstream, sizeof(struct v4l2_vstream), FINISH_STREAMON);

    IMAGE_BUFFER_S* doneBuf = NULL;
    struct {
        struct nlmsghdr hdr;
        struct v4l2_vpoll data;
    } recv_vpoll;
    struct {
        struct nlmsghdr hdr;
        struct v4l2_vbuffer data;
    } recv_vdqbuf;
    struct {
        struct nlmsghdr hdr;
        struct v4l2_vbuffer data;
    } recv_vqbuf2;
    while (1) {
        CLOG_INFO("[netlink] wait kernel poll");

        ret = netlink_recv(&recv_vpoll, sizeof(recv_vpoll), START_POLL);
        if (ret < 0) {
            CLOG_WARNING("recv vdqbuf failed, ret=%d", ret);
            break;
        } else if (ret == START_STREAMOFF)
            break;
try_again:
        if (List_IsEmpty(cpp_done_list[pipelineId]) == false) {
            snd_header.vpoll.ret = 0;
            netlink_send(&snd_header.vpoll, sizeof(struct v4l2_vpoll), FINISH_POLL);
        } else {
            // CLOG_WARNING("no buffer in cpp_done_list");
            usleep(100);
            goto try_again;
        }

        CLOG_INFO("[netlink] wait kernel dequeue buffer");

        ret = netlink_recv(&recv_vdqbuf, sizeof(recv_vdqbuf), START_DQBUF);
        if (ret < 0) {
            CLOG_WARNING("recv vdqbuf failed, ret=%d", ret);
            break;
        } else if  (ret == START_STREAMOFF)
            break;
        //TODO: get buffer from cpp_done_list
        doneBuf = List_Pop(cpp_done_list[pipelineId]);  //must success
        if (doneBuf == NULL) {
            CLOG_ERROR("error! no buffer");
            break;
        }
        snd_header.vbuf.index = doneBuf->index;
        snd_header.vbuf.m_fd = doneBuf->m.fd;
        snd_header.vbuf.ret = 0;
        netlink_send(&snd_header.vbuf, sizeof(struct v4l2_vbuffer), FINISH_DQBUF);

        CLOG_INFO("[netlink] wait kernel queue buffer");

        ret = netlink_recv(&recv_vqbuf2, sizeof(recv_vqbuf2), START_QBUF);
        if (ret < 0) {
            CLOG_WARNING("recv vqbuf failed, ret=%d", ret);
            break;
        } else if (ret == START_STREAMOFF)
            break;
        //TODO: List_FindItemIf by v4l2_vbuffer m_fd
        //TODO: push buffer to cpp_origin_list
        List_Push(cpp_origin_list[pipelineId], (void *)doneBuf);
        snd_header.vbuf.ret = 0;
        netlink_send(&snd_header.vbuf, sizeof(struct v4l2_vbuffer), FINISH_QBUF);
    }

    // CLOG_INFO("[netlink] wait kernel stream off, ret:%d", ret);
    // struct {
    //     struct nlmsghdr hdr;
    //     struct v4l2_vstream data;
    // } recv_vstreamoff;
    // ret = netlink_recv(&recv_vstreamoff, sizeof(recv_vstreamoff), START_STREAMOFF);

    streamOnFlags[pipelineId] = 0;
    viisp_vi_online_streamOff(pipelineId);
    testSensorStop(sensorHandle);
    viisp_isp_streamOff(firmwareId);
    cpp_stop(pipelineId);
    test_buffer_reset(pipelineId);
    CLOG_INFO("sensor stream off");

    ProcThreadDeinit(&pipelineProcThread[pipelineId]);

    viisp_isp_deinit(firmwareId, sensor_info.sensorId);
    viisp_vi_deInit();

    test_buffer_deInit(pipelineId, firmwareId);

    cpp_deInit(pipelineId);

    testSensorDeInit(sensorHandle);

    snd_header.vstream.ret = 0;
    netlink_send(&snd_header.vstream, sizeof(struct v4l2_vstream), FINISH_STREAMOFF);

    fflush(stdout);
    fflush(stderr);
    return ret;
}

