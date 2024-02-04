/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#include "os05a10_asr.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const uint8_t module_i2c_addr = 0x36; /* I2C Address 7-bit*/

struct regval_tab os05a10_asr_vendor_id[] = {
    {0x300A, 0x53},
    {0x300B, 0x05},
    {0x300C, 0x41},
};

#define OS05A10_INCREASE_LINES             (0) /* make real fps less than stand fps because NVR require*/
#define OS05A10_VMAX_4M30_10bit_LINEAR     (0x640 + OS05A10_INCREASE_LINES)
#define OS05A10_LINETIME_4M30_10bit_LINEAR 29800  // ns
#define OS05A10_VMAX_4M30_10bit_HDR        (0x8a4 + OS05A10_INCREASE_LINES)
#define OS05A10_LINETIME_4M30_10bit_HDR    15000  // ns x2
#define OS05A10_VMAX_1080P30_10bit_LINEAR  (0x4b0 + OS05A10_INCREASE_LINES)
#if defined(OS05A10_1080P_16FPS)
#define OS05A10_LINETIME_1080P30_10bit_LINEAR 52100  // ns
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_7FPS)
#define OS05A10_LINETIME_1080P30_10bit_LINEAR 113600  // ns
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_1FPS)
#define OS05A10_LINETIME_1080P30_10bit_LINEAR 256360  // ns
#else
#define OS05A10_LINETIME_1080P30_10bit_LINEAR 27800  // ns
#endif
#define OS05A10_VMAX_1080P30_10bit_HDR        (0x3c0 + OS05A10_INCREASE_LINES)
#define OS05A10_LINETIME_1080P30_10bit_HDR    34300  // ns x2
#define OS05A10_VMAX_VGAP120_10bit_LINEAR     (0x240 + OS05A10_INCREASE_LINES)
#define OS05A10_LINETIME_VGAP120_10bit_LINEAR 14500  // ns

/********************************************************************/
static int os05a10_asr_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = OS05A10_ASR_WORK_MODE_SIZE;
    return 0;
}

static int os05a10_asr_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != OS05A10_ASR_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, OS05A10_ASR_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = OS05A10_ASR_WORK_MODE_SIZE;
    for (i = 0; i < OS05A10_ASR_WORK_MODE_SIZE; i++) {
        switch (i) {
            case OS05A10_ASR_1920x1080_10bit_LINEAR_30_1LANE: {
                sensor_capability->snr_config[i].width = 1920;
                sensor_capability->snr_config[i].height = 1080;
                sensor_capability->snr_config[i].bitDepth = 10;
#if defined(OS05A10_1080P_16FPS)
                sensor_capability->snr_config[i].maxFps = 16;
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_7FPS)
                sensor_capability->snr_config[i].maxFps = 7;
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_1FPS)
                sensor_capability->snr_config[i].maxFps = 1;
#else
                sensor_capability->snr_config[i].maxFps = 30;
#endif
                sensor_capability->snr_config[i].minFps = 5;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 1;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OS05A10_ASR_1920x1080_10bit_LINEAR_30_1LANE;
                sensor_capability->snr_config[i].setting = NULL;
            } break;
            case OS05A10_ASR_1920x1080_10bit_HDR_30_1LANE: {
                sensor_capability->snr_config[i].width = 1920;
                sensor_capability->snr_config[i].height = 1080;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 5;
                sensor_capability->snr_config[i].image_mode = SENSOR_HDR_MODE;
                sensor_capability->snr_config[i].lane_num = 1;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OS05A10_ASR_1920x1080_10bit_HDR_30_1LANE;
                sensor_capability->snr_config[i].setting = NULL;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           OS05A10_ASR_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int os05a10_asr_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = os05a10_asr_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(os05a10_asr_vendor_id);

    switch (work_mode) {
        case OS05A10_ASR_1920x1080_10bit_LINEAR_30_1LANE: {
            snr_info->linetime = OS05A10_LINETIME_1080P30_10bit_LINEAR;  // 0.1 us
            snr_info->vts = OS05A10_VMAX_1080P30_10bit_LINEAR;
#if defined(OS05A10_1080P_16FPS)
            snr_info->f32maxFps = 16;
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_7FPS)
            snr_info->f32maxFps = 7;
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_1FPS)
            snr_info->f32maxFps = 1;
#else
            snr_info->f32maxFps = 30;
#endif
            snr_info->exp_time[0] = 0x200 * snr_info->linetime / 10;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = os05a10_asr_1920x1080_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(os05a10_asr_1920x1080_10bit_30fps_tab);
#if defined(OS05A10_1080P_16FPS)
            snr_info->mipi_clock = 480;  // Mhz
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_7FPS)
            snr_info->mipi_clock = 354;  // Mhz
#elif defined(ZYNQ_OS05A10_1080P_MCLK29_MIPI354_1FPS)
            snr_info->mipi_clock = 354;  // Mhz
#else
            snr_info->mipi_clock = 960;  // Mhz
#endif
        } break;
        case OS05A10_ASR_1920x1080_10bit_HDR_30_1LANE: {
            snr_info->linetime = OS05A10_LINETIME_1080P30_10bit_HDR;  // 0.1 us
            snr_info->vts = OS05A10_VMAX_1080P30_10bit_HDR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x280 * snr_info->linetime / 10;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->exp_time[1] = 0x80 * snr_info->linetime / 10;
            snr_info->again[1] = 1 * 0x100;   // Q8 format
            snr_info->dgain[1] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_HDR_MODE;
            snr_info->setting_table = os05a10_asr_HDR_1920x1080_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(os05a10_asr_HDR_1920x1080_10bit_30fps_tab);
            snr_info->mipi_clock = 960;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       OS05A10_ASR_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = NULL;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = NULL;

    return 0;
}

static int os05a10_asr_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = os05a10_asr_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(os05a10_asr_vendor_id);

    return 0;
}

static int os05a10_asr_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S os05a10_asr_Obj = {
    .name = "os05a10_asr",
    .pfnGetsnrCapSize = os05a10_asr_get_sensor_cap_size,
    .pfnGetsnrCapbility = os05a10_asr_get_sensor_capbility,
    .pfnGetSnrWorkInfo = os05a10_asr_get_sensor_work_info,
    .pfnGetSnrVendorId = os05a10_asr_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = os05a10_asr_get_sensor_i2c_addr,
};
