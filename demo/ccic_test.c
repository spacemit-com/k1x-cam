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
BUFFER_POOL *g_main_dump_pool = NULL;
BUFFER_POOL *g_sub_dump_pool = NULL;

int g_vc_mode = 0;

static int testAutoRunFlag = 0;
static struct condition testAutoRunCond;
static int outputDumpFlag[MAX_PIPELINE_NUM] = {};
static int streamOnFlags = 0;

static char path[32] = "/data/vendor_de/camera/";

int writeToFile(const char *filename, IMAGE_BUFFER_S *buffer)
{
    //    uint32_t i;
    //    int offset;

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        CLOG_INFO("%s: open \"%s\" failed", __func__, filename);
        return -1;
    }

    size_t ret = fwrite((uint8_t *)buffer->planes[0].virAddr, buffer->planes[0].length, sizeof(char), fp);
    CLOG_INFO("%s: write \"%s\" successfully, len %zu", __func__, filename, ret);
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
        if (testAutoRunFlag == 1) {
            if (counter_0 == DUMP_FRAME_NUM)
                outputDumpFlag[pipelineId] = 1;
            if (counter_0 == AUTO_FRAME_NUM)
                condition_post(&testAutoRunCond);
        }
    } else {
        counter_1++;
        if (counter_1 % 100 == 0) {
            CLOG_INFO("chn(%u) frame(%u) done timeStamp(%lu)\n", nChn, frameId, timeStamp);
        }
        if (testAutoRunFlag == 1) {
            if (counter_1 == DUMP_FRAME_NUM)
                outputDumpFlag[pipelineId] = 1;
        }
    }

    if (outputDumpFlag[pipelineId] == 1) {
        sprintf(filename, "%sch%u_vcmode_%d_size_%dx%d_frameid_%d.raw", path, nChn, g_vc_mode, buffer->planes[0].width / 2,
                buffer->planes[0].height, frameId);
        writeToFile(filename, buffer);
        CLOG_INFO("dump the %d frame from ch:%d, write size:%d!\n", frameId, nChn, ret);
        outputDumpFlag[pipelineId] = 0;
    }

