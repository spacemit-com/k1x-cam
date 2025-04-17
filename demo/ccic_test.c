/*
 * Copyright (C) 2024 SPACEMIT Micro Limited
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

#include "sensor_common.h"
#include "viisp_common.h"
#include "spm_cam_ccic.h"
#include "tuning_server.h"

#define BUFFER_NUM (1)
#define MAX_PIPELINE_NUM 2

// BUFFER_POOL *g_raw_dump_pool = NULL;
static BUFFER_POOL *g_main_dump_pool0 = NULL;
static BUFFER_POOL *g_main_dump_pool1 = NULL;

static BUFFER_POOL *g_sub_dump_pool = NULL;

static int g_vc_mode_0 = 0;
static int g_vc_mode_1 = 0;

static int testAutoRunFlag0 = 0;
static struct condition testAutoRunCond0;
static int testAutoRunFlag1 = 0;
static struct condition testAutoRunCond1;
static int outputDumpFlag0[MAX_PIPELINE_NUM] = {};
static int outputDumpFlag1[MAX_PIPELINE_NUM] = {};
static int streamOnFlags0 = 0;
static int streamOnFlags1 = 0;

static char path[32] = "/tmp/";

int writeToFile(const char *filename, IMAGE_BUFFER_S *buffer)
{
    //    uint32_t i;
    //    int offset;

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        CLOG_INFO("%s: open \"%s\" failed", __func__, filename);
        return -1;
    }

    // size_t ret = fwrite((uint8_t *)buffer->planes[0].virAddr, buffer->planes[0].length, sizeof(char), fp);
    size_t ret = fwrite((uint8_t *)buffer->planes[0].virAddr, buffer->planes[0].length, sizeof(char), fp);
    CLOG_INFO("write %s successfully, ret:%zu, plane len:%d", filename, ret, buffer->planes[0].length);
    // offset = 0;
    // for (i = 0; i < buffer->planes[0].height; i++) {
    //     fwrite((uint8_t*) buffer->planes[0].virAddr + offset, buffer->planes[0].width, 1, fp);
    //  //memset((uint8_t*) buffer->planes[0].virAddr + offset,0x5a,buffer->planes[0].width);
    //    offset += buffer->planes[0].stride;
    //}

    // offset = 0;
    // for (i = 0; i < buffer->planes[1].height; i++) {
    //     fwrite((uint8_t*)buffer->planes[1].virAddr + offset, buffer->planes[1].width, 1, fp);
    //      memset((uint8_t*) buffer->planes[1].virAddr + offset,0x5a,buffer->planes[1].width);
    //    offset += buffer->planes[1].stride;

    //}

    //    offset = 0;
    // for (i = 0; i < buffer->planes[2].height; i++) {
    //     fwrite((uint8_t*)buffer->planes[2].virAddr + offset, buffer->planes[2].width, 1, fp);
    //      memset((uint8_t*) buffer->planes[2].virAddr + offset,0x5a,buffer->planes[2].width);
    //    offset += buffer->planes[2].stride;
    //}
    return fclose(fp);
}

static int32_t buffer_callback(uint32_t nChn, CCIC_IMAGE_BUFFER_S *ccic_buffer)
{
    int32_t ret = 0;
    IMAGE_BUFFER_S *buffer = ccic_buffer->buffer;
    uint32_t frameId = ccic_buffer->frameId;
    uint64_t timeStamp = ccic_buffer->timeStamp;
    char filename[100] = { 0 };
    static uint64_t counter_0 = 0, counter_1 = 0;
#if 0
    if (nChn > 2 && frameId >= 100 && frameId <= 300) {
        CLOG_INFO("frame(%u) done timeStamp(%lu)\n", frameId, timeStamp);
        sprintf(filename,"/vendor/etc/camera/ch%u_ahd_%d.yuv", nChn, frameId);
        writeToFile(filename,buffer);
    }
#else

    int pipelineId = nChn < CCU_MAX_MAIN_CHN_NUM ? 0 : 1;

    if (nChn <= 2) {
        counter_0++;
        if (counter_0 % 100 == 0) {
            CLOG_INFO("chn(%u) frame(%u) done timeStamp(%lu)\n", nChn, frameId, timeStamp);
        }
        if (testAutoRunFlag0 == 1) {
            if (counter_0 == DUMP_FRAME_NUM)
                outputDumpFlag0[pipelineId] = 1;
            if (counter_0 >= AUTO_FRAME_NUM)
                condition_post(&testAutoRunCond0);
        }
    } else {
        counter_1++;
        if (counter_1 % 100 == 0) {
            CLOG_INFO("chn(%u) frame(%u) done timeStamp(%lu)\n", nChn, frameId, timeStamp);
        }
        if (testAutoRunFlag0 == 1) {
            if (counter_1 == DUMP_FRAME_NUM)
                outputDumpFlag0[pipelineId] = 1;
        }
    }

    if (outputDumpFlag0[pipelineId] == 1) {
        sprintf(filename, "%sch%u_vcmode_%d_size_%dx%d_frameid_%d.raw", path, nChn, g_vc_mode_0, buffer->planes[0].width / 2,
                buffer->planes[0].height, frameId);
        writeToFile(filename, buffer);
        CLOG_INFO("dump the %d frame from ch:%d!\n", frameId, nChn);
        outputDumpFlag0[pipelineId] = 0;
    }

#endif
    // memset((uint8_t *)buffer->planes[0].virAddr, 0, buffer->planes[0].length);
    ret = ASR_CCIC_ChnQueueBuffer(nChn, buffer);
    if (ret < 0) {
        CLOG_INFO("ASR_CCIC_ChnQueueBuffer failed file:%s line:%d\n", __FILE__, __LINE__);
    }

    return 0;
}

static int32_t buffer_callback2(uint32_t nChn, CCIC_IMAGE_BUFFER_S *ccic_buffer)
{
    int32_t ret = 0;
    IMAGE_BUFFER_S *buffer = ccic_buffer->buffer;
    uint32_t frameId = ccic_buffer->frameId;
    uint64_t timeStamp = ccic_buffer->timeStamp;
    char filename[100] = { 0 };
    static uint64_t counter_0 = 0, counter_1 = 0;
#if 0
    if (nChn > 2 && frameId >= 100 && frameId <= 300) {
        CLOG_INFO("frame(%u) done timeStamp(%lu)\n", frameId, timeStamp);
        sprintf(filename,"/vendor/etc/camera/ch%u_ahd_%d.yuv", nChn, frameId);
        writeToFile(filename,buffer);
    }
#else

    int pipelineId = nChn < CCU_MAX_MAIN_CHN_NUM ? 0 : 1;

    if (nChn <= 2) {
        counter_0++;
        if (counter_0 % 100 == 0) {
            CLOG_INFO("chn(%u) frame(%u) done timeStamp(%lu)\n", nChn, frameId, timeStamp);
        }
        if (testAutoRunFlag1 == 1) {
            if (counter_0 == DUMP_FRAME_NUM)
                outputDumpFlag1[pipelineId] = 1;
            if (counter_0 >= AUTO_FRAME_NUM)
                condition_post(&testAutoRunCond1);
        }
    } else {
        counter_1++;
        if (counter_1 % 100 == 0) {
            CLOG_INFO("chn(%u) frame(%u) done timeStamp(%lu)\n", nChn, frameId, timeStamp);
        }
        if (testAutoRunFlag1 == 1) {
            if (counter_1 == DUMP_FRAME_NUM)
                outputDumpFlag1[pipelineId] = 1;
        }
    }

    if (outputDumpFlag1[pipelineId] == 1) {
        sprintf(filename, "%sch%u_vcmode_%d_size_%dx%d_frameid_%d.raw", path, nChn, g_vc_mode_1, buffer->planes[0].width / 2,
                buffer->planes[0].height, frameId);
        writeToFile(filename, buffer);
        CLOG_INFO("dump the %d frame from ch:%d!\n", frameId, nChn);
        outputDumpFlag1[pipelineId] = 0;
    }

#endif
    // memset((uint8_t *)buffer->planes[0].virAddr, 0, buffer->planes[0].length);
    ret = ASR_CCIC_ChnQueueBuffer(nChn, buffer);
    if (ret < 0) {
        CLOG_INFO("ASR_CCIC_ChnQueueBuffer failed file:%s line:%d\n", __FILE__, __LINE__);
    }

    return 0;
}

int only_ccic_test(struct testConfig *config)
{
    int ret = 0, ccic_id = config->ispFeConfig[0].sensorId, i = 0;
    int work_mode_id = config->ispFeConfig[0].sensorWorkMode;
    int mainChn0 = 0, subChn = 0;
    g_vc_mode_0 = config->ispFeConfig[0].vcMode;
    CCIC_DEV_ATTR_S ccDevAttr0;
    CCIC_CHN_ATTR_S ccChnAttr0;
    IMAGE_BUFFER_S rawDumpBufInfo;
    void *sensor_handle = NULL;
    SENSORS_MODULE_INFO_S sensor_module_info;
    SENSORS_MODULE_CAPABILITY_S sensor_cap;
    SENSOR_CUSTOM_S snr_custom;
    void* sensorHandle = NULL;
    SENSOR_MODULE_INFO sensor_info0;

    IMAGE_BUFFER_S *buffer = NULL;
    bool stream_on0 = false;
    char sensors_name[50];
    strcpy(sensors_name, config->ispFeConfig[0].sensorName);

    CLOG_INFO("ccic_test enter, sensors_name %s, ccic_id %d work_mode_id %d vc_mode %d\n", sensors_name, ccic_id, work_mode_id,
              g_vc_mode_0);

    snr_custom.dev_id = ccic_id;
    snr_custom.i2c_addr = -1;
    snr_custom.board_id = config->boardId;

    ret = testSensorInit(&sensorHandle, config->ispFeConfig[0].sensorName,
                         config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode,
                         config);
    if (ret) {
        CLOG_ERROR("testSensorInit failed\n");
        return ret;
    }

    ret = testSensorGetDevInfo(sensorHandle, &sensor_info0);
    if (ret) {
        CLOG_ERROR("testSensorGetDevInfo failed\n");
        testSensorDeInit(sensorHandle);
        return ret;
    }

    // config ccic
    ASR_CCIC_Init();

    int raw_width = sensor_info0.sensor_cfg[work_mode_id].width;
    int raw_height = sensor_info0.sensor_cfg[work_mode_id].height;
    int raw_format = PIXEL_FORMAT_RAW;

    g_main_dump_pool0 = create_buffer_pool(raw_width, raw_height, raw_format, "main dump buffer pool");
    if (!g_main_dump_pool0) {
        CLOG_INFO("failed to create main buffer pool\n");
        goto disable_devs;
    }
    ret = buffer_pool_continous_alloc(g_main_dump_pool0, BUFFER_NUM, 1);
    if (ret) {
        CLOG_INFO("failed to alloc main buffer\n");
        goto destroy_main_buf_pool;
    }
    // g_sub_dump_pool = create_buffer_pool(raw_width, raw_height, raw_format, "sub dump buffer pool");
    // if (!g_sub_dump_pool) {
    //     CLOG_INFO("failed to create sub buffer pool\n");
    //     goto free_main_buffer;
    // }
    // ret = buffer_pool_continous_alloc(g_sub_dump_pool, BUFFER_NUM, 1);
    // if (ret) {
    //     CLOG_INFO("failed to alloc sub buffer\n");
    //     goto destroy_sub_buf_pool;
    // }
    // chenshuang
    ccDevAttr0.mipi_lane_num = sensor_info0.sensor_cfg[work_mode_id].lane_num;
    ccDevAttr0.mode = g_vc_mode_0;
    ccDevAttr0.main_vc = 0;
    ccDevAttr0.sub_vc = 0;
    ccDevAttr0.main_dt = 0x2b;
    ccDevAttr0.sub_dt = 0x2b;
    CLOG_INFO("ccic_test  ccDevAttr0.mode :%d\n", ccDevAttr0.mode);
    ret = ASR_CCIC_SetDevAttr(ccic_id, &ccDevAttr0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetDevAttr dev(%d) failed\n", ccic_id);
        goto free_raw_buffers;
    }
    ret = ASR_CCIC_EnableDev(ccic_id);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableDev dev(%d) failed\n", ccic_id);
        goto free_raw_buffers;
    }

    CCU_GET_CCIC_MAIN_CHN(ccic_id, mainChn0);
    ccChnAttr0.enPixFormat = CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_10BPP;
    ccChnAttr0.width = sensor_info0.sensor_cfg[work_mode_id].width;
    ccChnAttr0.height = sensor_info0.sensor_cfg[work_mode_id].height;
    CLOG_INFO("ccic_test  main ccDevAttr0.size  %d x %d\n", ccChnAttr0.width, ccChnAttr0.height);
    ret = ASR_CCIC_SetChnAttr(mainChn0, &ccChnAttr0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetChnAttr failed\n");
        goto close_sensor;
    }
    ret = ASR_CCIC_SetCallback(mainChn0, buffer_callback);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetCallback failed\n");
        goto close_sensor;
    }
    while (true) {
        buffer = buffer_pool_get_buffer(g_main_dump_pool0);
        if (!buffer)
            break;
        memset(buffer->planes[0].virAddr, 0x00, buffer->planes[0].length);
        ret = ASR_CCIC_ChnQueueBuffer(mainChn0, buffer);
        if (ret) {
            CLOG_INFO("ASR_CCIC_ChnQueueBuffer failed\n");
            goto close_sensor;
        }
    }
    // CCU_GET_CCIC_SUB_CHN(ccic_id, subChn);
    // ccChnAttr0.enPixFormat = CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_12BPP;
    // ccChnAttr0.width = sensor_cap.sensor_capability.snr_config[work_mode_id].width;
    // ccChnAttr0.height = sensor_cap.sensor_capability.snr_config[work_mode_id].height;
    // CLOG_INFO("ccic_test  sub ccDevAttr0.size  %d x %d\n", ccChnAttr0.width, ccChnAttr0.height);
    // ret = ASR_CCIC_SetChnAttr(subChn, &ccChnAttr0);
    // if (ret) {
    //     CLOG_INFO("ASR_CCIC_SetChnAttr sub failed\n");
    //     goto close_sensor;
    // }
    // ret = ASR_CCIC_SetCallback(subChn, buffer_callback);
    // if (ret) {
    //     CLOG_INFO("ASR_CCIC_SetCallback sub failed");
    //     goto close_sensor;
    // }
    // while (true) {
    //     buffer = buffer_pool_get_buffer(g_sub_dump_pool);
    //     if (!buffer)
    //         break;
    //     memset(buffer->planes[0].virAddr, 0x00, buffer->planes[0].length);
        // ret = ASR_CCIC_ChnQueueBuffer(subChn, buffer);
        // if (ret) {
        //     CLOG_INFO("ASR_CCIC_ChnQueueBuffer sub failed\n");
        //     goto close_sensor;
        // }
    // }
    ret = ASR_CCIC_EnableChn(mainChn0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableChn failed\n");
        goto close_sensor;
    }
    // if (g_vc_mode_0 != 0) {
    //     ret = ASR_CCIC_EnableChn(subChn);
    //     if (ret) {
    //         CLOG_INFO("ASR_CCIC_EnableChn sub failed\n");
    //         goto disable_main_chn;
    //     }
    //     CLOG_INFO("ASR_CCIC_EnableChn sub \n");
    // }
    // stream on sensor
    // sensor_handle = sensorHandle->sensors_handle;
    ret = testSensorStart(sensorHandle);
    // ret = SPM_SENSOR_StreamOn(sensor_handle);
    if (ret) {
        stream_on0 = false;
        CLOG_INFO("SPM_SENSOR_StreamOn failed\n");
        goto disable_rawdump;
    } else {
        stream_on0 = true;
    }

    testAutoRunFlag0 = 1;
    condition_init(&testAutoRunCond0);

    streamOnFlags0 = 1;
    CLOG_INFO("sensor stream on");

    condition_wait(&testAutoRunCond0);

    streamOnFlags0 = 0;

    CLOG_INFO("sensor stream off");

    condition_deinit(&testAutoRunCond0);

disable_rawdump:
    // ASR_CCIC_DisableChn(subChn);
disable_main_chn:
    ASR_CCIC_DisableChn(mainChn0);
close_sensor:
    if (stream_on0) {
        testSensorStop(sensorHandle);
    }
    testSensorDeInit(sensorHandle);
free_raw_buffers:
    // free_sub_buffer:
    // buffer_pool_free(g_sub_dump_pool);
destroy_sub_buf_pool:
    // destroy_buffer_pool(g_sub_dump_pool);
free_main_buffer:
    buffer_pool_free(g_main_dump_pool0);
destroy_main_buf_pool:
    destroy_buffer_pool(g_main_dump_pool0);
disable_devs:
    ASR_CCIC_DisableDev(ccic_id);
    ASR_CCIC_Deinit();

    return 0;
}


int only_dual_ccic_test(struct testConfig *config)
{
    int ret = 0, ccic_id0, ccic_id1, i = 0;
    int work_mode_id0, work_mode_id1;
    int mainChn0 = 0, mainChn1;
    CCIC_DEV_ATTR_S ccDevAttr0, ccDevAttr1;
    CCIC_CHN_ATTR_S ccChnAttr0, ccChnAttr1;
    IMAGE_BUFFER_S rawDumpBufInfo;
    void* sensorHandle0 = NULL;
    void* sensorHandle1 = NULL;
    SENSOR_MODULE_INFO sensor_info0;
    SENSOR_MODULE_INFO sensor_info1;
    int raw_width0, raw_width1;
    int raw_height0, raw_height1;
    int raw_format0, raw_format1;
    int ccic_lane_0, ccic_lane_1;

    IMAGE_BUFFER_S *buffer = NULL;
    bool stream_on0 = false;
    bool stream_on1 = false;
    char sensors_name0[50];
    char sensors_name1[50];

    ccic_id0 = config->ispFeConfig[0].sensorId;
    ccic_id1 = config->ispFeConfig[1].sensorId;
    work_mode_id0 = config->ispFeConfig[0].sensorWorkMode;
    work_mode_id1 = config->ispFeConfig[1].sensorWorkMode;
    g_vc_mode_0 = config->ispFeConfig[0].vcMode;
    g_vc_mode_1 = config->ispFeConfig[1].vcMode;

    strcpy(sensors_name0, config->ispFeConfig[0].sensorName);
    strcpy(sensors_name1, config->ispFeConfig[0].sensorName);

    CLOG_INFO("ccic_test enter, sensors_name0 %s, ccic_id0 %d work_mode_id0 %d g_vc_mode_0 %d\n",
             sensors_name0, ccic_id0, work_mode_id0, g_vc_mode_0);
    CLOG_INFO("ccic_test enter, sensors_name1 %s, ccic_id0 %d work_mode_id1 %d g_vc_mode_1 %d\n",
             sensors_name1, ccic_id1, work_mode_id1, g_vc_mode_1);

    ret = testSensorInit(&sensorHandle0, config->ispFeConfig[0].sensorName,
                         config->ispFeConfig[0].sensorId, config->ispFeConfig[0].sensorWorkMode,
                         config);
    if (ret) {
        CLOG_ERROR("testSensorInit failed\n");
        return ret;
    }

    ret = testSensorGetDevInfo(sensorHandle0, &sensor_info0);
    if (ret) {
        CLOG_ERROR("testSensorGetDevInfo failed\n");
        goto dis_snr0;
    }

    ret = testSensorInit(&sensorHandle1, config->ispFeConfig[1].sensorName,
                         config->ispFeConfig[1].sensorId, config->ispFeConfig[1].sensorWorkMode,
                         config);
    if (ret) {
        CLOG_ERROR("testSensorInit failed\n");
        goto dis_snr0;
    }

    ret = testSensorGetDevInfo(sensorHandle1, &sensor_info1);
    if (ret) {
        CLOG_ERROR("testSensorGetDevInfo failed\n");
        goto dis_snr1;
    }

    ret = ASR_CCIC_Init();
    if (ret) {
        CLOG_ERROR("ASR_CCIC_Init failed\n");
        goto dis_snr1;
    }

    raw_width0 = sensor_info0.sensor_cfg[work_mode_id0].width;
    raw_height0 = sensor_info0.sensor_cfg[work_mode_id0].height;
    ccic_lane_0 = sensor_info0.sensor_cfg[work_mode_id0].lane_num;
    raw_format0 = PIXEL_FORMAT_RAW;
    ccDevAttr0.mipi_lane_num = ccic_lane_0;
    ccDevAttr0.mode = g_vc_mode_0;
    ccDevAttr0.main_vc = 0;
    ccDevAttr0.sub_vc = 0;
    ccDevAttr0.main_dt = 0x2b;
    ccDevAttr0.sub_dt = 0x2b;
    ccChnAttr0.enPixFormat = CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_10BPP;
    ccChnAttr0.width = sensor_info0.sensor_cfg[work_mode_id0].width;
    ccChnAttr0.height = sensor_info0.sensor_cfg[work_mode_id0].height;

    raw_width1 = sensor_info1.sensor_cfg[work_mode_id1].width;
    raw_height1 = sensor_info1.sensor_cfg[work_mode_id1].height;
    ccic_lane_1 = sensor_info1.sensor_cfg[work_mode_id1].lane_num;
    raw_format1 = PIXEL_FORMAT_RAW;
    ccDevAttr1.mipi_lane_num = ccic_lane_1;
    ccDevAttr1.mode = g_vc_mode_1;
    ccDevAttr1.main_vc = 0;
    ccDevAttr1.sub_vc = 0;
    ccDevAttr1.main_dt = 0x2b;
    ccDevAttr1.sub_dt = 0x2b;
    ccChnAttr1.enPixFormat = CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_10BPP;
    ccChnAttr1.width = sensor_info1.sensor_cfg[work_mode_id1].width;
    ccChnAttr1.height = sensor_info1.sensor_cfg[work_mode_id1].height;

    g_main_dump_pool0 = create_buffer_pool(raw_width0, raw_height0, raw_format0, "main dump buffer pool0");
    if (!g_main_dump_pool0) {
        CLOG_INFO("failed to create main buffer pool0\n");
        goto deinit_ccic;
    }
    ret = buffer_pool_continous_alloc(g_main_dump_pool0, BUFFER_NUM, 1);
    if (ret) {
        CLOG_INFO("failed to alloc main buffer0\n");
        goto destroy_buf_pool0;
    }
    g_main_dump_pool1 = create_buffer_pool(raw_width1, raw_height1, raw_format1, "main dump buffer pool1");
    if (!g_main_dump_pool1) {
        CLOG_INFO("failed to create main buffer pool1\n");
        goto free_main0_buffer;
    }
    ret = buffer_pool_continous_alloc(g_main_dump_pool1, BUFFER_NUM, 1);
    if (ret) {
        CLOG_INFO("failed to alloc main buffer1\n");
        goto destroy_buf_pool1;
    }

    CLOG_INFO("ccic_test  ccDevAttr0.mode :%d\n", ccDevAttr0.mode);
    ret = ASR_CCIC_SetDevAttr(ccic_id0, &ccDevAttr0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetDevAttr dev(%d) failed\n", ccic_id0);
        goto free_main1_buffer;
    }
    ret = ASR_CCIC_EnableDev(ccic_id0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableDev dev(%d) failed\n", ccic_id0);
        goto free_main1_buffer;
    }

    CLOG_INFO("ccic_test  ccDevAttr1.mode :%d\n", ccDevAttr1.mode);
    ret = ASR_CCIC_SetDevAttr(ccic_id1, &ccDevAttr1);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetDevAttr dev(%d) failed\n", ccic_id1);
        goto disable_devs0;
    }
    ret = ASR_CCIC_EnableDev(ccic_id1);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableDev dev(%d) failed\n", ccic_id1);
        goto disable_devs0;
    }

    CCU_GET_CCIC_MAIN_CHN(ccic_id0, mainChn0);
    CLOG_INFO("ccic_test  main ccDevAttr0.size  %d x %d\n", ccChnAttr0.width, ccChnAttr0.height);
    ret = ASR_CCIC_SetChnAttr(mainChn0, &ccChnAttr0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetChnAttr failed\n");
        goto disable_devs1;
    }
    ret = ASR_CCIC_SetCallback(mainChn0, buffer_callback);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetCallback failed\n");
        goto disable_devs1;
    }

    CCU_GET_CCIC_MAIN_CHN(ccic_id1, mainChn1);
    CLOG_INFO("ccic_test  main ccDevAttr1.size  %d x %d\n", ccChnAttr1.width, ccChnAttr1.height);
    ret = ASR_CCIC_SetChnAttr(mainChn1, &ccChnAttr1);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetChnAttr failed\n");
        goto disable_devs1;
    }
    ret = ASR_CCIC_SetCallback(mainChn1, buffer_callback2);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetCallback failed\n");
        goto disable_devs1;
    }

    while (true) {
        buffer = buffer_pool_get_buffer(g_main_dump_pool0);
        if (!buffer)
            break;
        memset(buffer->planes[0].virAddr, 0x00, buffer->planes[0].length);
        ret = ASR_CCIC_ChnQueueBuffer(mainChn0, buffer);
        if (ret) {
            CLOG_INFO("ASR_CCIC_ChnQueueBuffer failed\n");
            goto disable_devs1;
        }
    }

    while (true) {
        buffer = buffer_pool_get_buffer(g_main_dump_pool1);
        if (!buffer)
            break;
        memset(buffer->planes[0].virAddr, 0x00, buffer->planes[0].length);
        ret = ASR_CCIC_ChnQueueBuffer(mainChn1, buffer);
        if (ret) {
            CLOG_INFO("ASR_CCIC_ChnQueueBuffer failed\n");
            goto disable_devs1;
        }
    }

    ret = ASR_CCIC_EnableChn(mainChn0);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableChn failed\n");
        goto disable_devs1;
    }

    ret = testSensorStart(sensorHandle0);
    if (ret) {
        stream_on0 = false;
        CLOG_INFO("SPM_SENSOR_StreamOn failed\n");
        goto disable_main0_chn;
    } else {
        stream_on0 = true;
    }

    ret = ASR_CCIC_EnableChn(mainChn1);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableChn failed\n");
        goto disable_main0_chn;
    }

    ret = testSensorStart(sensorHandle1);
    if (ret) {
        stream_on1 = false;
        CLOG_INFO("SPM_SENSOR_StreamOn failed\n");
        goto disable_main1_chn;
    } else {
        stream_on1 = true;
    }

    testAutoRunFlag0 = 1;
    condition_init(&testAutoRunCond0);
    testAutoRunFlag1 = 1;
    condition_init(&testAutoRunCond1);

    streamOnFlags0 = 1;
    streamOnFlags1 = 1;
    CLOG_INFO("sensor stream on");

    condition_wait(&testAutoRunCond0);
    condition_wait(&testAutoRunCond1);
    streamOnFlags0 = 0;

    CLOG_INFO("sensor stream off");

    condition_deinit(&testAutoRunCond0);
    condition_deinit(&testAutoRunCond1);

disable_main1_chn:
    ASR_CCIC_DisableChn(mainChn1);
    if (stream_on1)
        testSensorStop(sensorHandle1);
disable_main0_chn:
    ASR_CCIC_DisableChn(mainChn0);
    if (stream_on0)
        testSensorStop(sensorHandle0);
disable_devs1:
    ASR_CCIC_DisableDev(ccic_id1);
disable_devs0:
    ASR_CCIC_DisableDev(ccic_id0);
free_main1_buffer:
    buffer_pool_free(g_main_dump_pool1);
destroy_buf_pool1:
    destroy_buffer_pool(g_main_dump_pool1);
free_main0_buffer:
    buffer_pool_free(g_main_dump_pool0);
destroy_buf_pool0:
    destroy_buffer_pool(g_main_dump_pool0);
deinit_ccic:
    ASR_CCIC_Deinit();

dis_snr1:
    if (sensorHandle1)
        testSensorDeInit(sensorHandle1);
dis_snr0:
    if (sensorHandle0)
        testSensorDeInit(sensorHandle0);

    return 0;
}