#endif
    memset((uint8_t *)buffer->planes[0].virAddr, 0, buffer->planes[0].length);
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
    int mainChn = 0, subChn = 0;
    g_vc_mode = config->ispFeConfig[0].vcMode;
    CCIC_DEV_ATTR_S ccDevAttr;
    CCIC_CHN_ATTR_S ccChnAttr;
    IMAGE_BUFFER_S rawDumpBufInfo;
    void *sensor_handle = NULL;
    SENSORS_MODULE_INFO_S sensor_module_info;
    SENSORS_MODULE_CAPABILITY_S sensor_cap;
    SENSOR_CUSTOM_S snr_custom;

    IMAGE_BUFFER_S *buffer = NULL;
    bool stream_on = false;
    char sensors_name[20];
    strcpy(sensors_name, config->ispFeConfig[0].sensorName);

    CLOG_INFO("ccic_test enter, sensors_name %s, ccic_id %d work_mode_id %d vc_mode %d\n", sensors_name, ccic_id, work_mode_id,
              g_vc_mode);

    snr_custom.dev_id = ccic_id;
    snr_custom.i2c_addr = -1;

    ret = SPM_SENSORS_MODULE_Detect(sensors_name, snr_custom);
    if (ret) {
        CLOG_INFO("detect sensor fail!\n");
        return ret;
    }
    ret = SPM_SENSORS_MODULE_Init(&sensor_handle, sensors_name, ccic_id, &sensor_module_info, -1);
    if (ret) {
        CLOG_INFO("sensor module init fail!\n");
        return ret;
    }
    sensor_cap.sensor_capability.snr_config_num = sensor_module_info.snr_config_num;
    if (sensor_cap.sensor_capability.snr_config_num <= 0) {
        CLOG_INFO("invalid sensor config number (%d)\n", sensor_cap.sensor_capability.snr_config_num);
        goto deinit_sensor;
    }
    sensor_cap.sensor_capability.snr_config =
        (SENSOR_CONFIG_S *)calloc(1, sensor_cap.sensor_capability.snr_config_num * sizeof(SENSOR_CONFIG_S));
    if (NULL == sensor_cap.sensor_capability.snr_config) {
        CLOG_INFO("failed to alloc mem for sensor configs\n");
        goto deinit_sensor;
    }
    ret = SPM_SENSORS_MODULE_EnumCapability(sensor_handle, &sensor_cap);
    if (0 == ret) {
        CLOG_INFO("sensor config info number %d\n", sensor_cap.sensor_capability.snr_config_num);
        for (i = 0; i < sensor_cap.sensor_capability.snr_config_num; i++) {
            CLOG_INFO("sensor_config_info[%d].width %d\n", i, sensor_cap.sensor_capability.snr_config[i].width);
            CLOG_INFO("sensor_config_info[%d].height %d\n", i, sensor_cap.sensor_capability.snr_config[i].height);
            CLOG_INFO("sensor_config_info[%d].pattern %d\n", i, sensor_cap.sensor_capability.snr_config[i].pattern);
            CLOG_INFO("sensor_config_info[%d].bitDepth %d\n", i, sensor_cap.sensor_capability.snr_config[i].bitDepth);
            CLOG_INFO("sensor_config_info[%d].fps %f - %f \n", i, sensor_cap.sensor_capability.snr_config[i].minFps,
                      sensor_cap.sensor_capability.snr_config[i].maxFps);
            CLOG_INFO("sensor_config_info[%d].image_mode %d\n", i,
                      sensor_cap.sensor_capability.snr_config[i].image_mode);
            CLOG_INFO("sensor_config_info[%d].lane_num %d\n", i, sensor_cap.sensor_capability.snr_config[i].lane_num);
            CLOG_INFO("sensor_config_info[%d].work_mode %d\n", i, sensor_cap.sensor_capability.snr_config[i].work_mode);
        }
    } else {
        CLOG_INFO("failed to enum sensor configs\n");
        goto free_sensor_configs;
    }
    // ret = SPM_SENSOR_Open(sensor_handle);
    // if (ret) {
    //     CLOG_INFO("failed to open sensor\n");
    //     goto free_sensor_configs;
    // }
    // ret = SPM_SENSOR_Config(sensor_handle, sensor_cap.sensor_capability.snr_config[0].work_mode);
    // if (ret) {
    //     CLOG_INFO("failed to config sensor\n");
    //     goto free_sensor_configs;
    // }
    // config ccic
    ASR_CCIC_Init();

    int raw_width = sensor_cap.sensor_capability.snr_config[work_mode_id].width;
    int raw_height = sensor_cap.sensor_capability.snr_config[work_mode_id].height;
    int raw_format = PIXEL_FORMAT_RAW;

    g_main_dump_pool = create_buffer_pool(raw_width, raw_height, raw_format, "main dump buffer pool");
    if (!g_main_dump_pool) {
        CLOG_INFO("failed to create main buffer pool\n");
        goto disable_devs;
    }
    ret = buffer_pool_continous_alloc(g_main_dump_pool, BUFFER_NUM, 1);
    if (ret) {
        CLOG_INFO("failed to alloc main buffer\n");
        goto destroy_main_buf_pool;
    }
    g_sub_dump_pool = create_buffer_pool(raw_width, raw_height, raw_format, "sub dump buffer pool");
    if (!g_sub_dump_pool) {
        CLOG_INFO("failed to create sub buffer pool\n");
        goto free_main_buffer;
    }
    ret = buffer_pool_continous_alloc(g_sub_dump_pool, BUFFER_NUM, 1);
    if (ret) {
        CLOG_INFO("failed to alloc sub buffer\n");
        goto destroy_sub_buf_pool;
    }
    // chenshuang
    ccDevAttr.mipi_lane_num = sensor_cap.sensor_capability.snr_config[work_mode_id].lane_num;
    ccDevAttr.mode = g_vc_mode;
    ccDevAttr.main_vc = 0;
    ccDevAttr.sub_vc = 0;
    ccDevAttr.main_dt = 0x2b;
    ccDevAttr.sub_dt = 0x2b;
    CLOG_INFO("ccic_test  ccDevAttr.mode :%d\n", ccDevAttr.mode);
    ret = ASR_CCIC_SetDevAttr(ccic_id, &ccDevAttr);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetDevAttr dev(%d) failed\n", ccic_id);
        goto free_raw_buffers;
    }
    ret = ASR_CCIC_EnableDev(ccic_id);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableDev dev(%d) failed\n", ccic_id);
        goto free_raw_buffers;
    }
    ret = SPM_SENSOR_Open(sensor_handle);
    if (ret) {
        CLOG_INFO("failed to open sensor\n");
        goto disable_devs;
    }
    ret = SPM_SENSOR_Config(sensor_handle, sensor_cap.sensor_capability.snr_config[work_mode_id].work_mode);
    if (ret) {
        CLOG_INFO("failed to config sensor\n");
        goto disable_devs;
    }
    CCU_GET_CCIC_MAIN_CHN(ccic_id, mainChn);
    ccChnAttr.enPixFormat = CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_10BPP;
    ccChnAttr.width = sensor_cap.sensor_capability.snr_config[work_mode_id].width;
    ccChnAttr.height = sensor_cap.sensor_capability.snr_config[work_mode_id].height;
    CLOG_INFO("ccic_test  main ccDevAttr.size  %d x %d\n", ccChnAttr.width, ccChnAttr.height);
    ret = ASR_CCIC_SetChnAttr(mainChn, &ccChnAttr);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetChnAttr failed\n");
        goto close_sensor;
    }
    ret = ASR_CCIC_SetCallback(mainChn, buffer_callback);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetCallback failed\n");
        goto close_sensor;
    }
    while (true) {
        buffer = buffer_pool_get_buffer(g_main_dump_pool);
        if (!buffer)
            break;
        memset(buffer->planes[0].virAddr, 0x00, buffer->planes[0].length);
        ret = ASR_CCIC_ChnQueueBuffer(mainChn, buffer);
        if (ret) {
            CLOG_INFO("ASR_CCIC_ChnQueueBuffer failed\n");
            goto close_sensor;
        }
    }
    CCU_GET_CCIC_SUB_CHN(ccic_id, subChn);
    ccChnAttr.enPixFormat = CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_10BPP;
    ccChnAttr.width = sensor_cap.sensor_capability.snr_config[work_mode_id].width;
    ccChnAttr.height = sensor_cap.sensor_capability.snr_config[work_mode_id].height;
    CLOG_INFO("ccic_test  sub ccDevAttr.size  %d x %d\n", ccChnAttr.width, ccChnAttr.height);
    ret = ASR_CCIC_SetChnAttr(subChn, &ccChnAttr);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetChnAttr sub failed\n");
        goto close_sensor;
    }
    ret = ASR_CCIC_SetCallback(subChn, buffer_callback);
    if (ret) {
        CLOG_INFO("ASR_CCIC_SetCallback sub failed");
        goto close_sensor;
    }
    while (true) {
        buffer = buffer_pool_get_buffer(g_sub_dump_pool);
        if (!buffer)
            break;
        memset(buffer->planes[0].virAddr, 0x00, buffer->planes[0].length);
        ret = ASR_CCIC_ChnQueueBuffer(subChn, buffer);
        if (ret) {
            CLOG_INFO("ASR_CCIC_ChnQueueBuffer sub failed\n");
            goto close_sensor;
        }
    }
    ret = ASR_CCIC_EnableChn(mainChn);
    if (ret) {
        CLOG_INFO("ASR_CCIC_EnableChn failed\n");
        goto close_sensor;
    }
    if (g_vc_mode != 0) {
        ret = ASR_CCIC_EnableChn(subChn);
        if (ret) {
            CLOG_INFO("ASR_CCIC_EnableChn sub failed\n");
            goto disable_main_chn;
        }
        CLOG_INFO("ASR_CCIC_EnableChn sub \n");
    }
    // stream on sensor
    ret = SPM_SENSOR_StreamOn(sensor_handle);
    if (ret) {
        stream_on = false;
        CLOG_INFO("SPM_SENSOR_StreamOn failed\n");
        goto disable_rawdump;
    } else {
        stream_on = true;
    }

    testAutoRunFlag = 1;
    condition_init(&testAutoRunCond);

    ret = SPM_SENSOR_StreamOn(sensor_handle);
    if (ret) {
        stream_on = false;
        CLOG_INFO("SPM_SENSOR_StreamOn failed\n");
        goto disable_rawdump;
    } else {
        stream_on = true;
    }
    streamOnFlags = 1;
    CLOG_INFO("sensor stream on");

    condition_wait(&testAutoRunCond);

    streamOnFlags = 0;

    CLOG_INFO("sensor stream off");

    condition_deinit(&testAutoRunCond);

disable_rawdump:
    ASR_CCIC_DisableChn(subChn);
disable_main_chn:
    ASR_CCIC_DisableChn(mainChn);
close_sensor:
    if (stream_on) {
        SPM_SENSOR_StreamOff(sensor_handle);
    }
    SPM_SENSOR_Close(sensor_handle);
free_raw_buffers:
    // free_sub_buffer:
    buffer_pool_free(g_sub_dump_pool);
destroy_sub_buf_pool:
    destroy_buffer_pool(g_sub_dump_pool);
free_main_buffer:
    buffer_pool_free(g_main_dump_pool);
destroy_main_buf_pool:
    destroy_buffer_pool(g_main_dump_pool);
disable_devs:
    ASR_CCIC_DisableDev(ccic_id);
    ASR_CCIC_Deinit();
free_sensor_configs:
    free(sensor_cap.sensor_capability.snr_config);
deinit_sensor:
    SPM_SENSORS_MODULE_Deinit(sensor_handle);
    return 0;
}
